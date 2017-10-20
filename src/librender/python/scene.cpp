#include <mitsuba/render/scene.h>
#include <mitsuba/render/kdtree.h>
#include <mitsuba/render/mesh.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/python/python.h>

MTS_PY_EXPORT(ShapeKDTree) {
    MTS_PY_CLASS(ShapeKDTree, Object)
        .def(py::init<const Properties &>(), D(ShapeKDTree, ShapeKDTree))
        .mdef(ShapeKDTree, add_shape)
        .mdef(ShapeKDTree, primitive_count)
        .mdef(ShapeKDTree, shape_count)
        .def("shape", (Shape *(ShapeKDTree::*)(size_t)) &ShapeKDTree::shape, D(ShapeKDTree, shape))
        .def("__getitem__", [](ShapeKDTree &s, size_t i) -> py::object {
            if (i >= s.primitive_count())
                throw py::index_error();
            Shape *shape = s.shape(i);
            if (shape->class_()->derives_from(MTS_CLASS(Mesh)))
                return py::cast(static_cast<Mesh *>(s.shape(i)));
            else
                return py::cast(s.shape(i));
    })
        .def("__len__", &ShapeKDTree::primitive_count)
        .def("bbox", [] (ShapeKDTree &s) { return s.bbox(); })
        .mdef(ShapeKDTree, build)
        .def("ray_intersect_havran",       &ShapeKDTree::ray_intersect_havran<false>)
        .def("ray_intersect_dummy_scalar", &ShapeKDTree::ray_intersect_dummy<false, Ray3f>)
        .def("ray_intersect_dummy_packet", vectorize_wrapper(&ShapeKDTree::ray_intersect_dummy<false, Ray3fP>))
        .def("ray_intersect_pbrt_scalar",  &ShapeKDTree::ray_intersect_pbrt<false, Ray3f>)
        .def("ray_intersect_pbrt_packet",  vectorize_wrapper(&ShapeKDTree::ray_intersect_pbrt<false, Ray3fP>));
}

MTS_PY_EXPORT(Scene) {
    MTS_PY_CLASS(Scene, Object)
        .def("kdtree", (ShapeKDTree *(Scene::*)()) &Scene::kdtree, D(Scene, kdtree));
}
