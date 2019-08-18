#include <mitsuba/core/properties.h>
#include <mitsuba/python/python.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/render/texture3d.h>

/// Trampoline for derived types implemented in Python
class PyTexture3D : public Texture3D {
public:
    using Texture3D::Texture3D;

    Spectrumf eval(const Interaction3f &it) const override {
        PYBIND11_OVERLOAD(Spectrumf, Texture3D, eval, it);
    }
    SpectrumfP eval(const Interaction3fP &it, MaskP active) const override {
        PYBIND11_OVERLOAD(SpectrumfP, Texture3D, eval, it, active);
    }
#if defined(MTS_ENABLE_AUTODIFF)
    SpectrumfD eval(const Interaction3fD &it,
                    const MaskD &active) const override {
        PYBIND11_OVERLOAD(SpectrumfD, Texture3D, eval, it, active);
    }
#endif
    Float mean() const override { PYBIND11_OVERLOAD(Float, Texture3D, mean); }
    Float max() const override { PYBIND11_OVERLOAD(Float, Texture3D, max); }
    std::string to_string() const override {
        PYBIND11_OVERLOAD(std::string, Texture3D, to_string);
    }
};

MTS_PY_EXPORT(Texture3D) {

    MTS_PY_TRAMPOLINE_CLASS(PyTexture3D, Texture3D, DifferentiableObject)
        .def(py::init<const Properties &>())
        .def("eval",
             py::overload_cast<const Interaction3f &, bool>(&Texture3D::eval,
                                                            py::const_),
             D(Texture3D, eval), "it"_a, "active"_a = true)
        .def("eval",
             vectorize_wrapper(py::overload_cast<const Interaction3fP &, MaskP>(
                 &Texture3D::eval, py::const_)),
             D(Texture3D, eval), "it"_a, "active"_a = true)
#if defined(MTS_ENABLE_AUTODIFF)
        .def("eval",
             py::overload_cast<const Interaction3fD &, const MaskD &>(
                 &Texture3D::eval, py::const_),
             D(Texture3D, eval), "it"_a, "active"_a = true)
#endif
        .def("eval_gradient",
             py::overload_cast<const Interaction3f &, bool>(&Texture3D::eval_gradient,
                                                            py::const_),
             D(Texture3D, eval_gradient), "it"_a, "active"_a = true)
        .def("eval_gradient",
             vectorize_wrapper(py::overload_cast<const Interaction3fP &, MaskP>(
                 &Texture3D::eval_gradient, py::const_)),
             D(Texture3D, eval_gradient), "it"_a, "active"_a = true)
#if defined(MTS_ENABLE_AUTODIFF)
        .def("eval_gradient",
             py::overload_cast<const Interaction3fD &, const MaskD &>(
                 &Texture3D::eval_gradient, py::const_),
             D(Texture3D, eval_gradient), "it"_a, "active"_a = true)
#endif
        .mdef(Texture3D, mean)
        .mdef(Texture3D, max)
        .mdef(Texture3D, bbox)
        .def("__repr__", &Texture3D::to_string);
}
