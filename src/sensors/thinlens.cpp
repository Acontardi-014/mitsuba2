#include <mitsuba/render/sensor.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/transform.h>
#include <mitsuba/core/bbox.h>
#include <mitsuba/core/warp.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class ThinLensCamera final : public ProjectiveCamera<Float, Spectrum> {
public:
    MTS_DECLARE_CLASS_VARIANT(ThinLensCamera, ProjectiveCamera)
    MTS_IMPORT_BASE(ProjectiveCamera, m_world_transform, m_needs_sample_3, m_film, m_sampler,
                    m_resolution, m_shutter_open, m_shutter_open_time, m_aspect, m_near_clip,
                    m_far_clip, m_focus_distance)
    MTS_IMPORT_TYPES()

    // =============================================================
    //! @{ \name Constructors
    // =============================================================

    ThinLensCamera(const Properties &props) : Base(props) {
        m_x_fov = parse_fov(props, m_aspect);

        m_aperture_radius = props.float_("aperture_radius");

        if (m_aperture_radius == 0.f) {
            Log(Warn, "Can't have a zero aperture radius -- setting to %f", math::Epsilon<Float>);
            m_aperture_radius = math::Epsilon<Float>;
        }

        if (m_world_transform->has_scale())
            Throw("Scale factors in the camera-to-world transformation are not allowed!");

        ScalarVector2f film_size   = ScalarVector2f(m_film->size()),
                       crop_size   = ScalarVector2f(m_film->crop_size()),
                       rel_size    = crop_size / film_size;

        ScalarPoint2f  crop_offset = ScalarPoint2f(m_film->crop_offset()),
                       rel_offset  = crop_offset / film_size;

        /**
         * These do the following (in reverse order):
         *
         * 1. Create transform from camera space to [-1,1]x[-1,1]x[0,1] clip
         *    coordinates (not taking account of the aspect ratio yet)
         *
         * 2+3. Translate and scale to shift the clip coordinates into the
         *    range from zero to one, and take the aspect ratio into account.
         *
         * 4+5. Translate and scale the coordinates once more to account
         *     for a cropping window (if there is any)
         */
        m_camera_to_sample =
            ScalarTransform4f::scale(ScalarVector3f(1.f / rel_size.x(), 1.f / rel_size.y(), 1.f)) *
            ScalarTransform4f::translate(ScalarVector3f(-rel_offset.x(), -rel_offset.y(), 0.f)) *
            ScalarTransform4f::scale(ScalarVector3f(-0.5f, -0.5f * m_aspect, 1.f)) *
            ScalarTransform4f::translate(ScalarVector3f(-1.f, -1.f / m_aspect, 0.f)) *
            ScalarTransform4f::perspective(m_x_fov, m_near_clip, m_far_clip);

        m_sample_to_camera = m_camera_to_sample.inverse();

        // Position differentials on the near plane
        m_dx = m_sample_to_camera * ScalarPoint3f(1.f / m_resolution.x(), 0.f, 0.f)
             - m_sample_to_camera * ScalarPoint3f(0.f);
        m_dy = m_sample_to_camera * ScalarPoint3f(0.f, 1.f / m_resolution.y(), 0.f)
             - m_sample_to_camera * ScalarPoint3f(0.f);

        /* Precompute some data for importance(). Please
           look at that function for further details. */
        ScalarPoint3f pmin(m_sample_to_camera * ScalarPoint3f(0.f, 0.f, 0.f)),
                      pmax(m_sample_to_camera * ScalarPoint3f(1.f, 1.f, 0.f));

        m_image_rect.reset();
        m_image_rect.expand(ScalarPoint2f(pmin.x(), pmin.y()) / pmin.z());
        m_image_rect.expand(ScalarPoint2f(pmax.x(), pmax.y()) / pmax.z());
        m_normalization = 1.f / m_image_rect.volume();
        m_needs_sample_3 = true;
    }

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Sampling methods (Sensor interface)
    // =============================================================

    std::pair<Ray3f, Spectrum> sample_ray(Float time, Float wavelength_sample,
                                          const Point2f &position_sample,
                                          const Point2f &aperture_sample,
                                          Mask active) const override {
        auto [wavelengths, wav_weight] = sample_wavelength<Float, Spectrum>(wavelength_sample);
        Ray3f ray;
        ray.time = time;
        ray.wavelength = wavelengths;

        // Compute the sample position on the near plane (local camera space).
        Point3f near_p = m_sample_to_camera *
                        Point3f(position_sample.x(), position_sample.y(), 0.f);

        // Aperture position
        Point2f tmp = m_aperture_radius * warp::square_to_uniform_disk_concentric(aperture_sample);
        Point3f aperture_p(tmp.x(), tmp.y(), 0.f);

        // Sampled position on the focal plane
        Point3f focus_p = near_p * (m_focus_distance / near_p.z());

        // Convert into a normalized ray direction; adjust the ray interval accordingly.
        Vector3f d = normalize(Vector3f(focus_p - aperture_p));
        Float inv_z = rcp(d.z());
        ray.mint = m_near_clip * inv_z;
        ray.maxt = m_far_clip * inv_z;

        auto trafo = m_world_transform->eval(ray.time, active);
        ray.o = trafo.transform_affine(aperture_p);
        ray.d = trafo * d;
        ray.update();

        return std::make_pair(ray, wav_weight);
    }

    std::pair<RayDifferential3f, Spectrum>
    sample_ray_differential_impl(Float time, Float wavelength_sample,
                                 const Point2f &position_sample, const Point2f &aperture_sample,
                                 Mask active) const {
        auto [wavelengths, wav_weight] = sample_wavelength<Float, Spectrum>(wavelength_sample);
        RayDifferential3f ray;
        ray.time = time;
        ray.wavelength = wavelengths;

        // Compute the sample position on the near plane (local camera space).
        Point3f near_p = m_sample_to_camera *
                        Point3f(position_sample.x(), position_sample.y(), 0.f);

        // Aperture position
        Point2f tmp = m_aperture_radius * warp::square_to_uniform_disk_concentric(aperture_sample);
        Point3f aperture_p(tmp.x(), tmp.y(), 0.f);

        // Sampled position on the focal plane
        Float f_dist = m_focus_distance / near_p.z();
        Point3f focus_p   = near_p          * f_dist,
               focus_p_x = (near_p + m_dx) * f_dist,
               focus_p_y = (near_p + m_dy) * f_dist;

        // Convert into a normalized ray direction; adjust the ray interval accordingly.
        Vector3f d = normalize(Vector3f(focus_p - aperture_p));
        Float inv_z = rcp(d.z());
        ray.mint = m_near_clip * inv_z;
        ray.maxt = m_far_clip * inv_z;

        auto trafo = m_world_transform->eval(ray.time, active);
        ray.o = trafo.transform_affine(aperture_p);
        ray.d = trafo * d;
        ray.update();

        ray.o_x = ray.o_y = ray.o;

        ray.d_x = trafo * normalize(Vector3f(focus_p_x - aperture_p));
        ray.d_y = trafo * normalize(Vector3f(focus_p_y - aperture_p));
        ray.has_differentials = true;

        return std::make_pair(ray, wav_weight);
    }

    ScalarBoundingBox3f bbox() const override {
        return m_world_transform->translation_bounds();
    }

    //! @}
    // =============================================================

    std::string to_string() const override {
        using string::indent;

        std::ostringstream oss;
        oss << "ThinLensCamera[" << std::endl
            << "  x_fov = " << m_x_fov << "," << std::endl
            << "  near_clip = " << m_near_clip << "," << std::endl
            << "  far_clip = " << m_far_clip << "," << std::endl
            << "  focus_distance = " << m_focus_distance << "," << std::endl
            << "  film = " << indent(m_film->to_string()) << "," << std::endl
            << "  sampler = " << indent(m_sampler->to_string()) << "," << std::endl
            << "  resolution = " << m_resolution << "," << std::endl
            << "  shutter_open = " << m_shutter_open << "," << std::endl
            << "  shutter_open_time = " << m_shutter_open_time << "," << std::endl
            << "  aspect = " << m_aspect << "," << std::endl
            << "  world_transform = " << indent(m_world_transform)  << std::endl
            << "]";
        return oss.str();
    }

private:
    ScalarTransform4f m_camera_to_sample;
    ScalarTransform4f m_sample_to_camera;
    ScalarBoundingBox2f m_image_rect;
    ScalarFloat m_aperture_radius;
    ScalarFloat m_normalization;
    ScalarFloat m_x_fov;
    ScalarVector3f m_dx, m_dy;
};

MTS_EXPORT_PLUGIN(ThinLensCamera, "Thin Lens Camera");
NAMESPACE_END(mitsuba)
