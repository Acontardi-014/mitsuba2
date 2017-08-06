#include <mitsuba/render/records.h>
#include <mitsuba/python/python.h>

template <typename Point3>
auto bind_position_sample(py::module &m, const char *name) {
    using Type = PositionSample<Point3>;
    using SurfaceInteraction = typename Type::SurfaceInteraction;

    return py::class_<Type>(m, name, D(PositionSample))
        .def(py::init<>(), "Construct an unitialized position sample")
        .def(py::init<const Type &>(), "Copy constructor")
        .def(py::init<Float>(), D(PositionSample, PositionSample))
        .def(py::init<const SurfaceInteraction &, EMeasure>(),
             "intersection"_a, "measure"_a = ESolidAngle,
             D(PositionSample, PositionSample, 2))
        .def_readwrite("p", &Type::p, D(PositionSample, p))
        .def_readwrite("time", &Type::time, D(PositionSample, time))
        .def_readwrite("n", &Type::n, D(PositionSample, n))
        .def_readwrite("pdf", &Type::pdf, D(PositionSample, pdf))
        .def_readwrite("measure", &Type::measure, D(PositionSample, measure))
        .def_readwrite("uv", &Type::uv, D(PositionSample, uv))
        .def_readwrite("object", &Type::object, D(PositionSample, object))
        .def("__repr__", [](const Type &ps) {
            std::ostringstream oss;
            oss << ps;
            return oss.str();
        });
}

MTS_PY_EXPORT(PositionSample) {
    bind_position_sample<Point3f>(m, "PositionSample3f");
    auto ps3fx = bind_position_sample<Point3fX>(m, "PositionSample3fX");
    bind_slicing_operators<PositionSample3fX, PositionSample3f>(ps3fx);
}

// -----------------------------------------------------------------------------

template <typename Vector3> auto bind_direction_sample(py::module &m, const char *name) {
    using Type = DirectionSample<Vector3>;
    using SurfaceInteraction = typename Type::SurfaceInteraction;

    return py::class_<Type>(m, name, D(DirectionSample))
        .def(py::init<>(), "Construct an unitialized direction sample")
        .def(py::init<const Type &>(), "Copy constructor")
        .def(py::init<const Vector3 &, EMeasure>(), "d"_a,
             "measure"_a = ESolidAngle, D(DirectionSample, DirectionSample))
        .def(py::init<const SurfaceInteraction &, EMeasure>(),
             "intersection"_a, "measure"_a = ESolidAngle,
             D(DirectionSample, DirectionSample, 2))
        .def_readwrite("d", &Type::d, D(DirectionSample, d))
        .def_readwrite("pdf", &Type::pdf, D(DirectionSample, pdf))
        .def_readwrite("measure", &Type::measure, D(DirectionSample, measure))
        .def("__repr__", [](const Type &record) {
            std::ostringstream oss;
            oss << record;
            return oss.str();
        });
}

MTS_PY_EXPORT(DirectionSample) {
    bind_direction_sample<Vector3f>(m, "DirectionSample3f");
    auto ds3fx = bind_direction_sample<Vector3fX>(m, "DirectionSample3fX");
    bind_slicing_operators<DirectionSample3fX, DirectionSample3f>(ds3fx);
}

// -----------------------------------------------------------------------------

template <typename Point3, typename Base>
auto bind_direct_sample(py::module &m, const char *name) {
    using Type = DirectSample<Point3>;

    return py::class_<Type, Base>(m, name, D(DirectSample))
        .def(py::init<>(), "Construct an unitialized direct sample")
        .def(py::init<const Type &>(), "Copy constructor")
        .def(py::init<const Point3f &, Float>(),
             D(DirectSample, DirectSample))
        .def(py::init<const SurfaceInteraction3f &>(),
             D(DirectSample, DirectSample, 2))
        //.def(py::init<const MediumInteraction3f &>(), D(DirectSample,
        // DirectSample, 3))
        .def(py::init<const Ray3f &, const SurfaceInteraction3f, EMeasure>(),
             "ray"_a, "intersection"_a, "measure"_a = ESolidAngle,
             D(DirectSample, DirectSample, 4))
        .def_readwrite("ref_p", &Type::ref_p, D(DirectSample, ref_p))
        .def_readwrite("ref_n", &Type::ref_n, D(DirectSample, ref_n))
        .def_readwrite("d", &Type::d, D(DirectSample, d))
        .def_readwrite("dist", &Type::dist, D(DirectSample, dist))
        .def("__repr__", [](const Type &record) {
            std::ostringstream oss;
            oss << record;
            return oss.str();
        });
}

MTS_PY_EXPORT(DirectSample) {
    bind_direct_sample<Point3f, PositionSample3f>(m, "DirectSample3f");
    auto ds3fx = bind_direct_sample<Point3fX, PositionSample3fX>(m, "DirectSample3fX");
    bind_slicing_operators<DirectSample3fX, DirectSample3f>(ds3fx);
}
