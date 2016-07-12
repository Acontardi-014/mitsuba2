#pragma once

#include <mitsuba/core/object.h>

NAMESPACE_BEGIN(mitsuba)

class MTS_EXPORT_RENDER Shape : public Object {
public:
    /// Use 32 bit indices to keep track of indices to conserve memory
    using Index = uint32_t;

    /// Use 32 bit indices to keep track of sizes to conserve memory
    using Size  = uint32_t;

    virtual void doSomething() = 0;

    /**
     * \brief Return an axis aligned box that bounds all shape primitives
     * (including any transformations that may have been applied to them)
     */
    virtual BoundingBox3f bbox() const = 0;

    /**
     * \brief Return an axis aligned box that bounds a single shape primitives
     * (including any transformations that may have been applied to it)
     *
     * \remark The default implementation simply calls \ref bbox()
     */
    virtual BoundingBox3f bbox(Index Size) const;

    /**
     * \brief Returns the number of sub-primitives that make up this shape
     * \remark The default implementation simply returns \c 1
     */
    virtual Size primitiveCount() const;

    MTS_DECLARE_CLASS()

protected:
    virtual ~Shape();
};

NAMESPACE_END(mitsuba)
