#include <mitsuba/python/python.h>
#include <mitsuba/render/shape.h>
#include <mitsuba/render/records.h>
#include <mitsuba/render/scene.h>

MTS_PY_EXPORT_VARIANTS(PositionSample) {
    MTS_IMPORT_TYPES()
    MTS_PY_CHECK_ALIAS(PositionSample3f, m) {
        py::class_<PositionSample3f>(m, "PositionSample3f", D(PositionSample3f))
            .def(py::init<>(), "Construct an unitialized position sample")
            .def(py::init<const PositionSample3f &>(), "Copy constructor", "other"_a)
            .def(py::init<const SurfaceInteraction3f &>(), "si"_a, D(PositionSample3f, PositionSample3f))
            .def_field(PositionSample3f, p)
            .def_field(PositionSample3f, n)
            .def_field(PositionSample3f, uv)
            .def_field(PositionSample3f, time)
            .def_field(PositionSample3f, pdf)
            .def_field(PositionSample3f, delta)
            .def_field(PositionSample3f, object)
            .def_repr(PositionSample3f);
    }
}

MTS_PY_EXPORT_VARIANTS(DirectionSample) {
    MTS_IMPORT_TYPES()
    MTS_IMPORT_OBJECT_TYPES()
    using Base = typename DirectionSample3f::Base;
    MTS_PY_CHECK_ALIAS(DirectionSample3f, m) {
        py::class_<DirectionSample3f, Base>(m, "DirectionSample3f", D(DirectionSample3f))
            .def(py::init<>(), "Construct an unitialized direct sample")
            .def(py::init<const Base &>(), "Construct from a position sample", "other"_a)
            .def(py::init<const DirectionSample3f &>(), "Copy constructor", "other"_a)
            .def(py::init<const Point3f &, const Normal3f &, const Point2f &,
                        const Float &, const Float &, const Mask &,
                        const ObjectPtr &, const Vector3f &, const Float &>(),
                "p"_a, "n"_a, "uv"_a, "time"_a, "pdf"_a, "delta"_a, "object"_a, "d"_a, "dist"_a,
                "Element-by-element constructor")
            .def(py::init<const SurfaceInteraction3f &, const Interaction3f &>(),
                "si"_a, "ref"_a, D(PositionSample3f, PositionSample3f))
            .def_method(DirectionSample3f, set_query)
            .def_field(DirectionSample3f, d)
            .def_field(DirectionSample3f, dist)
            .def_repr(DirectionSample3f);
    }
}
