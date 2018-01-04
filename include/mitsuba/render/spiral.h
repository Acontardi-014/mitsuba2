#pragma once

#include <mutex>
#include <mitsuba/core/object.h>
#include <mitsuba/render/film.h>
#include <mitsuba/render/imageblock.h>
#include <tbb/tbb.h>

#if !defined(MTS_BLOCK_SIZE)
#  define MTS_BLOCK_SIZE 32
#endif

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Generates a spiral of blocks to be rendered.
 *
 * \author Adam Arbree
 * Aug 25, 2005
 * RayTracer.java
 * Used with permission.
 * Copyright 2005 Program of Computer Graphics, Cornell University
 * \ingroup librender
 */
class MTS_EXPORT_RENDER Spiral : public Object {
public:
    /// Create a new spiral generator for the given film and block size
    Spiral(const Film *film, size_t block_size);

    /// Return the maximum block size
    size_t max_block_size() const { return m_block_size; }

    /// Return the total number of blocks
    size_t block_count() { return m_block_count; }

    /// Reset the spiral to its initial state
    void reset();

    /**
     * \brief Return the offset and size of the next block.
     *
     * A size of zero indicates that the spiral traversal is done.
     */
    std::pair<Vector2i, Vector2i> next_block();

    MTS_DECLARE_CLASS()

protected:
    enum EDirection {
        ERight = 0,
        EDown,
        ELeft,
        EUp
    };

    size_t m_block_counter, //< Number of blocks generated so far
           m_block_count,   //< Total number of blocks to be generated
           m_block_size;    //< Size of the (square) blocks (in pixels)

    Vector2i m_size,        //< Size of the 2D image (in pixels).
             m_offset,      //< Offset to the crop region on the sensor (pixels).
             m_blocks;      //< Number of blocks in each direction.

    Point2i  m_position;    //< Relative position of the current block.

    /// Direction where the spiral is currently headed.
    int m_current_direction;

    /// Step counters.
    int m_steps_left, m_steps;

    /// Protects the spiral's state (thread safety).
    tbb::spin_mutex m_mutex;
};

NAMESPACE_END(mitsuba)
