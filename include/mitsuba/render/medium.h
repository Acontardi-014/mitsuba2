#pragma once

#include <mitsuba/core/object.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/core/traits.h>
#include <mitsuba/render/fwd.h>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class MTS_EXPORT_RENDER Medium : public Object {
public:
    MTS_DECLARE_CLASS_VARIANT(Medium, Object, "medium")

protected:
    virtual ~Medium();

};

NAMESPACE_END(mitsuba)

// -----------------------------------------------------------------------
//! @{ \name Enoki support for packets of Medium pointers
// -----------------------------------------------------------------------

// Enable usage of array pointers for our types
ENOKI_CALL_SUPPORT_TEMPLATE_BEGIN(mitsuba::Medium)
ENOKI_CALL_SUPPORT_TEMPLATE_END(mitsuba::Medium)

//! @}
// -----------------------------------------------------------------------
