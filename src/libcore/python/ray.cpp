#include <mitsuba/core/ray.h>
#include <mitsuba/python/python.h>
#include <mitsuba/render/autodiff.h>

template <typename Type, typename... Args, typename... Args2>
auto bind_ray(Args2&&... args2) {
    using Wavelength = typename Type::Wavelength;

    return py::class_<Type, Args...>(args2...)
        .def(py::init<>(), "Create an unitialized ray")
        .def(py::init<const Type &>(), "Copy constructor", "other"_a)
        .def(py::init<Point3f, Vector3f, Float, const Wavelength &>(),
             D(Ray, Ray, 5), "o"_a, "d"_a, "time"_a, "wavelength"_a)
        .def(py::init<Point3f, Vector3f, Float, Float, Float, const Wavelength &>(),
             D(Ray, Ray, 6), "o"_a, "d"_a, "mint"_a, "maxt"_a, "time"_a, "wavelength"_a)
        .def(py::init<const Type &, Float, Float>(),
             D(Ray, Ray, 7), "other"_a, "mint"_a, "maxt"_a)
        .def_method(Ray, update)
        .def("__call__", &Type::operator(), D(Ray, operator, call))
        .def_repr(Type)
        .def_field(Ray, o)
        .def_field(Ray, d)
        .def_field(Ray, d_rcp)
        .def_field(Ray, mint)
        .def_field(Ray, maxt)
        .def_field(Ray, time)
        .def_field(Ray, wavelength)
        ;
}

template <typename Type, typename... Args, typename... Args2>
auto bind_ray_differential(Args2&&... args2) {
    return bind_ray<Type, Args...>(args2...)
        .def_field(RayDifferential, o_x)
        .def_field(RayDifferential, o_y)
        .def_field(RayDifferential, d_x)
        .def_field(RayDifferential, d_y)
        .def_field(RayDifferential, has_differentials);
}

MTS_PY_EXPORT(Ray) {
    bind_ray<Ray3f>(m, "Ray3f", D(Ray));
    auto r3fx = bind_ray<Ray3fX>(m, "Ray3fX", D(Ray));
    bind_slicing_operators<Ray3fX, Ray3f>(r3fx);

#if defined(MTS_ENABLE_AUTODIFF)
    auto r3fd = bind_ray<Ray3fD>(m, "Ray3fD", D(Ray));
    bind_slicing_operators<Ray3fD, Ray3f>(r3fd);
#endif

    bind_ray_differential<RayDifferential3f, Ray3f>(m, "RayDifferential3f", D(RayDifferential));
    auto rd3fx = bind_ray_differential<RayDifferential3fX, Ray3fX>(m, "RayDifferential3fX", D(RayDifferential));
    bind_slicing_operators<RayDifferential3fX, RayDifferential3f>(rd3fx);

#if defined(MTS_ENABLE_AUTODIFF)
    auto rd3fd = bind_ray_differential<RayDifferential3fD, Ray3fD>(m, "RayDifferential3fD", D(RayDifferential));
    bind_slicing_operators<RayDifferential3fD, RayDifferential3f>(rd3fd);
#endif
}
