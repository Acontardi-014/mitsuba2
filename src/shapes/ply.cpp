#include <mitsuba/render/shape.h>
#include <mitsuba/core/fstream.h>
#include <mitsuba/core/mstream.h>
#include <mitsuba/core/fresolver.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/logger.h>
#include <mitsuba/core/struct.h>
#include <simdfloat/static.h>
#include <unordered_map>
#include <fstream>

NAMESPACE_BEGIN(mitsuba)

class PLYMesh : public Shape {
public:
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

    PLYMesh(const Properties &props) {
        auto fs = Thread::thread()->fileResolver();
        fs::path filePath = fs->resolve(props.string("filename"));

		Log(EInfo, "Loading geometry from \"%s\" ..", filePath.filename());
		if (!fs::exists(filePath))
			Log(EError, "PLY file \"%s\" could not be found!", filePath);

        ref<Stream> stream = new FileStream(filePath, false);

        auto header = parsePLYHeader(stream);
        if (header.ascii)
            stream = parseASCII((FileStream *) stream.get(), header.elements);

        for (auto const &el : header.elements) {
        }
    }

    PLYHeader parsePLYHeader(Stream *stream) {
        Struct::EByteOrder byteOrder = Struct::hostByteOrder();
        bool plySpecifierProcessed = false;
        bool headerProcessed = false;
        PLYHeader header;

        std::unordered_map<std::string, Struct::EType> fmtMap;
        fmtMap["char"]   = Struct::EInt8;
        fmtMap["uchar"]  = Struct::EUInt8;
        fmtMap["short"]  = Struct::EInt16;
        fmtMap["ushort"] = Struct::EUInt16;
        fmtMap["int"]    = Struct::EInt32;
        fmtMap["uint"]   = Struct::EUInt32;
        fmtMap["float"]  = Struct::EFloat32;
        fmtMap["double"] = Struct::EFloat64;

        /* Unofficial extensions :) */
        fmtMap["long"]   = Struct::EInt64;
        fmtMap["ulong"]  = Struct::EUInt64;
        fmtMap["half"]   = Struct::EFloat16;

        ref<Struct> struct_;

        while (true) {
            std::string line = stream->readLine();
            std::istringstream iss(line);
            std::string token;
            if (!(iss >> token))
                continue;

            if (token == "comment") {
                std::getline(iss, line);
                header.comments.push_back(string::trim(line));
                continue;
            } else if (token == "ply") {
                if (plySpecifierProcessed)
                    Throw("Invalid PLY header: duplicate 'ply' tag!");
                plySpecifierProcessed = true;
                if (iss >> token)
                    Throw("Invalid PLY header: excess tokens after 'ply'");
            } else if (token == "format") {
                if (!plySpecifierProcessed)
                    Throw("Invalid PLY header: 'format' before 'ply' tag!");
                if (headerProcessed)
                    Throw("Invalid PLY header: duplicate 'format' tag!");
                if (!(iss >> token))
                    Throw("Invalid PLY header: missing token after 'format'");
                if (token == "ascii")
                    header.ascii = true;
                else if (token == "binary_little_endian")
                    byteOrder = Struct::ELittleEndian;
                else if (token == "binary_big_endian")
                    byteOrder = Struct::EBigEndian;
                else
                    Throw("Invalid PLY header: invalid token after 'format'");
                if (!(iss >> token))
                    Throw("Invalid PLY header: missing version number after 'format'");
                if (token != "1.0")
                    Throw("PLY file has unknown version number '%s'", token);
                if (iss >> token)
                    Throw("Invalid PLY header: excess tokens after 'format'");
                headerProcessed = true;
            } else if (token == "element") {
                if (!(iss >> token))
                    Throw("Invalid PLY header: missing token after 'element'");
                header.elements.emplace_back();
                auto &element = header.elements.back();
                element.name = token;
                if (!(iss >> token))
                    Throw("Invalid PLY header: missing token after 'element'");
                element.count = (size_t) stoull(token);
                struct_ = element.struct_ = new Struct(true, byteOrder);
            } else if (token == "property") {
                if (!headerProcessed)
                    Throw("Invalid PLY header: encountered 'property' before 'format'!");
                if (header.elements.empty())
                    Throw("Invalid PLY header: encountered 'property' before 'element'!");
                if (!(iss >> token))
                    Throw("Invalid PLY header: missing token after 'property'");

                if (token == "list") {
                    if (!(iss >> token))
                        Throw("Invalid PLY header: missing token after 'property list'");
                    auto it1 = fmtMap.find(token);
                    if (it1 == fmtMap.end())
                        Throw("Invalid PLY header: unknown format type '%s'", token);

                    if (!(iss >> token))
                        Throw("Invalid PLY header: missing token after 'property list'");
                    auto it2 = fmtMap.find(token);
                    if (it2 == fmtMap.end())
                        Throw("Invalid PLY header: unknown format type '%s'", token);

                    if (!(iss >> token))
                        Throw("Invalid PLY header: missing token after 'property list'");

                    struct_->append(token + ".count", it1->second, Struct::EAssert, 3);
                    for (int i = 0; i<3; ++i)
                        struct_->append(tfm::format("%s.i%i", token, i), it2->second);
                } else {
                    auto it = fmtMap.find(token);
                    if (it == fmtMap.end())
                        Throw("Invalid PLY header: unknown format type '%s'", token);
                    if (!(iss >> token))
                        Throw("Invalid PLY header: missing token after 'property'");
                    int flags = 0;
                    if (it->second >= Struct::EInt8 && it->second <= Struct::EUInt64)
                        flags |= Struct::ENormalized | Struct::EGamma;
                    struct_->append(token, it->second, flags);
                }

                if (iss >> token)
                    Throw("Invalid PLY header: excess tokens after 'property'");
            } else if (token == "end_header") {
                if (iss >> token)
                    Throw("Invalid PLY header: excess tokens after 'end_header'");
                break;
            } else {
                Throw("Invalid PLY header: unknown token '%s'", token);
            }
        }
        if (!headerProcessed)
            Throw("Invalid PLY file: no header information");
        return header;
    }

    ref<Stream> parseASCII(FileStream *in, const std::vector<PLYElement> &elements) {
        ref<Stream> out = new MemoryStream();
        std::fstream &is = *in->native();
        for (auto const &el : elements) {
            for (size_t i = 0; i < el.count; ++i) {
                for (auto const &field : *(el.struct_)) {
                    switch (field.type) {
                        case Struct::EInt8: {
                                int8_t value;
                                if (!(is >> value)) Throw("Could not parse 'char' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EUInt8: {
                                uint8_t value;
                                if (!(is >> value)) Throw("Could not parse 'uchar' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EInt16: {
                                int16_t value;
                                if (!(is >> value)) Throw("Could not parse 'short' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EUInt16: {
                                uint16_t value;
                                if (!(is >> value)) Throw("Could not parse 'ushort' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EInt32: {
                                int32_t value;
                                if (!(is >> value)) Throw("Could not parse 'int' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EUInt32: {
                                uint32_t value;
                                if (!(is >> value)) Throw("Could not parse 'uint' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EInt64: {
                                int64_t value;
                                if (!(is >> value)) Throw("Could not parse 'long' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EUInt64: {
                                uint64_t value;
                                if (!(is >> value)) Throw("Could not parse 'ulong' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EFloat16: {
                                float value;
                                if (!(is >> value)) Throw("Could not parse 'half' value");
                                out->write(simd::half::float32_to_float16(value));
                            }
                            break;

                        case Struct::EFloat32: {
                                float value;
                                if (!(is >> value)) Throw("Could not parse 'float' value");
                                out->write(value);
                            }
                            break;

                        case Struct::EFloat64: {
                                double value;
                                if (!(is >> value)) Throw("Could not parse 'double' value");
                                out->write(value);
                            }
                            break;

                        default:
                            Throw("Internal error!");
                    }
                }
            }
        }
        std::string token;
        if (is >> token)
            Throw("Trailing tokens after end of PLY file");
        return out;
    }

    void doSomething() override { }

    MTS_DECLARE_CLASS()
};

MTS_IMPLEMENT_CLASS(PLYMesh, Shape)
MTS_EXPORT_PLUGIN(PLYMesh, "PLY Mesh")

NAMESPACE_END(mitsuba)
