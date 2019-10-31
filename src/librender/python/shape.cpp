#include <mitsuba/python/python.h>
#include <mitsuba/core/struct.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/medium.h>
#include <mitsuba/render/mesh.h>
#include <mitsuba/render/shape.h>

MTS_PY_EXPORT_CLASS_VARIANTS(Shape) {
    using Ray3f = typename Shape::Ray3f;
    using Mask = typename Shape::Mask;
    using Index = typename Shape::Index;
    using BoundingBox3f = typename Shape::BoundingBox3f;

    auto shape = MTS_PY_CLASS(Shape, Object)
        .def_method(Shape, sample_position, "time"_a, "sample"_a, "active"_a = true)
        .def_method(Shape, pdf_position, "ps"_a, "active"_a = true)
        .def_method(Shape, sample_direction, "it"_a, "sample"_a, "active"_a = true)
        .def_method(Shape, pdf_direction, "it"_a, "ps"_a, "active"_a = true)
        .def("ray_intersect",
             py::overload_cast<const Ray3f &, Mask>(&Shape::ray_intersect, py::const_),
             "ray"_a, "active"_a = true, D(Shape, ray_intersect))
        .def_method(Shape, ray_test, "ray"_a, "active"_a = true)
        .def_method(Shape, fill_surface_interaction, "ray"_a, "cache"_a, "si"_a, "active"_a = true)
        .def("bbox", py::overload_cast<>(
            &Shape::bbox, py::const_), D(Shape, bbox))
        .def("bbox", py::overload_cast<Index>(
            &Shape::bbox, py::const_), D(Shape, bbox, 2), "index"_a)
        .def("bbox", py::overload_cast<Index, const BoundingBox3f &>(
            &Shape::bbox, py::const_), D(Shape, bbox, 3), "index"_a, "clip"_a)
        .def_method(Shape, surface_area)
        .def_method(Shape, normal_derivative, "si"_a, "shading_frame"_a = true, "active"_a = true)
        .def_method(Shape, id)
        .def_method(Shape, is_mesh)
        .def_method(Shape, is_medium_transition)
        .def_method(Shape, interior_medium)
        .def_method(Shape, exterior_medium)
        .def_method(Shape, is_emitter)
        .def_method(Shape, is_sensor)
        .def("emitter", py::overload_cast<bool>(&Shape::emitter, py::const_), "active"_a = true)
        .def("sensor", py::overload_cast<>(&Shape::sensor, py::const_))
        .def_method(Shape, primitive_count)
        .def_method(Shape, effective_primitive_count)
        ;

    // TODO figure out that logic
    // if constexpr (is_array_v<Float> && !is_dynamic_v<Float>) {
    //     shape.def("sample_position", enoki::vectorize_wrapper(&Shape::sample_position),
    //         "time"_a, "sample"_a, "active"_a = true, D(Shape, sample_position))
    //     .def("pdf_position", enoki::vectorize_wrapper(&Shape::pdf_position),
    //             "ps"_a, "active"_a = true, D(Shape, pdf_position))
    //     .def("sample_direction", enoki::vectorize_wrapper(&Shape::sample_direction),
    //             "it"_a, "sample"_a, "active"_a = true, D(Shape, sample_direction))
    //     .def("pdf_direction", enoki::vectorize_wrapper(&Shape::pdf_direction),
    //             "it"_a, "ps"_a, "active"_a = true, D(Shape, pdf_direction))
    //     .def("normal_derivative", enoki::vectorize_wrapper(&Shape::normal_derivative),
    //             "si"_a, "shading_frame"_a = true, "active"_a = true,
    //             D(Shape, normal_derivative))
    //     // TODO
    //     // .def("ray_intersect",
    //     //      enoki::vectorize_wrapper(
    //     //          py::overload_cast<const Ray3fP &, MaskP>(&Shape::ray_intersect, py::const_)),
    //     //      "ray"_a, "active"_a = true, D(Shape, ray_intersect))
    //     ;
    // }

    using Mesh = Mesh<Float, Spectrum>;
    using Size = typename Mesh::Size;

    MTS_PY_CLASS(Mesh, Shape)
        .def(py::init<const std::string &, Struct *, Size, Struct *, Size>(),
             D(Mesh, Mesh))
        .def_method(Mesh, vertex_struct)
        .def_method(Mesh, face_struct)
        .def_method(Mesh, has_vertex_normals)
        .def_method(Mesh, has_vertex_texcoords)
        .def_method(Mesh, has_vertex_colors)
        .def_method(Mesh, write)
        .def_method(Mesh, recompute_vertex_normals)
        .def_method(Mesh, recompute_bbox)
        .def("vertices", [](py::object &o) {
            Mesh &m = py::cast<Mesh&>(o);
            py::dtype dtype = o.attr("vertex_struct")().attr("dtype")();
            return py::array(dtype, m.vertex_count(), m.vertices(), o);
        }, D(Mesh, vertices))
        .def("faces", [](py::object &o) {
            Mesh &m = py::cast<Mesh&>(o);
            py::dtype dtype = o.attr("face_struct")().attr("dtype")();
            return py::array(dtype, m.face_count(), m.faces(), o);
        }, D(Mesh, faces))
        ;
}
