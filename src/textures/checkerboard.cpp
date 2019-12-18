#include <mitsuba/render/texture.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/transform.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class Checkerboard final : public Texture<Float, Spectrum> {
public:
    MTS_IMPORT_TYPES(Texture)

    Checkerboard(const Properties &props) {
        m_color0 = props.texture<Texture>("color0", .4f);
        m_color1 = props.texture<Texture>("color1", .2f);
        m_transform = props.transform("to_uv", ScalarTransform4f()).extract();
    }

    UnpolarizedSpectrum eval(const SurfaceInteraction3f &it, Mask active) const override {
        Point2f uv = m_transform.transform_affine(it.uv);
        mask_t<Point2f> mask = uv - floor(uv) > .5f;
        UnpolarizedSpectrum result = zero<UnpolarizedSpectrum>();

        Mask m0 = eq(mask.x(), mask.y()),
             m1 = !m0;

        m0 &= active; m1 &= active;

        if (any_or<true>(m0))
            result[m0] = m_color0->eval(it, m0);

        if (any_or<true>(m1))
            result[m1] = m_color1->eval(it, m1);

        return result;
    }

    Float eval_1(const SurfaceInteraction3f &it, Mask active) const override {
        Point2f uv = m_transform.transform_affine(it.uv);
        mask_t<Point2f> mask = (uv - floor(uv)) > .5f;
        Float result = 0.f;

        Mask m0 = neq(mask.x(), mask.y()),
             m1 = !m0;

        m0 &= active; m1 &= active;

        if (any_or<true>(m0))
            masked(result, m0) = m_color0->eval_1(it, m0);

        if (any_or<true>(m1))
            masked(result, m1) = m_color1->eval_1(it, m1);

        return result;
    }

    ScalarFloat mean() const override {
        return .5f * (m_color0->mean() + m_color1->mean());
    }

    void traverse(TraversalCallback *callback) override {
        callback->put_parameter("transform", m_transform);
        callback->put_object("color0", m_color0.get());
        callback->put_object("color1", m_color1.get());
    }

    MTS_DECLARE_CLASS()
protected:
    ref<Texture> m_color0;
    ref<Texture> m_color1;
    ScalarTransform3f m_transform;
};

MTS_IMPLEMENT_CLASS_VARIANT(Checkerboard, Texture)
MTS_EXPORT_PLUGIN(Checkerboard, "Checkerboard texture")
NAMESPACE_END(mitsuba)
