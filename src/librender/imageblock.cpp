#include <mitsuba/render/imageblock.h>

NAMESPACE_BEGIN(mitsuba)

ImageBlock::ImageBlock(Bitmap::EPixelFormat fmt, const Vector2i &size,
    const ReconstructionFilter *filter, size_t channels, bool warn)
    : m_offset(0), m_size(size), m_filter(filter),
    m_weights_x(nullptr), m_weights_y(nullptr),
    m_weights_x_p(nullptr), m_weights_y_p(nullptr),
    m_warn(warn) {
    m_border_size = filter ? filter->border_size() : 0;

    // Allocate a small bitmap data structure for the block
    m_bitmap = new Bitmap(fmt, Struct::EType::EFloat,
                          size + Vector2i(2 * m_border_size), channels);
    m_bitmap->clear();

    if (filter) {
        // Temporary buffers used in put()
        // TODO: allocate just the ones we need (either vectorized or scalar).
        int temp_buffer_size = (int) std::ceil(2 * filter->radius()) + 1;
        m_weights_x = new Float[2 * temp_buffer_size];
        m_weights_y = m_weights_x + temp_buffer_size;
        // Vectorized variant
        m_weights_x_p = new FloatP[2 * temp_buffer_size];
        m_weights_y_p = m_weights_x_p + temp_buffer_size;
    }
}

ImageBlock::~ImageBlock() {
    // Note that m_weights_y points to the same array as m_weights_x, so there's
    // no need to delete it.
    if (m_weights_x)
        delete[] m_weights_x;
    if (m_weights_x_p)
        delete[] m_weights_x_p;
}

bool ImageBlock::put(const Point2f &_pos, const Float *value, bool /*unused*/) {
    Assert(m_filter != nullptr);
    const int channels = m_bitmap->channel_count();

    // Check if all sample values are valid
    bool valid_sample = true;
    if (m_warn) {
        for (int i = 0; i < channels; ++i) {
            if (unlikely((!std::isfinite(value[i]) || value[i] < 0))) {
                valid_sample = false;
                break;
            }
        }

        if (unlikely(!valid_sample)) {
            std::ostringstream oss;
            oss << "Invalid sample value: [";
            for (int i = 0; i < channels; ++i) {
                oss << value[i];
                if (i + 1 < channels) oss << ", ";
            }
            oss << "]";
            Log(EWarn, "%s", oss.str());
            return false;
        }
    }

    const Float filter_radius = m_filter->radius();
    const Vector2i &size = m_bitmap->size();

    // Convert to pixel coordinates within the image block
    const Point2f pos(
        _pos.x() - 0.5f - (m_offset.x() - m_border_size),
        _pos.y() - 0.5f - (m_offset.y() - m_border_size)
    );

    // Determine the affected range of pixels
    const Point2i lo(
        std::max((int) std::ceil(pos.x() - filter_radius), 0),
        std::max((int) std::ceil(pos.y() - filter_radius), 0)
    );
    const Point2i hi(
        std::min((int) std::floor(pos.x() + filter_radius), size.x() - 1),
        std::min((int) std::floor(pos.y() + filter_radius), size.y() - 1)
    );

    // Lookup values from the pre-rasterized filter
    for (int x = lo.x(), idx = 0; x <= hi.x(); ++x)
        m_weights_x[idx++] = m_filter->eval_discretized(x - pos.x());
    for (int y = lo.y(), idx = 0; y <= hi.y(); ++y)
        m_weights_y[idx++] = m_filter->eval_discretized(y - pos.y());

    // Rasterize the filtered sample into the framebuffer
    for (int y = lo.y(), yr = 0; y <= hi.y(); ++y, ++yr) {
        const Float weightY = m_weights_y[yr];
        auto dest = static_cast<Float *>(m_bitmap->data())
            + (y * (size_t) size.x() + lo.x()) * channels;

        for (int x = lo.x(), xr = 0; x <= hi.x(); ++x, ++xr) {
            const Float weight = m_weights_x[xr] * weightY;

            for (int k = 0; k < channels; ++k)
                *dest++ += weight * value[k];
        }
    }
    return true;
}

mask_t<FloatP> ImageBlock::put(const Point2fP &_pos, const FloatP *value,
                               const mask_t<FloatP> &active) {
    Assert(m_filter != nullptr);
    using Mask = mask_t<FloatP>;

    const int channels = m_bitmap->channel_count();

    // Check if all sample values are valid
    Mask is_valid(true);
    if (m_warn) {
        for (int k = 0; k < channels; ++k) {
            // We only care about active lanes
            is_valid &= (~active) | (enoki::isfinite(value[k]) & (value[k] >= 0));
        }

        if (unlikely(any(~is_valid))) {
            std::ostringstream oss;
            oss << "Invalid sample value(s): [";
            for (int i = 0; i < channels; ++i) {
                oss << value[i];
                if (i + 1 < channels) oss << ", ";
            }
            oss << "]";
            Log(EWarn, "%s", oss.str());

            if (none(is_valid))  // Early return
                return is_valid;
        }
    }

    const Float filter_radius = m_filter->radius();
    const Vector2i &size = m_bitmap->size();

    // Convert to pixel coordinates within the image block
    const Point2fP pos(
        _pos.x() - 0.5f - (m_offset.x() - m_border_size),
        _pos.y() - 0.5f - (m_offset.y() - m_border_size)
    );

    // Determine the affected range of pixels
    const Point2iP lo(
        max(ceil(pos.x() - filter_radius), 0.0f),
        max(ceil(pos.y() - filter_radius), 0.0f)
    );
    const Point2iP hi(
        min(floor(pos.x() + filter_radius), (int) size.x() - 1.0f),
        min(floor(pos.y() + filter_radius), (int) size.y() - 1.0f)
    );

    // Lookup values from the pre-rasterized filter
    Vector2iP window_sizes = max(hi - lo, 0);
    Point2i max_size(
        hmax(window_sizes.x()),
        hmax(window_sizes.y())
    );

    auto corner = lo - pos;
    for (int i = 0; i <= max_size.x(); ++i)
        m_weights_x_p[i] = m_filter->eval_discretized(corner.x() + i);
    for (int i = 0; i <= max_size.y(); ++i)
        m_weights_y_p[i] = m_filter->eval_discretized(corner.y() + i);


    // Rasterize the filtered sample into the framebuffer
    auto *buffer = (Float *) m_bitmap->data();
    Mask enabled;
    for (int yr = 0; yr <= max_size.y(); ++yr) {
        enabled = active & is_valid & (yr <= window_sizes.y());
        auto y = lo.y() + yr;

        for (int xr = 0; xr <= max_size.x(); ++xr) {
            enabled &= (xr <= window_sizes.x());
            if (none(enabled))
                continue;
            // Linearized offsets: n_channels * (y * n_x + x)
            auto offsets = channels * (y * size.x() + (lo.x() + xr));
            auto weights = m_weights_y_p[yr] * m_weights_x_p[xr];

            for (int k = 0; k < channels; ++k) {
                // We need to be extra-careful about the "histogram problem". See:
                //   http://enoki.readthedocs.io/en/master/
                //   advanced.html#the-histogram-problem-and-conflict-detection.
                enoki::transform<FloatP>(
                    // Base offset into the bitmap's buffer.
                    buffer + k,
                    // Index of each position, relative to the base (may have
                    // repeated values, which is why we're using `transform`).
                    offsets,
                    // Perform operation on active lanes only.
                    enabled,
                    // Operation (accumulate weighted value).
                    [](auto &&x, auto &&w, auto &&v) { x += w * v; },
                    // Reconstruction weights, values to store.
                    weights, value[k]
                );
            }
        }
    }

    return is_valid;
}

std::string ImageBlock::to_string() const {
    std::ostringstream oss;
    oss << "ImageBlock[" << std::endl
        << "  offset = " << m_offset << "," << std::endl
        << "  size = "   << m_size   << "," << std::endl
        << "  border_size = " << m_border_size << std::endl
        << "]";
    return oss.str();
}


template MTS_EXPORT_RENDER bool ImageBlock::put<>(
    const Point2f &, const Spectrumf &, const Float &, const mask_t<Float> &);
template MTS_EXPORT_RENDER mask_t<FloatP> ImageBlock::put<>(
    const Point2fP &, const SpectrumfP &, const FloatP &, const mask_t<FloatP> &);


MTS_IMPLEMENT_CLASS(ImageBlock, Object)
NAMESPACE_END(mitsuba)
