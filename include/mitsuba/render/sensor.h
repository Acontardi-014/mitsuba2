#pragma once

#include <mitsuba/core/spectrum.h>
#include <mitsuba/core/transform.h>
#include <mitsuba/render/endpoint.h>
#include <mitsuba/core/vector.h>
#include <mitsuba/render/film.h>
#include <mitsuba/render/fwd.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/render/records.h>
#include <mitsuba/render/sampler.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class MTS_EXPORT_RENDER Sensor : public Endpoint<Float, Spectrum> {
public:
    MTS_REGISTER_INTERFACE(Sensor, Endpoint)
    MTS_USING_BASE(Endpoint, sample_ray)
    MTS_IMPORT_TYPES();
    using Base::m_needs_sample_3;
    using Film    = typename Aliases::Film;
    using Sampler = typename Aliases::Sampler;

    // =============================================================
    //! @{ \name Sensor-specific sampling functions
    // =============================================================

    /**
     * \brief Importance sample a ray differential proportional to the sensor's
     * sensitivity profile.
     *
     * The sensor profile is a six-dimensional quantity that depends on time,
     * wavelength, surface position, and direction. This function takes a given
     * time value and five uniformly distributed samples on the interval [0, 1]
     * and warps them so that the returned ray the profile. Any
     * discrepancies between ideal and actual sampled profile are absorbed into
     * a spectral importance weight that is returned along with the ray.
     *
     * In contrast to \ref Endpoint::sample_ray(), this function returns
     * differentials with respect to the X and Y axis in screen space.
     *
     * \param time
     *    The scene time associated with the ray_differential to be sampled
     *
     * \param sample1
     *     A uniformly distributed 1D value that is used to sample the spectral
     *     dimension of the sensitivity profile.
     *
     * \param sample2
     *    This argument corresponds to the sample position in fractional pixel
     *    coordinates relative to the crop window of the underlying film.
     *
     * \param sample3
     *    A uniformly distributed sample on the domain <tt>[0,1]^2</tt>. This
     *    argument determines the position on the aperture of the sensor. This
     *    argument is ignored if <tt>needs_sample_3() == false</tt>.
     *
     * \return
     *    The sampled ray differential and (potentially spectrally varying)
     *    importance weights. The latter account for the difference between the
     *    sensor profile and the actual used sampling density function.
     */
    virtual std::pair<RayDifferential3f, Spectrum>
    sample_ray_differential(Float time,
                            Float sample1, const Point2f &sample2, const Point2f &sample3,
                            Mask active = true) const;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Additional query functions
    // =============================================================

    /// Return the time value of the shutter opening event
    Float shutter_open() const { return m_shutter_open; }

    /// Return the length, for which the shutter remains open
    Float shutter_open_time() const { return m_shutter_open_time; }

    /// Does the sampling technique require a sample for the aperture position?
    bool needs_aperture_sample() const { return m_needs_sample_3; }

    /// Updates the film's crop window, and adjusts any state accordingly.
    virtual void set_crop_window(const Vector2i &crop_size, const Point2i &crop_offset);

    /// Return the \ref Film instance associated with this sensor
    Film *film() { return m_film; }

    /// Return the \ref Film instance associated with this sensor (const)
    const Film *film() const { return m_film.get(); }

    /**
     * \brief Return the sensor's sample generator
     *
     * This is the \a root sampler, which will later be cloned a
     * number of times to provide each participating worker thread
     * with its own instance (see \ref Scene::sampler()).
     * Therefore, this sampler should never be used for anything
     * except creating clones.
     */
    Sampler *sampler() { return m_sampler; }

    /**
     * \brief Return the sensor's sampler (const version).
     *
     * This is the \a root sampler, which will later be cloned a
     * number of times to provide each participating worker thread
     * with its own instance (see \ref Scene::sampler()).
     * Therefore, this sampler should never be used for anything
     * except creating clones.
     */
    const Sampler *sampler() const { return m_sampler.get(); }

    //! @}
    // =============================================================

protected:
    Sensor(const Properties &props);

    virtual ~Sensor();

protected:
    ref<Film> m_film;
    ref<Sampler> m_sampler;
    Vector2f m_resolution;
    Float m_shutter_open;
    Float m_shutter_open_time;
    Float m_aspect;
};


/**
 * \brief Projective camera interface
 *
 * This class provides an abstract interface to several types of sensors that
 * are commonly used in computer graphics, such as perspective and orthographic
 * camera models.
 *
 * The interface is meant to be implemented by any kind of sensor, whose
 * world to clip space transformation can be explained using only linear
 * operations on homogeneous coordinates.
 *
 * A useful feature of \ref ProjectiveCamera sensors is that their view can be
 * rendered using the traditional OpenGL pipeline.
 *
 * \ingroup librender
 */
template <typename Float, typename Spectrum>
class MTS_EXPORT_RENDER ProjectiveCamera : public Sensor<Float, Spectrum> {
public:
    MTS_REGISTER_CLASS(ProjectiveCamera, Sensor)
    MTS_IMPORT_TYPES();
    using Base = Sensor<Float, Spectrum>;
    using Base::world_transform;

    /// Return the near clip plane distance
    Float near_clip() const { return m_near_clip; }

    /// Return the far clip plane distance
    Float far_clip() const { return m_far_clip; }

    /// Return the distance to the focal plane
    Float focus_distance() const { return m_focus_distance; }

protected:
    ProjectiveCamera(const Properties &props);

    virtual ~ProjectiveCamera();

protected:
    Float m_near_clip;
    Float m_far_clip;
    Float m_focus_distance;
};

NAMESPACE_END(mitsuba)
