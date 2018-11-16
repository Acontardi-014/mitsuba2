#pragma once

#include <mitsuba/render/fwd.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/core/frame.h>
#include <mitsuba/core/ray.h>

NAMESPACE_BEGIN(mitsuba)

/** \brief Generic surface interaction data structure
 */
template <typename Point3_> struct Interaction {
    // =============================================================
    //! @{ \name Type declarations
    // =============================================================

    using Point3   = Point3_;
    using Vector3  = vector3_t<Point3>;
    using Value    = value_t<Point3>;
    using Spectrum = mitsuba::Spectrum<Value>;
    using Mask     = mask_t<Value>;
    using Ray3     = Ray<Point3>;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Fields
    // =============================================================

    /// Distance traveled along the ray
    Value t = math::Infinity;

    /// Time value associated with the interaction
    Value time;

    /// Wavelengths associated with the ray that produced this interaction
    Spectrum wavelengths;

    /// Position of the interaction in world coordinates
    Point3 p;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Methods
    // =============================================================

    /// Is the current interaction valid?
    Mask is_valid() const {
        return neq(t, math::Infinity);
    }

    /// Spawn a semi-infinite ray towards the given direction
    Ray3 spawn_ray(const Vector3 &d) {
        return Ray3(p, d, (1.f + hmax(abs(p))) * math::Epsilon,
                    math::Infinity, time, wavelengths);
    }

    /// Spawn a finite ray towards the given position
    Ray3 spawn_ray_to(const Point3 &t) {
        Vector3 d = t - p;
        Value dist = norm(d);
        d /= dist;

        return Ray3(p, d, (1.f + hmax(abs(p))) * math::Epsilon,
                    dist * (1.f - math::ShadowEpsilon), time, wavelengths);
    }

    //! @}
    // =============================================================

    ENOKI_STRUCT(Interaction, t, time, wavelengths, p);
};

/** \brief Container for all information related to a scattering
 * event on a surface
 */
template <typename Point3_> struct SurfaceInteraction : Interaction<Point3_> {

    // =============================================================
    //! @{ \name Type declarations
    // =============================================================
    //
    using Base = Interaction<Point3_>;
    using typename Base::Point3;
    using typename Base::Vector3;
    using typename Base::Value;
    using typename Base::Mask;
    using typename Base::Spectrum;

    using Index               = uint32_array_t<Value>;

    using Point2              = point2_t<Point3>;
    using Vector2             = vector2_t<Point3>;
    using Normal3             = normal3_t<Point3>;

    using Frame3              = Frame<Vector3>;
    using Color3              = Color<Value, 3>;
    using RayDifferential3    = RayDifferential<Point3>;

    using BSDFPtr             = replace_scalar_t<Value, const BSDF *>;
    using MediumPtr           = replace_scalar_t<Value, const Medium *>;
    using ShapePtr            = replace_scalar_t<Value, const Shape *>;
    using EmitterPtr          = replace_scalar_t<Value, const Emitter *>;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Fields
    // =============================================================

    using Base::t;
    using Base::time;
    using Base::wavelengths;
    using Base::p;

    /// Pointer to the associated shape
    ShapePtr shape = nullptr;

    /// UV surface coordinates
    Point2 uv;

    /// Geometric normal
    Normal3 n;

    /// Shading frame
    Frame3 sh_frame;

    /// Position partials wrt. the UV parameterization
    Vector3 dp_du, dp_dv;

    /// UV partials wrt. changes in screen-space
    Vector2 duv_dx, duv_dy;

    /// Incident direction in the local shading frame
    Vector3 wi;

    /// Primitive index, e.g. the triangle ID (if applicable)
    Index prim_index;

    /// Stores a pointer to the parent instance (if applicable)
    ShapePtr instance = nullptr;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Methods
    // =============================================================

    /**
     * Construct from a position sample.
     * Unavailable fields such as `wi` and the partial derivatives are left
     * uninitialized.
     * The `shape` pointer is left uninitialized because we can't guarantee that
     * the given \ref PositionSample::object points to a Shape instance.
     */
    explicit SurfaceInteraction(const PositionSample<Point3> &ps,
                                const Spectrum &wavelengths)
        : Base(0.f, ps.time, wavelengths, ps.p), uv(ps.uv), n(ps.n),
          sh_frame(Frame3(ps.n)) { }

    using Base::is_valid;

    /// Convert a local shading-space vector into world space
    Vector3 to_world(const Vector3 &v) const {
        return sh_frame.to_world(v);
    }

    /// Convert a world-space vector into local shading coordinates
    Vector3 to_local(const Vector3 &v) const {
        return sh_frame.to_local(v);
    }

    /// Return the emitter associated with the intersection (if any)
    template <typename Scene = mitsuba::Scene>
    EmitterPtr emitter(const Scene *scene) const {
        if (enoki::is_array<Value>::value)
            return select(is_valid(), shape->emitter(), scene->environment());
        else
            return all(is_valid()) ? shape->emitter() : scene->environment();
    }

    /// Is the intersected shape also a sensor?
    Mask is_sensor() const { return shape->is_sensor(); }

    /// Does the surface mark a transition between two media?
    Mask is_medium_transition() const { return shape->is_medium_transition(); }

    /**
     * \brief Determine the target medium
     *
     * When \c is_medium_transition() = \c true, determine the medium that
     * contains the ray (\c this->p, \c d)
     */
    MediumPtr target_medium(const Vector3 &d) const {
        return target_medium(dot(d, n));
    }

    /**
     * \brief Determine the target medium based on the cosine
     * of the angle between the geometric normal and a direction
     *
     * Returns the exterior medium when \c cos_theta > 0 and
     * the interior medium when \c cos_theta <= 0.
     */
    MediumPtr target_medium(const Value &cos_theta) const {
        return select(cos_theta > 0, shape->exterior_medium(),
                                     shape->interior_medium());
    }

    /**
     * \brief Returns the BSDF of the intersected shape.
     *
     * The parameter \c ray must match the one used to create the interaction
     * record. This function computes texture coordinate partials if this is
     * required by the BSDF (e.g. for texture filtering).
     *
     * Implementation in 'bsdf.h'
     */
    BSDFPtr bsdf(const RayDifferential3 &ray);

    // Returns the BSDF of the intersected shape
    BSDFPtr bsdf() const { return shape->bsdf(); }

    /// Calls the suitable implementation of \ref Shape::normal_derivative()
    std::pair<Vector3, Vector3> normal_derivative(
            bool shading_frame = true, mask_t<Value> active = true) const {
        ShapePtr target = select(neq(instance, nullptr), instance, shape);
        return target->normal_derivative(*this, shading_frame, active);
    }

    /// Computes texture coordinate partials
    void compute_partials(const RayDifferential3 &ray) {
        if (!ray.has_differentials)
            return;

        /* Compute interaction with the two offset rays */
        auto d   = dot(n, p),
             t_x = (d - dot(n, ray.o_x)) / dot(n, ray.d_x),
             t_y = (d - dot(n, ray.o_y)) / dot(n, ray.d_y);

        /* Corresponding positions near the surface */
        auto dp_dx = fmadd(ray.d_x, t_x, ray.o_x) - p,
             dp_dy = fmadd(ray.d_y, t_y, ray.o_y) - p;

        /* Solve a least squares problem to turn this into UV coordinates */
        auto a00 = dot(dp_du, dp_du),
             a01 = dot(dp_du, dp_dv),
             a11 = dot(dp_dv, dp_dv),
             inv_det = rcp(a00*a11 - a01*a01);

        auto b0x = dot(dp_du, dp_dx),
             b1x = dot(dp_dv, dp_dx),
             b0y = dot(dp_du, dp_dy),
             b1y = dot(dp_dv, dp_dy);

        /* Set the UV partials to zero if dpdu and/or dpdv == 0 */
        inv_det = select(enoki::isfinite(inv_det), inv_det, 0.f);

        duv_dx = Vector2(fmsub(a11, b0x, a01 * b1x) * inv_det,
                         fmsub(a00, b1x, a01 * b0x) * inv_det);

        duv_dy = Vector2(fmsub(a11, b0y, a01 * b1y) * inv_det,
                         fmsub(a00, b1y, a01 * b0y) * inv_det);
    }

    //! @}
    // =============================================================

    bool has_uv_partials() const {
        return any_nested(neq(duv_dx, 0.f) || neq(duv_dy, 0.f));
    }

    ENOKI_DERIVED_STRUCT(SurfaceInteraction, Base,
        ENOKI_BASE_FIELDS(t, time, wavelengths, p),
        ENOKI_DERIVED_FIELDS(shape, uv, n, sh_frame, dp_du, dp_dv,
                             duv_dx, duv_dy, wi, prim_index, instance)
    )
};

// -----------------------------------------------------------------------------

template <typename Point3>
std::ostream &operator<<(std::ostream &os, const Interaction<Point3> &it) {
    if (none(it.is_valid())) {
        os << "Interaction[invalid]";
    } else {
        os << "Interaction[" << std::endl
           << "  t = " << it.t << "," << std::endl
           << "  time = " << it.time << "," << std::endl
           << "  wavelengths = " << it.wavelengths << "," << std::endl
           << "  p = " << string::indent(it.p, 6) << std::endl
           << "]";
    }
    return os;
}

template <typename Point3>
std::ostream &operator<<(std::ostream &os, const SurfaceInteraction<Point3> &it) {
    if (none(it.is_valid())) {
        os << "SurfaceInteraction[invalid]";
    } else {
        os << "SurfaceInteraction[" << std::endl
           << "  t = " << it.t << "," << std::endl
           << "  time = " << it.time << "," << std::endl
           << "  wavelengths = " << it.wavelengths << "," << std::endl
           << "  p = " << string::indent(it.p, 6) << "," << std::endl
           << "  shape = " << string::indent(it.shape, 2) << "," << std::endl
           << "  uv = " << string::indent(it.uv, 7) << "," << std::endl
           << "  n = " << string::indent(it.n, 6) << "," << std::endl
           << "  sh_frame = " << string::indent(it.sh_frame, 2) << "," << std::endl
           << "  dp_du = " << string::indent(it.dp_du, 10) << "," << std::endl
           << "  dp_dv = " << string::indent(it.dp_dv, 10) << "," << std::endl;

        if (it.has_uv_partials())
            os << "  duv_dx = " << string::indent(it.duv_dx, 11) << "," << std::endl
               << "  duv_dy = " << string::indent(it.duv_dy, 11) << "," << std::endl;

        os << "  wi = " << string::indent(it.wi, 7) << "," << std::endl
           << "  prim_index = " << it.prim_index << "," << std::endl
           << "  instance = " << string::indent(it.instance, 13) << std::endl
           << "]";
    }
    return os;
}

NAMESPACE_END(mitsuba)

// -----------------------------------------------------------------------
//! @{ \name Enoki accessors for dynamic vectorization
// -----------------------------------------------------------------------

ENOKI_STRUCT_SUPPORT(mitsuba::Interaction, t, time, wavelengths, p);

ENOKI_STRUCT_SUPPORT(mitsuba::SurfaceInteraction, t, time, wavelengths, p,
                     shape, uv, n, sh_frame, dp_du, dp_dv, duv_dx, duv_dy, wi,
                     prim_index, instance)

//! @}
// -----------------------------------------------------------------------
