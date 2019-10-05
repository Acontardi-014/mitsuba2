#include <mitsuba/core/fresolver.h>
#include <mitsuba/core/bsphere.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/render/scene.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/kdtree.h>
#include <mitsuba/render/spectrum.h>
#include <mitsuba/core/plugin.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class ConstantBackgroundEmitter final : public Emitter<Float, Spectrum> {
public:
    MTS_IMPORT_TYPES()
    using Base               = Emitter<Float, Spectrum>;
    using Scene              = typename Aliases::Scene;
    using Shape              = typename Aliases::Shape;
    using ContinuousSpectrum = typename Aliases::ContinuousSpectrum;


    ConstantBackgroundEmitter(const Properties &props) : Base(props) {
        /* Until `create_shape` is called, we have no information
           about the scene and default to the unit bounding sphere. */
        m_bsphere = BoundingSphere3f(Point3f(0.f), 1.f);

        m_radiance =
            props.spectrum<Float, Spectrum>("radiance", ContinuousSpectrum::D65(1.f));
    }

    ref<Shape> create_shape(const Scene *scene) override {
        // Create a bounding sphere that surrounds the scene
        m_bsphere = scene->bbox().bounding_sphere();
        m_bsphere.radius = max(math::Epsilon<Float>, m_bsphere.radius * 1.5f);

        Properties props("sphere");
        props.set_point3f("center", m_bsphere.center);
        props.set_float("radius", m_bsphere.radius);

        // Sphere is "looking in" towards the scene
        props.set_bool("flip_normals", true);

        return PluginManager::instance()->create_object<Shape>(props);
    }

    MTS_INLINE Spectrum eval_impl(const SurfaceInteraction3f &si, Mask active) const {
        return m_radiance->eval(si.wavelengths, active);
    }

    MTS_INLINE auto sample_ray_impl(Float time, Float wavelength_sample, const Point2f &sample2,
                                    const Point2f &sample3, Mask active) const {
        // 1. Sample spectrum
        auto [wavelengths, weight] = m_radiance->sample(
            math::sample_shifted<Spectrum>(wavelength_sample), active);

        // 2. Sample spatial component
        Vector3f v0 = warp::square_to_uniform_sphere(sample2);

        // 3. Sample directional component
        Vector3f v1 = warp::square_to_cosine_hemisphere(sample3);

        Float r2 = m_bsphere.radius * m_bsphere.radius;
        return std::make_pair(Ray3f(m_bsphere.center + v0 * m_bsphere.radius,
                                    Frame3f(-v0).to_world(v1), time, wavelengths),
                              (4 * math::Pi<Float> * math::Pi<Float> * r2) * weight);
    }

    MTS_INLINE auto sample_direction_impl(const Interaction3f &it, const Point2f &sample,
                                          Mask active) const {
        Vector3f d = warp::square_to_uniform_sphere(sample);
        Float dist = 2.f * m_bsphere.radius;

        DirectionSample3f ds;
        ds.p      = it.p + d * dist;
        ds.n      = -d;
        ds.uv     = Point2f(0.f);
        ds.time   = it.time;
        ds.pdf    = warp::square_to_uniform_sphere_pdf(d);
        ds.delta  = false;
        ds.object = this;
        ds.d      = d;
        ds.dist   = dist;

        return std::make_pair(
            ds,
            m_radiance->eval(it.wavelengths, active) / ds.pdf
        );
    }

    MTS_INLINE Float pdf_direction_impl(const Interaction3f &, const DirectionSample3f &ds,
                                        Mask) const {
        return warp::square_to_uniform_sphere_pdf(ds.d);
    }

    /// This emitter does not occupy any particular region of space, return an invalid bounding box
    BoundingBox3f bbox() const override {
        return BoundingBox3f();
    }

    bool is_environment() const override {
        return true;
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "ConstantBackgroundEmitter[" << std::endl
            << "  radiance = " << string::indent(m_radiance) << "," << std::endl
            << "  bsphere = " << m_bsphere << "," << std::endl
            << "]";
        return oss.str();
    }

    // MTS_IMPLEMENT_EMITTER_ALL()
    MTS_DECLARE_CLASS()
protected:
    ref<ContinuousSpectrum> m_radiance;
    BoundingSphere3f m_bsphere;
};

// MTS_IMPLEMENT_CLASS(ConstantBackgroundEmitter, Emitter)
// MTS_EXPORT_PLUGIN(ConstantBackgroundEmitter, "Constant background emitter");
NAMESPACE_END(mitsuba)
