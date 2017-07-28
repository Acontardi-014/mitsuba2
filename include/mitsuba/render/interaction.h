#pragma once

// TODO: move as many includes as possible to the .cpp
#include <mitsuba/render/common.h>
#include <mitsuba/core/fwd.h>
#include <mitsuba/core/frame.h>
#include <mitsuba/core/math.h>
#include <mitsuba/core/ray.h>
#include <mitsuba/core/spectrum.h>

NAMESPACE_BEGIN(mitsuba)

/** \brief Container for all information related to a surface intersection.
 */
template <typename Point3> struct SurfaceInteraction {
    using Value            = value_t<Point3>;
    using Index            = uint32_array_t<Value>;
    using Mask             = mask_t<Value>;

    using Point2           = point2_t<Point3>;
    using Vector2          = vector2_t<Point3>;
    using Vector3          = vector3_t<Point3>;
    using Normal3          = normal3_t<Point3>;

    using Frame3           = Frame<Vector3>;
    using Color3           = Color<Value, 3>;
    using RayDifferential3 = RayDifferential<Point3>;

    using BSDFPtr          = like_t<Value, const BSDF *>;
    using MediumPtr        = like_t<Value, const Medium *>;
    using ShapePtr         = like_t<Value, const Shape *>;

    // -------------------------------------------------------------------------

    /// Pointer to the associated shape
    ShapePtr shape = nullptr;

    /// Distance traveled along the ray
    Value t = std::numeric_limits<Float>::infinity();

    /// Time value associated with the intersection
    Value time;

    /* SurfaceInteraction point in 3D coordinates */
    Point3 p;

    /// Geometric normal
    Normal3f n;

    /// UV surface coordinates
    Point2f uv;

    /// Shading frame
    Frame3 sh_frame;

    /// Position partials wrt. the UV parameterization
    Vector3 dp_du, dp_dv;

    /// UV partials wrt. changes in screen-space
    Vector2 duv_dx, duv_dy;

    /// Interpolated vertex color
    Color3 color;

    /// Incident direction in the local shading frame
    Vector3 wi;

    /// Primitive index, e.g. the triangle ID (if applicable)
    Index prim_index;

    /// Stores a pointer to the parent instance (if applicable)
    ShapePtr instance;

    /// Have texture coordinate partials been computed?
    bool has_uv_partials;

    // -------------------------------------------------------------------------

    /// Convert a local shading-space vector into world space
    Vector3 to_world(const Vector3 &v) const {
        return sh_frame.to_world(v);
    }

    /// Convert a world-space vector into local shading coordinates
    Vector3 to_local(const Vector3 &v) const {
        return sh_frame.to_local(v);
    }

    /// Is the current intersection valid?
    Mask is_valid() const {
        return neq(t, std::numeric_limits<Float>::infinity());
    }

    /// Is the intersected shape also a emitter?
    Mask is_emitter() const {
        return shape->is_emitter();
    }

    /// Is the intersected shape also a sensor?
    Mask is_sensor() const {
        return shape->is_sensor();
    }

    /// Does the intersected shape have a subsurface integrator?
    Mask has_subsurface() const {
        return shape->has_subsurface();
    }

    /// Does the surface mark a transition between two media?
    Mask is_medium_transition() const {
        return shape->is_medium_transition();
    }

    /**
     * \brief Determine the target medium
     *
     * When \c is_medium_transition() = \c true, determine the medium that
     * contains the ray (\c this->p, \c d)
     */
    const MediumPtr target_medium(const Vector3 &d) const {
        return target_medium(dot(d, n));
    }

    /**
     * \brief Determine the target medium based on the cosine
     * of the angle between the geometric normal and a direction
     *
     * Returns the exterior medium when \c cos_theta > 0 and
     * the interior medium when \c cos_theta <= 0.
     */
    const MediumPtr target_medium(Value cos_theta) const {
        return select(cos_theta > 0,
                      shape->exterior_medium(),
                      shape->interior_medium());
    }

    /**
     * \brief Returns the BSDF of the intersected shape.
     *
     * The parameter \c ray must match the one used to create the intersection
     * record. This function computes texture coordinate partials if this is
     * required by the BSDF (e.g. for texture filtering).
     *
     * \remark This function should only be called if there is a valid
     * intersection!
     */
    const BSDFPtr bsdf(const RayDifferential3 &ray) {
        const BSDFPtr bsdf = shape->bsdf();

        if (!has_uv_partials && any(bsdf->uses_ray_differentials()))
            compute_partials(ray);

        return bsdf;
    }

    /// Returns the BSDF of the intersected shape
    const BSDFPtr bsdf() const {
        return shape->bsdf();
    }

#if 0
    /** TODO
     * \brief Returns radiance emitted into direction d.
     *
     * \remark This function should only be called if the
     * intersected shape is actually an emitter.
     */
    Spectrum Le(const Vector3 &/*d*/) const {
        Log(EError, "Not implemented yet");
        // return shape->emitter()->eval(*this, d);
        return Spectrum();
    }

    /**
     * \brief Returns radiance from a subsurface integrator
     * emitted into direction d.
     *
     * \remark Should only be called if the intersected
     * shape is actually a subsurface integrator.
     */
    // TODO: should we take vectorized samplers, or a single one that we use
    // in a vectorized way?
    Spectrum Lo_sub(const ShapePtr /*scene*/, Sampler */*sampler*/,
                           const Vector3 &/*d*/, int /*depth = 0*/) const {
        Log(EError, "Not implemented yet");
        // return shape->subsurface()->Lo(scene, sampler, *this, d, depth);
        return Spectrum();
    }

    /// Move the intersection forward or backward through time
    void adjust_time(Value /*time*/) {
        Log(EError, "Not implemented yet");
        // if (instance)
        //     instance->adjust_time(*this, time);
        // else if (shape)
        //     shape->adjust_time(*this, time);
        // else
        //     this->time = time;
    }

    /// Calls the suitable implementation of \ref Shape::normal_derivative()
    std::pair<Vector3, Vector3> normal_derivative(bool shading_frame = true) const {
        ShapePtr target = select(neq(instance, nullptr), instance, shape);
        return target->normal_derivative(*this);
    }
#endif

    /// Computes texture coordinate partials
    void compute_partials(const RayDifferential3 &ray) {
        if (has_uv_partials || !ray.has_differentials)
            return;

        /* Compute intersection with the two offset rays */
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

        duv_dx = Vector2(fmsub(a11, b0x, a01 * b1x) * inv_det, fmsub(a00, b1x, a01 * b0x) * inv_det);
        duv_dy = Vector2(fmsub(a11, b0y, a01 * b1y) * inv_det, fmsub(a00, b1y, a01 * b0y) * inv_det);

        has_uv_partials = true;
    }

    ENOKI_STRUCT(SurfaceInteraction, shape, t, time, p, n, uv, sh_frame,
                 dp_du, dp_dv, duv_dx, duv_dy, color, wi, prim_index, instance,
                 has_uv_partials)

    ENOKI_ALIGNED_OPERATOR_NEW()
};

// -----------------------------------------------------------------------------

template <typename Point3>
std::ostream &operator<<(std::ostream &os, const SurfaceInteraction<Point3> &it) {
    if (none(it.is_valid())) {
        os << "SurfaceInteraction[invalid]";
    } else {
        os << "SurfaceInteraction[" << std::endl
           << "  shape = " << it.shape << std::endl
           << "  t = " << it.t << "," << std::endl
           << "  time = " << it.time << "," << std::endl
           << "  p = " << it.p << "," << std::endl
           << "  n = " << it.n << "," << std::endl
           << "  uv = " << it.uv << "," << std::endl
           << "  sh_frame = " << it.sh_frame << "," << std::endl
           << "  dp_du = " << it.dp_du << "," << std::endl
           << "  dp_dv = " << it.dp_dv << "," << std::endl;

        if (it.has_uv_partials)
            os << "  duv_dx = " << it.duv_dx << "," << std::endl
               << "  duv_dy = " << it.duv_dy << "," << std::endl;

        os << "  color = " << it.color << "," << std::endl
           << "  wi = " << it.wi << "," << std::endl
           << "  prim_index = " << it.prim_index << "," << std::endl
           << "  instance = " << it.instance << "," << std::endl
           << "  has_uv_partials = " << it.has_uv_partials << "," << std::endl
           << "]";
    }
    return os;
}

NAMESPACE_END(mitsuba)

// -----------------------------------------------------------------------
//! @{ \name Enoki accessors for dynamic vectorization
// -----------------------------------------------------------------------

ENOKI_STRUCT_DYNAMIC(mitsuba::SurfaceInteraction, shape, t, time, p, n, uv,
                     sh_frame, dp_du, dp_dv, duv_dx, duv_dy, color,
                     wi, prim_index, instance, has_uv_partials)

//! @}
// -----------------------------------------------------------------------
