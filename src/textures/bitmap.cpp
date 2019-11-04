#include <mitsuba/core/bitmap.h>
#include <mitsuba/core/fresolver.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/render/spectrum.h>
#include <mitsuba/render/srgb.h>

NAMESPACE_BEGIN(mitsuba)

// Forward declaration
template <typename Float, typename Spectrum, size_t ChannelCount, bool IsRawData>
class BitmapTextureImpl;

/**
 * Bilinearly interpolated image texture.
 */
// TODO(!): test the "single-channel texture" use-case that was added during refactoring
template <typename Float, typename Spectrum>
class BitmapTexture : public ContinuousSpectrum<Float, Spectrum> {
public:
    MTS_REGISTER_CLASS(BitmapTexture, ContinuousSpectrum)
    MTS_USING_BASE(ContinuousSpectrum)
    MTS_IMPORT_TYPES()

    BitmapTexture(const Properties &props) {
        m_transform = props.transform("to_uv", Transform4f()).extract();

        auto fs = Thread::thread()->file_resolver();
        fs::path file_path = fs->resolve(props.string("filename"));
        m_name = file_path.filename().string();
        Log(Debug, "Loading bitmap texture from \"%s\" ..", m_name);

        m_bitmap = new Bitmap(file_path);

        // Optional texture upsampling
        ScalarFloat upscale = props.float_("upscale", 1.f);
        if (upscale != 1.f) {
            using SVector2s = Vector<size_t, 2>;
            using ReconstructionFilter = mitsuba::ReconstructionFilter<Float>;

            SVector2s old_size = m_bitmap->size(),
                      new_size = upscale * old_size;

            auto rfilter =
                PluginManager::instance()->create_object<ReconstructionFilter>(Properties("tent"));

            Log(Info, "Upsampling bitmap texture \"%s\" to %ix%i ..", m_name,
                       new_size.x(), new_size.y());
            m_bitmap = m_bitmap->resample(new_size, rfilter);
        }

        m_is_raw_data = props.bool_("raw_data", false) || !props.bool_("color_processing", true);
    }

    BitmapTexture(Bitmap *bitmap, const std::string &name, const Transform3f &transform)
        : m_bitmap(bitmap), m_name(name), m_transform(transform) {}

    /**
     * Recursively expand into an implementation specialized for the channel count
     * of the bitmap actually loaded.
     */
    std::vector<ref<Object>> expand() const override {
        // Allow bitmap to be modified in expanded object.
        Bitmap *bitmap = const_cast<Bitmap *>(m_bitmap.get());
#define MTS_SELECT_IMPL(IsRaw)                                                                     \
    switch (m_bitmap->channel_count()) {                                                           \
        case 1:                                                                                    \
            return { new BitmapTextureImpl<Float, Spectrum, 1, IsRaw>(bitmap, m_name,              \
                                                                      m_transform) };              \
        case 3:                                                                                    \
            return { new BitmapTextureImpl<Float, Spectrum, 3, IsRaw>(bitmap, m_name,              \
                                                                      m_transform) };              \
        case 4:                                                                                    \
            return { new BitmapTextureImpl<Float, Spectrum, 4, IsRaw>(bitmap, m_name,              \
                                                                      m_transform) };              \
        default:                                                                                   \
            Throw("Unsupported channel count: %d (expected 1, 3 or 4)",                            \
                  m_bitmap->channel_count());                                                      \
    }

        if (m_is_raw_data) {
            MTS_SELECT_IMPL(true)
        } else {
            MTS_SELECT_IMPL(false)
        }
#undef MTS_SELECT_IMPL
    }

protected:
    ref<Bitmap> m_bitmap;
    std::string m_name;
    Transform3f m_transform;
    /// Whether this texture holds raw data, which should not be interpreted as colors.
    bool m_is_raw_data;
};

template <typename Float, typename Spectrum, size_t ChannelCount, bool IsRawData>
class BitmapTextureImpl final : public BitmapTexture<Float, Spectrum> {
public:
    MTS_REGISTER_CLASS(BitmapTextureImpl, BitmapTexture)
    MTS_IMPORT_TYPES()
    MTS_USING_BASE(BitmapTexture, m_bitmap, m_name, m_transform)
    using StorageType = Vector<ScalarFloat, ChannelCount>;

    BitmapTextureImpl(Bitmap *bitmap, const std::string &name, const Transform3f &transform)
        : Base(bitmap, name, transform) {

        /* Convert to linear RGB float bitmap, will be converted
           into spectral profile coefficients below (in place) */
        auto color_mode = IsRawData ? m_bitmap->pixel_format() : Bitmap::ERGB;
        m_bitmap = m_bitmap->convert(color_mode, Bitmap::EFloat, false);

        // Convert to spectral coefficients, monochrome or leave in RGB.
        auto *ptr = (ScalarFloat *) m_bitmap->data();
        double mean = 0.0;
        for (size_t i = 0; i < m_bitmap->pixel_count(); ++i) {
            // Load data from the scalar-typed buffer
            const StorageType raw = load_unaligned<StorageType>(ptr);

            if constexpr (IsRawData || (is_monochrome_v<Spectrum> && ChannelCount == 1)) {
                // Leave data untouched
                mean += (double) hsum(raw) / StorageType::Size;
            } else if constexpr (is_monochrome_v<Spectrum>) {
                // Convert to luminance
                ScalarFloat lum;
                if constexpr (ChannelCount <= 3)
                    lum = luminance(raw);
                else
                    lum = hsum(raw) / ChannelCount;
                mean += (double) lum;
                store_unaligned(ptr, lum);
            } else if constexpr (is_spectral_v<Spectrum>) {
                // Fetch spectral fit for given sRGB color value (3 coefficients)
                ScalarVector3f coeff = srgb_model_fetch(raw);
                mean += (double) srgb_model_mean(coeff);
                store_unaligned(ptr, coeff);
            }

            ptr += ChannelCount;
        }

        m_mean = Float(mean / hprod(m_bitmap->size()));

#if defined(MTS_ENABLE_AUTODIFF)
        m_bitmap_d = FloatC::copy(m_bitmap->data(), hprod(m_bitmap->size()) * 3);
#endif
    }

    // Evaluation of color data
    Spectrum eval(const SurfaceInteraction3f &si, Mask active) const override {
        if constexpr (ChannelCount < 3) {
            // TODO: we might still want to upscale, maybe let the user pass a flag to allow it
            Throw("Cannot spectrally evaluate a %d-channel texture", ChannelCount);
        } else if (IsRawData) {
            Throw("Cannot spectrally evaluate a raw-data texture");
        } else {
            return interpolate<Vector3f, false, depolarized_t<Spectrum>>(si, active);
        }
    }

    // Evaluation of raw 3-channel data (e.g. normal map)
    Vector3f eval3(const SurfaceInteraction3f &si, Mask active = true) const override {
        if constexpr (ChannelCount != 1) {
            // TODO: we might still want to downscale, maybe let the user pass a flag to allow it
            Throw("Cannot evaluate a %d-channel texture as a scalar", ChannelCount);
        } else if (!IsRawData) {
            Throw("Cannot evaluate a color texture as raw data");
        } else {
            return interpolate<Vector3f, true, Vector3f>(si, active);
        }
    }

    // Evaluation of raw scalar data
    Float eval1(const SurfaceInteraction3f &si, Mask active = true) const override {
        if constexpr (ChannelCount != 1) {
            // TODO: we might still want to downscale, maybe let the user pass a flag to allow it
            Throw("Cannot evaluate a %d-channel texture as a scalar", ChannelCount);
        } else {
            return interpolate<Float, true, Float>(si, active);
        }
    }

    template <typename GatherType, bool IsRaw, typename Return>
    Return interpolate(const SurfaceInteraction3f &si, Mask active) const {
        Point2f uv = m_transform.transform_affine(si.uv);
        uv -= floor(uv);
        uv *= Vector2f(m_bitmap->size() - 1u);

        Point2u pos = min(Point2u(uv), m_bitmap->size() - 2u);

        Point2f w1 = uv - Point2f(pos),
                w0 = 1.f - w1;

        UInt32 index = pos.x() + pos.y() * (uint32_t) m_bitmap->size().x();

        if constexpr (is_diff_array_v<Float>) {
            // TODO: restore and unify this for GPU
            NotImplementedError("interpolate on the GPU");
// #if defined(MTS_ENABLE_AUTODIFF)
//             uint32_t width = (uint32_t) m_bitmap->size().x();
//             v00 = gather<Vector3f>(m_bitmap_d, index, active);
//             v10 = gather<Vector3f>(m_bitmap_d, index + 1u, active);
//             v01 = gather<Vector3f>(m_bitmap_d, index + width, active);
//             v11 = gather<Vector3f>(m_bitmap_d, index + width + 1u, active);
// #endif
        }
        uint32_t width = (uint32_t) m_bitmap->size().x() * 3;
        auto *ptr = (const ScalarFloat *) m_bitmap->data();

        GatherType v00 = gather<GatherType, 0, true>(ptr, index, active);
        GatherType v10 = gather<GatherType, 0, true>(ptr + ChannelCount, index, active);
        GatherType v01 = gather<GatherType, 0, true>(ptr + width, index, active);
        GatherType v11 = gather<GatherType, 0, true>(ptr + width + ChannelCount, index, active);

        Return r00, r10, r01, r11;
        if constexpr (IsRaw || !is_spectral_v<Spectrum>) {
            // No further processing on the data stored in memory
            if constexpr (is_monochrome_v<Spectrum> && ChannelCount > 1) {
                r00 = v00.x(); r01 = v01.x(); r10 = v10.x(); r11 = v11.x();
            } else {
                r00 = v00; r01 = v01; r10 = v10; r11 = v11;
            }
        } else {
            // Evaluate spectral upsampling model from stored coefficients
            r00 = srgb_model_eval(v00, si.wavelengths);
            r10 = srgb_model_eval(v10, si.wavelengths);
            r01 = srgb_model_eval(v01, si.wavelengths);
            r11 = srgb_model_eval(v11, si.wavelengths);
        }

        // Bilinear interpolation
        Return r0 = fmadd(w0.x(), r00, w1.x() * r10),
               r1 = fmadd(w0.x(), r01, w1.x() * r11);
        return fmadd(w0.y(), r0, w1.y() * r1);
    }

    Float mean() const override { return m_mean; }

#if defined(MTS_ENABLE_AUTODIFF)
    void put_parameters(DifferentiableParameters &dp) override {
        dp.put(this, "bitmap", m_bitmap_d);
    }
#endif

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Bitmap[" << std::endl
            << "  name = \"" << m_name << "\"," << std::endl
            << "  bitmap = " << string::indent(m_bitmap) << std::endl
            << "]";
        return oss.str();
    }

protected:
    Float m_mean;
};

MTS_IMPLEMENT_PLUGIN(BitmapTexture, ContinuousSpectrum, "Bitmap texture")
NAMESPACE_END(mitsuba)
