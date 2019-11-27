#include <mitsuba/core/string.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/render/bsdf.h>
#include <enoki/stl.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class TwoSidedBRDF final : public BSDF<Float, Spectrum> {
public:
    MTS_DECLARE_CLASS_VARIANT(TwoSidedBRDF, BSDF)
    MTS_IMPORT_BASE(BSDF, m_flags, m_components)
    MTS_IMPORT_TYPES(BSDF)

    TwoSidedBRDF(const Properties &props) : Base(props) {
        auto bsdfs = props.objects();
        if (bsdfs.size() > 0)
            m_nested_brdf[0] = dynamic_cast<BSDF *>(bsdfs[0].second.get());
        if (bsdfs.size() == 2)
            m_nested_brdf[1] = dynamic_cast<BSDF *>(bsdfs[1].second.get());
        else if (bsdfs.size() > 2)
            Throw("At most two nested BSDFs can be specified!");

        if (!m_nested_brdf[0])
            Throw("A nested one-sided material is required!");
        if (!m_nested_brdf[1])
            m_nested_brdf[1] = m_nested_brdf[0];

        parameters_changed();
    }

    void parameters_changed() override {
        // Add all nested components, overwriting any front / back side flag.
        m_flags = BSDFFlags(0);
        m_components.clear();
        for (size_t i = 0; i < m_nested_brdf[0]->component_count(); ++i) {
            auto c = (m_nested_brdf[0]->flags(i) & ~BSDFFlags::BackSide);
            m_components.push_back(c | BSDFFlags::FrontSide);
            m_flags = m_flags | m_components.back();
        }
        for (size_t i = 0; i < m_nested_brdf[1]->component_count(); ++i) {
            auto c = (m_nested_brdf[1]->flags(i) & ~BSDFFlags::FrontSide);
            m_components.push_back(c | BSDFFlags::BackSide);
            m_flags = m_flags | m_components.back();
        }

        if (has_flag(m_flags, BSDFFlags::Transmission))
            Throw("Only materials without a transmission component can be nested!");
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx_,
                                             const SurfaceInteraction3f &si_, Float sample1,
                                             const Point2f &sample2, Mask active) const override {
        using Result = std::pair<BSDFSample3f, Spectrum>;

        SurfaceInteraction3f si(si_);
        BSDFContext ctx(ctx_);

        Mask front_side = Frame3f::cos_theta(si.wi) > 0.f && active,
             back_side  = Frame3f::cos_theta(si.wi) < 0.f && active;

        Result result = zero<Result>();
        if (any_or<true>(front_side))
            masked(result, front_side) =
                m_nested_brdf[0]->sample(ctx, si, sample1, sample2, front_side);

        if (any_or<true>(back_side)) {
            if (ctx.component != (uint32_t) -1)
                ctx.component -= (uint32_t) m_nested_brdf[0]->component_count();

            si.wi.z() *= -1.f;
            masked(result, back_side) =
                m_nested_brdf[1]->sample(ctx, si, sample1, sample2, back_side);
            masked(result.first.wo.z(), back_side) *= -1.f;
        }

        return result;
    }

    Spectrum eval(const BSDFContext &ctx_, const SurfaceInteraction3f &si_, const Vector3f &wo_,
                  Mask active) const override {
        SurfaceInteraction3f si(si_);
        BSDFContext ctx(ctx_);
        Vector3f wo(wo_);
        Spectrum result = 0.f;

        Mask front_side = Frame3f::cos_theta(si.wi) > 0.f && active,
             back_side  = Frame3f::cos_theta(si.wi) < 0.f && active;

        if (any_or<true>(front_side))
            result = m_nested_brdf[0]->eval(ctx, si, wo, front_side);

        if (any_or<true>(back_side)) {
            if (ctx.component != (uint32_t) -1)
                ctx.component -= (uint32_t) m_nested_brdf[0]->component_count();

            si.wi.z() *= -1.f;
            wo.z() *= -1.f;

            masked(result, back_side) = m_nested_brdf[1]->eval(ctx, si, wo, back_side);
        }

        return result;
    }

    Float pdf(const BSDFContext &ctx_, const SurfaceInteraction3f &si_, const Vector3f &wo_,
              Mask active) const override {
        SurfaceInteraction3f si(si_);
        BSDFContext ctx(ctx_);
        Vector3f wo(wo_);
        Float result = 0.f;

        Mask front_side = Frame3f::cos_theta(si.wi) > 0.f && active,
             back_side  = Frame3f::cos_theta(si.wi) < 0.f && active;

        if (any_or<true>(front_side))
            result = m_nested_brdf[0]->pdf(ctx, si, wo, front_side);

        if (any_or<true>(back_side)) {
            if (ctx.component != (uint32_t) -1)
                ctx.component -= (uint32_t) m_nested_brdf[0]->component_count();

            si.wi.z() *= -1.f;
            wo.z() *= -1.f;

            masked(result, back_side) = m_nested_brdf[1]->pdf(ctx, si, wo, back_side);
        }

        return result;
    }

    void traverse(TraversalCallback *callback) override {
        callback->put_object("nested_brdf_0", m_nested_brdf[0].get());
        callback->put_object("nested_brdf_1", m_nested_brdf[1].get());
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "TwoSided[" << std::endl
            << "  nested_brdf[0] = " << string::indent(m_nested_brdf[0]->to_string()) << "," << std::endl
            << "  nested_brdf[1] = " << string::indent(m_nested_brdf[1]->to_string()) << std::endl
            << "]";
        return oss.str();
    }

protected:
    ref<BSDF> m_nested_brdf[2];
};

MTS_EXPORT_PLUGIN(TwoSidedBRDF, "Two-sided material adapter");
NAMESPACE_END(mitsuba)
