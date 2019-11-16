#include <mitsuba/render/integrator.h>
#include <mitsuba/render/records.h>

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Depth integrator (for debugging). Returns the distance from the
 * camera to the closest intersected object, or 0 if no intersection was found.
 */
template <typename Float, typename Spectrum>
class DepthIntegrator final : public SamplingIntegrator<Float, Spectrum> {
public:
    MTS_DECLARE_CLASS_VARIANT(DepthIntegrator, SamplingIntegrator)
    MTS_IMPORT_TYPES(Scene, Sampler)

    DepthIntegrator(const Properties &props) : Base(props) {}

    std::pair<Spectrum, Mask> sample(const Scene *scene, Sampler * /*sampler*/,
                                     const RayDifferential3f &ray, Mask active) const override {
        SurfaceInteraction3f si = scene->ray_intersect(ray, active);
        return { select(si.is_valid(), si.t, 0.f), si.is_valid() };
    }
};

MTS_EXPORT_PLUGIN(DepthIntegrator, "Depth integrator");
NAMESPACE_END(mitsuba)
