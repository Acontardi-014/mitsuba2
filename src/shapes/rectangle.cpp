#include <mitsuba/core/fwd.h>
#include <mitsuba/core/math.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/string.h>
#include <mitsuba/core/transform.h>
#include <mitsuba/core/util.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/fwd.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/render/shape.h>

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Flat, rectangular shape, e.g. for use as a ground plane or area emitter.
 *
 * By default, the rectangle covers the XY-range [-1, 1]^2 and has a surface
 * normal that points into the positive $Z$ direction. To change the rectangle
 * scale, rotation, or translation, use the 'to_world' parameter.
 */
template <typename Float, typename Spectrum>
class Rectangle final : public Shape<Float, Spectrum> {
public:
    MTS_REGISTER_CLASS(Rectangle, Shape)
    MTS_USING_BASE(Shape, bsdf, emitter, is_emitter)
    MTS_IMPORT_TYPES()
    using Index = typename Base::Index;
    using Size  = typename Base::Size;

    Rectangle(const Properties &props) : Base(props) {
        m_object_to_world = props.transform("to_world", Transform4f());
        if (props.bool_("flip_normals", false))
            m_object_to_world = m_object_to_world * Transform4f::scale(Vector3f(1.f, 1.f, -1.f));

        m_world_to_object = m_object_to_world.inverse();

        m_dp_du = m_object_to_world * Vector3f(2.f, 0.f, 0.f);
        m_dp_dv = m_object_to_world * Vector3f(0.f, 2.f, 0.f);
        Normal3f normal = normalize(m_object_to_world * Normal3f(0.f, 0.f, 1.f));
        m_frame = Frame3f(normalize(m_dp_du), normalize(m_dp_dv), normal);

        m_inv_surface_area = rcp(surface_area());
        if (abs(dot(normalize(m_dp_du), normalize(m_dp_dv))) > math::Epsilon<Float>)
            Throw("The `to_world` transformation contains shear, which is not"
                  " supported by the Rectangle shape.");

        if (is_emitter())
            emitter()->set_shape(this);
    }

    BoundingBox3f bbox() const override {
        BoundingBox3f bbox;
        bbox.expand(m_object_to_world.transform_affine(Point3f(-1.f, -1.f, 0.f)));
        bbox.expand(m_object_to_world.transform_affine(Point3f( 1.f, -1.f, 0.f)));
        bbox.expand(m_object_to_world.transform_affine(Point3f( 1.f,  1.f, 0.f)));
        bbox.expand(m_object_to_world.transform_affine(Point3f(-1.f,  1.f, 0.f)));
        return bbox;
    }

    Float surface_area() const override {
        return norm(m_dp_du) * norm(m_dp_dv);
    }

    // =============================================================
    //! @{ \name Sampling routines
    // =============================================================

    PositionSample3f sample_position(Float time, const Point2f &sample,
                                     Mask /*active*/) const override {
        PositionSample3f ps;
        ps.p = m_object_to_world.transform_affine(
            Point3f(sample.x() * 2.f - 1.f, sample.y() * 2.f - 1.f, 0.f));
        ps.n    = m_frame.n;
        ps.pdf  = m_inv_surface_area;
        ps.uv   = sample;
        ps.time = time;

        return ps;
    }

    Float pdf_position(const PositionSample3f & /*ps*/, Mask /*active*/) const override {
        return m_inv_surface_area;
    }

    DirectionSample3f sample_direction(const Interaction3f &it, const Point2f &sample,
                                       Mask active) const override {
        DirectionSample3f ds = sample_position(it.time, sample, active);
        ds.d = ds.p - it.p;

        Float dist_squared = squared_norm(ds.d);
        ds.dist  = sqrt(dist_squared);
        ds.d    /= ds.dist;

        Float dp = abs_dot(ds.d, ds.n);
        ds.pdf *= select(neq(dp, 0.f), dist_squared / dp, Float(0.f));

        return ds;
    }

    Float pdf_direction(const Interaction3f & /*it*/, const DirectionSample3f &ds,
                        Mask active) const override {
        Float pdf = pdf_position(ds, active),
              dp  = abs_dot(ds.d, ds.n);

        return pdf * select(neq(dp, 0.f), (ds.dist * ds.dist) / dp, 0.f);
    }

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Ray tracing routines
    // =============================================================

    std::pair<Mask, Float> ray_intersect(const Ray3f &ray_, Float *cache,
                                         Mask active) const override {
        Ray3f ray     = m_world_to_object.transform_affine(ray_);
        Float t       = -ray.o.z() * ray.d_rcp.z();
        Point3f local = ray(t);

        // Is intersection within ray segment and rectangle?
        active = active && t >= ray.mint
                        && t <= ray.maxt
                        && abs(local.x()) <= 1.f
                        && abs(local.y()) <= 1.f;

        t = select(active, t, Float(math::Infinity<Float>));

        if (cache) {
            masked(cache[0], active) = local.x();
            masked(cache[1], active) = local.y();
        }

        return { active, t };
    }

    Mask ray_test(const Ray3f &ray_, Mask active) const override {
        Ray3f ray     = m_world_to_object.transform_affine(ray_);
        Float t       = -ray.o.z() * ray.d_rcp.z();
        Point3f local = ray(t);

        // Is intersection within ray segment and rectangle?
        return active && t >= ray.mint
                      && t <= ray.maxt
                      && abs(local.x()) <= 1.f
                      && abs(local.y()) <= 1.f;
    }

    void fill_surface_interaction(const Ray3f &ray, const Float *cache,
                                  SurfaceInteraction3f &si_out, Mask active) const override {
        SurfaceInteraction3f si(si_out);

        si.n          = m_frame.n;
        si.sh_frame.n = m_frame.n;
        si.dp_du      = m_dp_du;
        si.dp_dv      = m_dp_dv;
        si.p          = ray(si.t);
        si.time       = ray.time;
        si.uv         = Point2f(fmadd(cache[0], .5f, .5f),
                                fmadd(cache[1], .5f, .5f));

        si_out[active] = si;
    }

    std::pair<Vector3f, Vector3f> normal_derivative(const SurfaceInteraction3f & /*si*/,
                                                    bool /*shading_frame*/,
                                                    Mask /*active*/) const override {
        return { Vector3f(0.f), Vector3f(0.f) };
    }

    Size primitive_count() const override { return 1; }

    Size effective_primitive_count() const override { return 1; }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Rectangle[" << std::endl
            << "  object_to_world = " << string::indent(m_object_to_world) << "," << std::endl
            << "  frame = " << string::indent(m_frame) << "," << std::endl
            << "  inv_surface_area = " << m_inv_surface_area << "," << std::endl
            << "  bsdf = " << string::indent(bsdf()->to_string()) << std::endl
            << "]";
        return oss.str();
    }

private:
    Transform4f m_object_to_world;
    Transform4f m_world_to_object;
    Frame3f m_frame;
    Vector3f m_dp_du, m_dp_dv;
    ScalarFloat m_inv_surface_area;
};

MTS_IMPLEMENT_PLUGIN(Rectangle, Shape, "Rectangle intersection primitive");
NAMESPACE_END(mitsuba)
