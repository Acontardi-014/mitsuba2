#include <mitsuba/python/python.h>
#include <mitsuba/render/fresnel.h>

MTS_PY_EXPORT(fresnel) {
    // --------------------------------------------------------------------

    m.def("fresnel", &fresnel<float>, D(fresnel), "cos_theta_i"_a, "eta"_a);
    m.def("fresnel", vectorize_wrapper(&fresnel<FloatP>), "cos_theta_i"_a, "eta"_a);

    // --------------------------------------------------------------------

    m.def("fresnel_conductor", &fresnel_conductor<float>, D(fresnel_conductor), "cos_theta_i"_a, "eta"_a);
    m.def("fresnel_conductor", vectorize_wrapper(&fresnel_conductor<FloatP>), "cos_theta_i"_a, "eta"_a);

    // --------------------------------------------------------------------

    m.def("fresnel_polarized", py::overload_cast<float, Complex<float>>(&fresnel_polarized<float>), D(fresnel_polarized, 2), "cos_theta_i"_a, "eta"_a);
    m.def("fresnel_polarized", vectorize_wrapper(py::overload_cast<FloatP, Complex<FloatP>>(&fresnel_polarized<FloatP>)), "cos_theta_i"_a, "eta"_a);

    // --------------------------------------------------------------------

    m.def("reflect",
            py::overload_cast<const Vector3f &>(&reflect<Vector3f>),
            "wi"_a, D(reflect));
    m.def("reflect", vectorize_wrapper(
            py::overload_cast<const Vector3fP &>(&reflect<Vector3fP>)),
            "wi"_a);

    // --------------------------------------------------------------------

    m.def("reflect",
            py::overload_cast<const Vector3f &, const Normal3f &>(&reflect<Vector3f, Normal3f>),
            "wi"_a, "m"_a, D(reflect, 2));
    m.def("reflect", vectorize_wrapper(
            py::overload_cast<const Vector3fP &, const Normal3fP &>(&reflect<Vector3fP, Normal3fP>)),
            "wi"_a, "m"_a);

    // --------------------------------------------------------------------

    m.def("refract",
          py::overload_cast<const Vector3f &, Float, Float>(
              &refract<Vector3f, Float>),
          "wi"_a, "cos_theta_t"_a, "eta_ti"_a, D(refract));
    m.def(
        "refract",
        vectorize_wrapper(py::overload_cast<const Vector3fP &, FloatP, FloatP>(
            &refract<Vector3fP, FloatP>)),
        "wi"_a, "cos_theta_t"_a, "eta_ti"_a);

    // --------------------------------------------------------------------

    m.def("refract",
          py::overload_cast<const Vector3f &, const Normal3f &, Float, Float>(
              &refract<Vector3f, Normal3f, Float>),
          "wi"_a, "m"_a, "cos_theta_t"_a, "eta_ti"_a, D(refract, 2));
    m.def(
        "refract",
        vectorize_wrapper(py::overload_cast<const Vector3fP &,
                                            const Normal3fP &, FloatP, FloatP>(
            &refract<Vector3fP, Normal3fP, FloatP>)),
        "wi"_a, "m"_a, "cos_theta_t"_a, "eta_ti"_a);
}
