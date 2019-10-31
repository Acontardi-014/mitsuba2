#include <mitsuba/core/rfilter.h>
#include <mitsuba/python/python.h>

MTS_PY_EXPORT(rfilter) {
    using Resampler = mitsuba::Resampler<Float>;

    auto rfilter = MTS_PY_CLASS(ReconstructionFilter, Object)
        .def_method(ReconstructionFilter, border_size)
        .def_method(ReconstructionFilter, radius)
        .def("eval",
             py::overload_cast<Float>(&ReconstructionFilter::eval, py::const_),
             D(ReconstructionFilter, eval), "x"_a)
        .def("eval",
             enoki::vectorize_wrapper(py::overload_cast<FloatP>(&ReconstructionFilter::eval, py::const_)),
             D(ReconstructionFilter, eval), "x"_a)
#if defined(MTS_ENABLE_AUTODIFF)
        .def("eval",
             py::overload_cast<FloatD>(&ReconstructionFilter::eval, py::const_),
             D(ReconstructionFilter, eval), "x"_a)
#endif
        .def("eval_discretized",
             &ReconstructionFilter::eval_discretized<Float>,
             D(ReconstructionFilter, eval_discretized), "x"_a, "active"_a = true)
        .def("eval_discretized",
             vectorize_wrapper(
                 &ReconstructionFilter::eval_discretized<FloatP>),
             D(ReconstructionFilter, eval_discretized), "x"_a, "active"_a = true);

    auto bc = py::enum_<FilterBoundaryCondition>(rfilter, "FilterBoundaryCondition")
        .value("EClamp", FilterBoundaryCondition::Clamp, D(ReconstructionFilter, FilterBoundaryCondition, EClamp))
        .value("ERepeat", FilterBoundaryCondition::Repeat, D(ReconstructionFilter, FilterBoundaryCondition, ERepeat))
        .value("EMirror", FilterBoundaryCondition::Mirror, D(ReconstructionFilter, FilterBoundaryCondition, EMirror))
        .value("EZero", FilterBoundaryCondition::Zero, D(ReconstructionFilter, FilterBoundaryCondition, EZero))
        .value("EOne", FilterBoundaryCondition::One, D(ReconstructionFilter, FilterBoundaryCondition, EOne))
        .export_values();

    auto resampler = py::class_<Resampler>(m, "Resampler", D(Resampler))
        .def(py::init<const ReconstructionFilter *, uint32_t, uint32_t>(),
             "rfilter"_a, "source_res"_a, "target_res"_a,
             D(Resampler, Resampler))
        .def_method(Resampler, source_resolution)
        .def_method(Resampler, target_resolution)
        .def_method(Resampler, boundary_condition)
        .def_method(Resampler, set_boundary_condition)
        .def_method(Resampler, set_clamp)
        .def_method(Resampler, taps)
        .def_method(Resampler, clamp)
        .def("__repr__", &Resampler::to_string)
        .def("resample",
             [](Resampler &resampler, const py::array &source,
                uint32_t source_stride, py::array &target,
                uint32_t target_stride, uint32_t channels) {
                 if (!source.dtype().is(py::dtype::of<Float>()))
                     throw std::runtime_error(
                         "'source' has an incompatible type!");
                 if (!target.dtype().is(py::dtype::of<Float>()))
                     throw std::runtime_error(
                         "'target' has an incompatible type!");
                 if (resampler.source_resolution() * source_stride != (size_t) source.size())
                     throw std::runtime_error(
                         "'source' has an incompatible size!");
                 if (resampler.target_resolution() * target_stride != (size_t) target.size())
                     throw std::runtime_error(
                         "'target' has an incompatible size!");

                 resampler.resample((const Float *) source.data(), source_stride,
                                    (Float *) target.mutable_data(), target_stride,
                                    channels);
             },
             D(Resampler, resample), "self"_a, "source"_a, "source_stride"_a,
             "target_stride"_a, "channels"_a);

    resampler.attr("FilterBoundaryCondition") = bc;

    m.attr("MTS_FILTER_RESOLUTION") = MTS_FILTER_RESOLUTION;
}
