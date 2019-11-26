#include <mitsuba/python/python.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/render/medium.h>
#include <mitsuba/render/records.h>
#include <mitsuba/render/scene.h>

MTS_PY_EXPORT_STRUCT(Interaction) {
    MTS_IMPORT_TYPES()
    MTS_PY_CHECK_ALIAS(Interaction3f, m) {
        auto inter = py::class_<Interaction3f>(m, "Interaction3f", D(Interaction3f))
            // Members
            .def_field(Interaction3f, t)
            .def_field(Interaction3f, time)
            .def_field(Interaction3f, wavelengths)
            .def_field(Interaction3f, p)
            // Methods
            .def(py::init<>(), D(Interaction3f, Interaction3f))
            .def_method(Interaction3f, spawn_ray)
            .def_method(Interaction3f, spawn_ray_to)
            .def_method(Interaction3f, is_valid)
            .def_repr(Interaction3f);
        bind_slicing_operators<Interaction3f, Interaction<ScalarFloat, scalar_spectrum_t<Spectrum>>>(inter);
    }
}

MTS_PY_EXPORT_STRUCT(SurfaceInteraction) {
    MTS_IMPORT_TYPES()
    MTS_PY_CHECK_ALIAS(SurfaceInteraction3f, m) {
        auto inter = py::class_<SurfaceInteraction3f, Interaction3f>(m, "SurfaceInteraction3f", D(SurfaceInteraction3f))
            // Members
            .def_field(SurfaceInteraction3f, shape)
            .def_field(SurfaceInteraction3f, uv)
            .def_field(SurfaceInteraction3f, n)
            .def_field(SurfaceInteraction3f, sh_frame)
            .def_field(SurfaceInteraction3f, dp_du)
            .def_field(SurfaceInteraction3f, dp_dv)
            .def_field(SurfaceInteraction3f, duv_dx)
            .def_field(SurfaceInteraction3f, duv_dy)
            .def_field(SurfaceInteraction3f, wi)
            .def_field(SurfaceInteraction3f, prim_index)
            .def_field(SurfaceInteraction3f, instance)
            // // Methods
            .def(py::init<>(), D(SurfaceInteraction3f, SurfaceInteraction3f))
            .def(py::init<const PositionSample3f &, const Wavelength &>(),
                "ps"_a, "wavelengths"_a, D(SurfaceInteraction3f, SurfaceInteraction3f))
            .def_method(SurfaceInteraction3f, to_world)
            .def_method(SurfaceInteraction3f, to_local)
            .def_method(SurfaceInteraction3f, to_world_mueller,
                        "M_local"_a, "wi_local"_a, "wo_local"_a)
            .def_method(SurfaceInteraction3f, to_local_mueller,
                        "M_world"_a, "wi_world"_a, "wo_world"_a)
            .def_method(SurfaceInteraction3f, emitter, "scene"_a, "active"_a = true)
            .def_method(SurfaceInteraction3f, is_sensor)
            .def_method(SurfaceInteraction3f, is_medium_transition)
            .def("target_medium",
                py::overload_cast<const Vector3f &>(&SurfaceInteraction3f::target_medium, py::const_),
                "d"_a, D(SurfaceInteraction3f, target_medium))
            .def("target_medium",
                py::overload_cast<const Float &>(&SurfaceInteraction3f::target_medium, py::const_),
                "cos_theta"_a, D(SurfaceInteraction3f, target_medium, 2))
            .def("bsdf", py::overload_cast<const RayDifferential3f &>(&SurfaceInteraction3f::bsdf),
                "ray"_a, D(SurfaceInteraction3f, bsdf))
            .def("bsdf", py::overload_cast<>(&SurfaceInteraction3f::bsdf, py::const_),
                D(SurfaceInteraction3f, bsdf, 2))
            // .def_method(SurfaceInteraction3f, normal_derivative)
            .def_method(SurfaceInteraction3f, compute_partials)
            .def_method(SurfaceInteraction3f, has_uv_partials)
            .def_repr(SurfaceInteraction3f);

        // TODO
        // bind_slicing_operators<SurfaceInteraction3f, SurfaceInteraction<ScalarFloat, scalar_spectrum_t<Spectrum>>>(inter);
    }
}
