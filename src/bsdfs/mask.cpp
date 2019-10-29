#include <mitsuba/core/properties.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/core/string.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/spectrum.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class MaskBSDF final : public BSDF<Float, Spectrum> {
public:
    MTS_DECLARE_PLUGIN(MaskBSDF, BSDF)
    MTS_USING_BASE(BSDF, component_count, m_components, m_flags)
    using BSDF               = Base;
    using ContinuousSpectrum = typename Aliases::ContinuousSpectrum;

    MaskBSDF(const Properties &props) : Base(props) {
        for (auto &kv : props.objects()) {
            auto *bsdf = dynamic_cast<BSDF *>(kv.second.get());
            if (bsdf) {
                if (m_nested_bsdf)
                    Throw("Cannot specify more than one child BSDF");
                m_nested_bsdf = bsdf;
            }
        }
        if (!m_nested_bsdf)
           Throw("Child BSDF not specified");

        // Scalar-typed opacity texture
        m_opacity = props.spectrum<Float, Spectrum>("opacity", 0.5f);

        for (size_t i = 0; i < m_nested_bsdf->component_count(); ++i)
            m_components.push_back(m_nested_bsdf->flags(i));

        // The "transmission" BSDF component is at the last index.
        m_components.push_back(BSDFFlags::Null | BSDFFlags::FrontSide | BSDFFlags::BackSide);
        m_flags = m_nested_bsdf->flags() | m_components.back();
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                                             Float sample1, const Point2f &sample2,
                                             Mask active) const override {
        uint32_t null_index = (uint32_t) component_count() - 1;

        bool sample_transmission = ctx.is_enabled(BSDFFlags::Null, null_index);
        bool sample_nested       = ctx.component == (uint32_t) -1 || ctx.component < null_index;

        BSDFSample3f bs;
        Spectrum result(0.f);
        if (unlikely(!sample_transmission && !sample_nested))
            return { bs, result };

        Float opacity = eval_opacity(si, active);
        if (sample_transmission != sample_nested)
            opacity = sample_transmission ? 1.f : 0.f;

        bs.wo                = -si.wi;
        bs.eta               = 1.f;
        bs.sampled_component = null_index;
        bs.sampled_type      = +BSDFFlags::Null;
        bs.pdf               = 1.f - opacity;
        result               = 1.f;

        Mask nested_mask = active && sample1 < opacity;
        if (any_or<true>(nested_mask)) {
            sample1 /= opacity;
            auto tmp                = m_nested_bsdf->sample(ctx, si, sample1, sample2, nested_mask);
            masked(bs, nested_mask) = tmp.first;
            masked(result, nested_mask) = tmp.second;
        }

        return { bs, result };
    }

    Spectrum eval(const BSDFContext &ctx, const SurfaceInteraction3f &si, const Vector3f &wo,
                  Mask active) const override {
        Float opacity = eval_opacity(si, active);
        return m_nested_bsdf->eval(ctx, si, wo, active) * opacity;
    }

    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si, const Vector3f &wo,
              Mask active) const override {
        uint32_t null_index      = (uint32_t) component_count() - 1;
        bool sample_transmission = ctx.is_enabled(BSDFFlags::Null, null_index);
        bool sample_nested       = ctx.component == (uint32_t) -1 || ctx.component < null_index;

        if (!sample_nested)
            return 0.f;

        Float result = m_nested_bsdf->pdf(ctx, si, wo, active);
        if (sample_transmission)
            result *= eval_opacity(si, active);

        return result;
    }

    MTS_INLINE Float eval_opacity(const SurfaceInteraction3f &si, Mask active) const {
        return clamp(m_opacity->eval1(si, active), 0.f, 1.f);
    }

    std::vector<ref<Object>> children() override {
        return { m_opacity.get(), m_nested_bsdf.get() };
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Mask[" << std::endl
            << "  opacity = " << m_opacity << "," << std::endl
            << "  nested_bsdf = " << string::indent(m_nested_bsdf->to_string()) << std::endl
            << "]";
        return oss.str();
    }

protected:
    ref<ContinuousSpectrum> m_opacity;
    ref<BSDF> m_nested_bsdf;
};

MTS_IMPLEMENT_PLUGIN(MaskBSDF, BSDF, "Mask material")
NAMESPACE_END(mitsuba)
