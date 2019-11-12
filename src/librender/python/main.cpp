#include <mitsuba/python/python.h>

// MTS_PY_DECLARE_VARIANTS(autodiff);
MTS_PY_DECLARE_VARIANTS(TransportMode);
// MTS_PY_DECLARE_VARIANTS(Scene);
// MTS_PY_DECLARE_VARIANTS(Shape);
// MTS_PY_DECLARE_VARIANTS(ShapeKDTree);
MTS_PY_DECLARE_VARIANTS(Interaction);
MTS_PY_DECLARE_VARIANTS(SurfaceInteraction);
MTS_PY_DECLARE_VARIANTS(Endpoint);
MTS_PY_DECLARE_VARIANTS(Emitter);
MTS_PY_DECLARE_VARIANTS(Sensor);
MTS_PY_DECLARE_VARIANTS(BSDF);
MTS_PY_DECLARE_VARIANTS(BSDFSample3f);
MTS_PY_DECLARE_VARIANTS(BSDFContext);
MTS_PY_DECLARE_VARIANTS(ImageBlock);
MTS_PY_DECLARE_VARIANTS(Film);
MTS_PY_DECLARE_VARIANTS(Spiral);
MTS_PY_DECLARE_VARIANTS(Integrator);
MTS_PY_DECLARE_VARIANTS(Sampler);
MTS_PY_DECLARE_VARIANTS(ContinuousSpectrum);
MTS_PY_DECLARE_VARIANTS(MicrofacetDistribution);
MTS_PY_DECLARE_VARIANTS(fresnel);
MTS_PY_DECLARE_VARIANTS(srgb);
MTS_PY_DECLARE_VARIANTS(mueller);
// MTS_PY_DECLARE_VARIANTS(Texture3D);

PYBIND11_MODULE(mitsuba_render_ext, m_) {
    (void) m_; /* unused */

    py::module m = py::module::import("mitsuba.render");

    // // MTS_PY_IMPORT_VARIANTS(autodiff);
    // MTS_PY_IMPORT_VARIANTS(TransportMode);
    // // MTS_PY_IMPORT_VARIANTS(Scene);
    // // MTS_PY_IMPORT_VARIANTS(Shape);
    // // MTS_PY_IMPORT_VARIANTS(ShapeKDTree);
    // MTS_PY_IMPORT_VARIANTS(Interaction);
    // MTS_PY_IMPORT_VARIANTS(SurfaceInteraction);
    // MTS_PY_IMPORT_VARIANTS(Endpoint);
    // MTS_PY_IMPORT_VARIANTS(Emitter);
    // MTS_PY_IMPORT_VARIANTS(Sensor);
    // MTS_PY_IMPORT_VARIANTS(BSDF);
    // MTS_PY_IMPORT_VARIANTS(BSDFSample3f);
    // MTS_PY_IMPORT_VARIANTS(BSDFContext);
    // MTS_PY_IMPORT_VARIANTS(ImageBlock);
    // MTS_PY_IMPORT_VARIANTS(Film);
    // MTS_PY_IMPORT_VARIANTS(Spiral);
    // MTS_PY_IMPORT_VARIANTS(Integrator);
    // MTS_PY_IMPORT_VARIANTS(Sampler);
    // MTS_PY_IMPORT_VARIANTS(ContinuousSpectrum);
    // MTS_PY_IMPORT_VARIANTS(MicrofacetDistribution);
    // MTS_PY_IMPORT_VARIANTS(fresnel);
    // MTS_PY_IMPORT_VARIANTS(srgb);
    // MTS_PY_IMPORT_VARIANTS(mueller);
    // // MTS_PY_IMPORT_VARIANTS(Texture3D);
}
