#include <mitsuba/render/mesh.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/core/fstream.h>
#include <mitsuba/core/mstream.h>
#include <mitsuba/core/fresolver.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/util.h>
#include <mitsuba/core/timer.h>
#include <enoki/half.h>
#include <unordered_map>
#include <fstream>

NAMESPACE_BEGIN(mitsuba)

class PLYMesh final : public Mesh {
public:
    MTS_DECLARE_CLASS(PLYMesh, Mesh)

    struct PLYElement {
        std::string name;
        size_t count;
        ref<Struct> struct_;
    };

    struct PLYHeader {
        bool ascii = false;
        std::vector<std::string> comments;
        std::vector<PLYElement> elements;
    };

    PLYMesh(const Properties &props) : Mesh(props) {
        /// Process vertex/index records in large batches
        constexpr size_t elements_per_packet = 1024;

        auto fs = Thread::thread()->file_resolver();
        fs::path file_path = fs->resolve(props.string("filename"));
        m_name = file_path.filename().string();

        auto fail = [&](const char *descr) {
            Throw("Error while loading PLY file \"%s\": %s!", m_name, descr);
        };

        Log(Debug, "Loading mesh from \"%s\" ..", m_name);
        if (!fs::exists(file_path))
            fail("file not found");

        ref<Stream> stream = new FileStream(file_path);
        Timer timer;

        PLYHeader header;
        try {
            header = parse_ply_header(stream);
            if (header.ascii) {
                if (stream->size() > 100 * 1024)
                    Log(Warn,
                        "\"%s\": performance warning -- this file uses the ASCII PLY format, which "
                        "is slow to parse. Consider converting it to the binary PLY format.",
                        m_name);
                stream = parse_ascii((FileStream *) stream.get(), header.elements);
            }
        } catch (const std::exception &e) {
            fail(e.what());
        }

        bool has_vertex_normals = false;
        for (auto &el : header.elements) {
            size_t size = el.struct_->size();
            if (el.name == "vertex") {
                m_vertex_struct = new Struct();

                for (auto name : { "x", "y", "z" })
                    m_vertex_struct->append(name, struct_type_v<Float>);

                if (!m_disable_vertex_normals) {
                    for (auto name : { "nx", "ny", "nz" })
                        m_vertex_struct->append(name, struct_type_v<Float>, FieldFlags::Default, 0.0);

                    if (el.struct_->has_field("nx") &&
                        el.struct_->has_field("ny") &&
                        el.struct_->has_field("nz"))
                        has_vertex_normals = true;

                    m_normal_offset = (Index) m_vertex_struct->field("nx").offset;
                }

                if (el.struct_->has_field("u") && el.struct_->has_field("v")) {
                    /* all good */
                } else if (el.struct_->has_field("texture_u") &&
                           el.struct_->has_field("texture_v")) {
                    el.struct_->field("texture_u").name = "u";
                    el.struct_->field("texture_v").name = "v";
                } else if (el.struct_->has_field("s") &&
                           el.struct_->has_field("t")) {
                    el.struct_->field("s").name = "u";
                    el.struct_->field("t").name = "v";
                }
                if (el.struct_->has_field("u") && el.struct_->has_field("v")) {
                    for (auto name : { "u", "v" })
                        m_vertex_struct->append(name, struct_type_v<Float>);

                    m_texcoord_offset = (Index) m_vertex_struct->field("u").offset;
                }

                size_t i_struct_size = el.struct_->size();
                size_t o_struct_size = m_vertex_struct->size();

                ref<StructConverter> conv;
                try {
                    conv = new StructConverter(el.struct_, m_vertex_struct);
                } catch (const std::exception &e) {
                    fail(e.what());
                }

                /* Allocate memory for vertices (+1 unused entry) */
                m_vertices = VertexHolder(new uint8_t[(el.count + 1) * o_struct_size]);

                /* Clear unused entry */
                memset(m_vertices.get() + o_struct_size * el.count, 0, o_struct_size);

                size_t packet_count     = el.count / elements_per_packet;
                size_t remainder_count  = el.count % elements_per_packet;
                size_t i_packet_size    = i_struct_size * elements_per_packet;
                size_t i_remainder_size = i_struct_size * remainder_count;

                std::unique_ptr<uint8_t[]> buf(new uint8_t[i_packet_size]);
                uint8_t *target = (uint8_t *) m_vertices.get();

                for (size_t i = 0; i <= packet_count; ++i) {
                    size_t psize = (i != packet_count) ? i_packet_size : i_remainder_size;
                    size_t count = (i != packet_count) ? elements_per_packet : remainder_count;

                    stream->read(buf.get(), psize);
                    if (unlikely(!conv->convert(count, buf.get(), target)))
                        fail("incompatible contents -- is this a triangle mesh?");

                    if (!has_vertex_normals) {
                        for (size_t j = 0; j < count; ++j) {
                            Point3f p = enoki::load<Point3f>(target);
                            p = m_to_world.transform_affine(p);
                            if (unlikely(!all(enoki::isfinite(p))))
                                fail("mesh contains invalid vertex positions/normal data");
                            m_bbox.expand(p);
                            enoki::store_unaligned(target, p);
                            target += o_struct_size;
                        }
                    } else {
                        for (size_t j = 0; j < count; ++j) {
                            Point3f p = enoki::load<Point3f>(target);
                            Normal3f n = enoki::load<Normal3f>(target + sizeof(Float) * 3);
                            n = normalize(m_to_world.transform_affine(n));
                            p = m_to_world.transform_affine(p);
                            if (unlikely(!all(enoki::isfinite(p) && enoki::isfinite(n))))
                                fail("mesh contains invalid vertex positions/normal data");
                            m_bbox.expand(p);
                            enoki::store_unaligned(target, p);
                            enoki::store_unaligned(target + sizeof(Float) * 3, n);
                            target += o_struct_size;
                        }
                    }
                }

                m_vertex_count = (Size) el.count;
                m_vertex_size = (Size) o_struct_size;
            } else if (el.name == "face") {
                m_face_struct = new Struct();

                std::string field_name;
                if (el.struct_->has_field("vertex_index.count"))
                    field_name = "vertex_index";
                else if (el.struct_->has_field("vertex_indices.count"))
                    field_name = "vertex_indices";
                else
                    fail("vertex_index/vertex_indices property not found");

                for (size_t i = 0; i < 3; ++i)
                    m_face_struct->append(tfm::format("i%i", i), struct_type_v<Index>);

                size_t i_struct_size = el.struct_->size();
                size_t o_struct_size = m_face_struct->size();

                ref<StructConverter> conv;
                try {
                    conv = new StructConverter(el.struct_, m_face_struct);
                } catch (const std::exception &e) {
                    fail(e.what());
                }

                m_faces = FaceHolder(new uint8_t[(el.count + 1) * o_struct_size]);

                size_t packet_count     = el.count / elements_per_packet;
                size_t remainder_count  = el.count % elements_per_packet;
                size_t i_packet_size    = i_struct_size * elements_per_packet;
                size_t o_packet_size    = o_struct_size * elements_per_packet;
                size_t i_remainder_size = i_struct_size * remainder_count;

                std::unique_ptr<uint8_t[]> buf(new uint8_t[i_packet_size]);
                uint8_t *target = (uint8_t *) m_faces.get();

                for (size_t i = 0; i <= packet_count; ++i) {
                    size_t psize = (i != packet_count) ? i_packet_size : i_remainder_size;
                    size_t count = (i != packet_count) ? elements_per_packet : remainder_count;

                    stream->read(buf.get(), psize);
                    if (unlikely(!conv->convert(count, buf.get(), target)))
                        fail("incompatible contents -- is this a triangle mesh?");

                    target += o_packet_size;
                }

                m_face_count = (Size) el.count;
                m_face_size = (Size) o_struct_size;
            } else {
                Log(Warn, "\"%s\": Skipping unknown element \"%s\"", m_name, el.name);
                stream->seek(stream->tell() + size * el.count);
            }
        }

        if (stream->tell() != stream->size())
            fail("invalid file -- trailing content");

        Log(Debug, "\"%s\": read %i faces, %i vertices (%s in %s)",
            m_name, m_face_count, m_vertex_count,
            util::mem_string(m_face_count * m_face_struct->size() +
                             m_vertex_count * m_vertex_struct->size()),
            util::time_string(timer.value())
        );

        if (!m_disable_vertex_normals && !has_vertex_normals)
            recompute_vertex_normals();

        if (is_emitter())
            emitter()->set_shape(this);
    }

    std::string type_name(const FieldType type) const {
        switch (type) {
            case FieldType::Int8:    return "char";
            case FieldType::UInt8:   return "uchar";
            case FieldType::Int16:   return "short";
            case FieldType::UInt16:  return "ushort";
            case FieldType::Int32:   return "int";
            case FieldType::UInt32:  return "uint";
            case FieldType::Int64:   return "long";
            case FieldType::UInt64:  return "ulong";
            case FieldType::Float16: return "half";
            case FieldType::Float32: return "float";
            case FieldType::Float64: return "double";
            default: Throw("internal error");
        }
    }

    void write(Stream *stream) const override {
        std::string stream_name = "<stream>";
        auto fs = dynamic_cast<FileStream *>(stream);
        if (fs)
            stream_name = fs->path().filename().string();

        Log(Info, "Writing mesh to \"%s\" ..", stream_name);

        Timer timer;
        stream->write_line("ply");
        if (Struct::host_byte_order() == FieldByteOrder::BigEndian)
            stream->write_line("format binary_big_endian 1.0");
        else
            stream->write_line("format binary_little_endian 1.0");

        if (m_vertex_struct->field_count() > 0) {
            stream->write_line(tfm::format("element vertex %i", m_vertex_count));
            for (auto const &f : *m_vertex_struct)
                stream->write_line(
                    tfm::format("property %s %s", type_name(f.type), f.name));
        }

        if (m_face_struct->field_count() > 0) {
            stream->write_line(tfm::format("element face %i", m_face_count));
            stream->write_line(tfm::format("property list uchar %s vertex_indices",
                type_name((*m_face_struct)[0].type)));
        }

        stream->write_line("end_header");

        if (m_vertex_struct->field_count() > 0) {
            stream->write(
                m_vertices.get(),
                m_vertex_struct->size() * m_vertex_count
            );
        }

        if (m_face_struct->field_count() > 0) {
            ref<Struct> face_struct_out = new Struct(true);

            face_struct_out->append("__size", FieldType::UInt8, FieldFlags::Default, 3.0);
            for (auto f: *m_face_struct)
                face_struct_out->append(f.name, f.type);

            ref<StructConverter> conv =
                new StructConverter(m_face_struct, face_struct_out);

            FaceHolder temp(new uint8_t[face_struct_out->size() * m_face_count]);

            if (!conv->convert(m_face_count, m_faces.get(), temp.get()))
                Throw("PLYMesh::write(): internal error during conversion");

            stream->write(
                temp.get(),
                face_struct_out->size() * m_face_count
            );
        }

        Log(Info, "\"%s\": wrote %i faces, %i vertices (%s in %s)",
            m_name, m_face_count, m_vertex_count,
            util::mem_string(m_face_count * m_face_struct->size() +
                             m_vertex_count * m_vertex_struct->size()),
            util::time_string(timer.value())
        );
    }

private:
    PLYHeader parse_ply_header(Stream *stream) {
        FieldByteOrder byte_order = Struct::host_byte_order();
        bool ply_tag_seen = false;
        bool header_processed = false;
        PLYHeader header;

        std::unordered_map<std::string, FieldType> fmt_map;
        fmt_map["char"]   = FieldType::Int8;
        fmt_map["uchar"]  = FieldType::UInt8;
        fmt_map["short"]  = FieldType::Int16;
        fmt_map["ushort"] = FieldType::UInt16;
        fmt_map["int"]    = FieldType::Int32;
        fmt_map["uint"]   = FieldType::UInt32;
        fmt_map["float"]  = FieldType::Float32;
        fmt_map["double"] = FieldType::Float64;

        /* Unofficial extensions :) */
        fmt_map["uint8"]   = FieldType::UInt8;
        fmt_map["uint16"]  = FieldType::UInt16;
        fmt_map["uint32"]  = FieldType::UInt32;
        fmt_map["int8"]    = FieldType::Int8;
        fmt_map["int16"]   = FieldType::Int16;
        fmt_map["int32"]   = FieldType::Int32;
        fmt_map["long"]    = FieldType::Int64;
        fmt_map["ulong"]   = FieldType::UInt64;
        fmt_map["half"]    = FieldType::Float16;
        fmt_map["float16"] = FieldType::Float16;
        fmt_map["float32"] = FieldType::Float32;
        fmt_map["float64"] = FieldType::Float64;

        ref<Struct> struct_;

        while (true) {
            std::string line = stream->read_line();
            std::istringstream iss(line);
            std::string token;
            if (!(iss >> token))
                continue;

            if (token == "comment") {
                std::getline(iss, line);
                header.comments.push_back(string::trim(line));
                continue;
            } else if (token == "ply") {
                if (ply_tag_seen)
                    Throw("invalid PLY header: duplicate \"ply\" tag");
                ply_tag_seen = true;
                if (iss >> token)
                    Throw("invalid PLY header: excess tokens after \"ply\"");
            } else if (token == "format") {
                if (!ply_tag_seen)
                    Throw("invalid PLY header: \"format\" before \"ply\" tag");
                if (header_processed)
                    Throw("invalid PLY header: duplicate \"format\" tag");
                if (!(iss >> token))
                    Throw("invalid PLY header: missing token after \"format\"");
                if (token == "ascii")
                    header.ascii = true;
                else if (token == "binary_little_endian")
                    byte_order = FieldByteOrder::LittleEndian;
                else if (token == "binary_big_endian")
                    byte_order = FieldByteOrder::BigEndian;
                else
                    Throw("invalid PLY header: invalid token after \"format\"");
                if (!(iss >> token))
                    Throw("invalid PLY header: missing version number after \"format\"");
                if (token != "1.0")
                    Throw("PLY file has unknown version number \"%s\"", token);
                if (iss >> token)
                    Throw("invalid PLY header: excess tokens after \"format\"");
                header_processed = true;
            } else if (token == "element") {
                if (!(iss >> token))
                    Throw("invalid PLY header: missing token after \"element\"");
                header.elements.emplace_back();
                auto &element = header.elements.back();
                element.name = token;
                if (!(iss >> token))
                    Throw("invalid PLY header: missing token after \"element\"");
                element.count = (size_t) stoull(token);
                struct_ = element.struct_ = new Struct(true, byte_order);
            } else if (token == "property") {
                if (!header_processed)
                    Throw("invalid PLY header: encountered \"property\" before \"format\"");
                if (header.elements.empty())
                    Throw("invalid PLY header: encountered \"property\" before \"element\"");
                if (!(iss >> token))
                    Throw("invalid PLY header: missing token after \"property\"");

                if (token == "list") {
                    if (!(iss >> token))
                        Throw("invalid PLY header: missing token after \"property list\"");
                    auto it1 = fmt_map.find(token);
                    if (it1 == fmt_map.end())
                        Throw("invalid PLY header: unknown format type \"%s\"", token);

                    if (!(iss >> token))
                        Throw("invalid PLY header: missing token after \"property list\"");
                    auto it2 = fmt_map.find(token);
                    if (it2 == fmt_map.end())
                        Throw("invalid PLY header: unknown format type \"%s\"", token);

                    if (!(iss >> token))
                        Throw("invalid PLY header: missing token after \"property list\"");

                    struct_->append(token + ".count", it1->second, FieldFlags::Assert, 3);
                    for (int i = 0; i<3; ++i)
                        struct_->append(tfm::format("i%i", i), it2->second);
                } else {
                    auto it = fmt_map.find(token);
                    if (it == fmt_map.end())
                        Throw("invalid PLY header: unknown format type \"%s\"", token);
                    if (!(iss >> token))
                        Throw("invalid PLY header: missing token after \"property\"");
                    int flags = 0;
                    if (it->second >= FieldType::Int8 && it->second <= FieldType::UInt64)
                        flags |= FieldFlags::Normalized | FieldFlags::Gamma;
                    struct_->append(token, it->second, flags);
                }

                if (iss >> token)
                    Throw("invalid PLY header: excess tokens after \"property\"");
            } else if (token == "end_header") {
                if (iss >> token)
                    Throw("invalid PLY header: excess tokens after \"end_header\"");
                break;
            } else {
                Throw("invalid PLY header: unknown token \"%s\"", token);
            }
        }
        if (!header_processed)
            Throw("invalid PLY file: no header information");
        return header;
    }

    ref<Stream> parse_ascii(FileStream *in, const std::vector<PLYElement> &elements) {
        ref<Stream> out = new MemoryStream();
        std::fstream &is = *in->native();
        for (auto const &el : elements) {
            for (size_t i = 0; i < el.count; ++i) {
                for (auto const &field : *(el.struct_)) {
                    switch (field.type) {
                        case FieldType::Int8: {
                                int value;
                                if (!(is >> value)) Throw("Could not parse \"char\" value for field %s", field.name);
                                if (value < -128 || value > 127)
                                    Throw("Could not parse \"char\" value for field %s", field.name);
                                out->write((int8_t) value);
                            }
                            break;

                        case FieldType::UInt8: {
                                int value;
                                if (!(is >> value))
                                    Throw("Could not parse \"uchar\" value for field %s (may be due to non-triangular faces)", field.name);
                                if (value < 0 || value > 255)
                                    Throw("Could not parse \"uchar\" value for field %s (may be due to non-triangular faces)", field.name);
                                out->write((uint8_t) value);
                            }
                            break;

                        case FieldType::Int16: {
                                int16_t value;
                                if (!(is >> value)) Throw("Could not parse \"short\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::UInt16: {
                                uint16_t value;
                                if (!(is >> value)) Throw("Could not parse \"ushort\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::Int32: {
                                int32_t value;
                                if (!(is >> value)) Throw("Could not parse \"int\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::UInt32: {
                                uint32_t value;
                                if (!(is >> value)) Throw("Could not parse \"uint\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::Int64: {
                                int64_t value;
                                if (!(is >> value)) Throw("Could not parse \"long\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::UInt64: {
                                uint64_t value;
                                if (!(is >> value)) Throw("Could not parse \"ulong\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::Float16: {
                                float value;
                                if (!(is >> value)) Throw("Could not parse \"half\" value for field %s", field.name);
                                out->write(enoki::half::float32_to_float16(value));
                            }
                            break;

                        case FieldType::Float32: {
                                float value;
                                if (!(is >> value)) Throw("Could not parse \"float\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        case FieldType::Float64: {
                                double value;
                                if (!(is >> value)) Throw("Could not parse \"double\" value for field %s", field.name);
                                out->write(value);
                            }
                            break;

                        default:
                            Throw("internal error");
                    }
                }
            }
        }
        std::string token;
        if (is >> token)
            Throw("Trailing tokens after end of PLY file");
        out->seek(0);
        return out;
    }
};

MTS_IMPLEMENT_PLUGIN(PLYMesh, Mesh, "PLY Mesh")
NAMESPACE_END(mitsuba)
