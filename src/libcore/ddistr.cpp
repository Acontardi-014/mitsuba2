#include <mitsuba/core/ddistr.h>
#include <mitsuba/core/string.h>

NAMESPACE_BEGIN(mitsuba)

#if 0

template MTS_EXPORT_CORE uint32_t DiscreteDistribution::sample(Float,  mask_t<Float>) const;
template MTS_EXPORT_CORE UInt32P  DiscreteDistribution::sample(FloatP, mask_t<FloatP>) const;

template MTS_EXPORT_CORE std::pair<uint32_t, Float>  DiscreteDistribution::sample_pdf(Float,  mask_t<Float>) const;
template MTS_EXPORT_CORE std::pair<UInt32P,  FloatP> DiscreteDistribution::sample_pdf(FloatP, mask_t<FloatP>) const;

template MTS_EXPORT_CORE std::pair<uint32_t, Float> DiscreteDistribution::sample_reuse(Float,  mask_t<Float>) const;
template MTS_EXPORT_CORE std::pair<UInt32P, FloatP> DiscreteDistribution::sample_reuse(FloatP, mask_t<FloatP>) const;

template MTS_EXPORT_CORE std::tuple<uint32_t, Float, Float>   DiscreteDistribution::sample_reuse_pdf(Float,  mask_t<Float>) const;
template MTS_EXPORT_CORE std::tuple<UInt32P,  FloatP, FloatP> DiscreteDistribution::sample_reuse_pdf(FloatP, mask_t<FloatP>) const;

#endif

NAMESPACE_END(mitsuba)
