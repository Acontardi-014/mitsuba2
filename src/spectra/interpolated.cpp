#include <mitsuba/render/texture.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/math.h>

NAMESPACE_BEGIN(mitsuba)

/// Linear interpolant of a regularly sampled spectrum
template <typename Float, typename Spectrum>
class InterpolatedSpectrum final : public Texture<Float, Spectrum> {
public:
    MTS_DECLARE_CLASS_VARIANT(InterpolatedSpectrum, Texture)
    MTS_IMPORT_BASE(Texture)
    MTS_IMPORT_TYPES()

    using Index = replace_scalar_t<Wavelength, uint32_t>;
    using FloatStorage = host_vector<Float>;

public:
    /**
     * \brief Construct a linearly interpolated spectrum
     *
     * \param lambda_min
     *      Lowest wavelength value associated with a sample
     *
     * \param lambda_max
     *      Largest wavelength value associated with a sample
     *
     * \param size
     *      Number of sample values
     *
     * \param values
     *      Pointer to the sample values. The data is copied,
     *      hence there is no need to keep 'data' alive.
     */
    InterpolatedSpectrum(const Properties &props) {
        m_lambda_min = props.float_("lambda_min");
        m_lambda_max = props.float_("lambda_max");

        if (props.type("values") == Properties::Type::String) {
            std::vector<std::string> values_str =
                string::tokenize(props.string("values"), " ,");

            m_data.resize(values_str.size());
            int ctr = 0;
            for (const auto &s : values_str) {
                try {
                    m_data[ctr++] = (ScalarFloat) std::stod(s);
                } catch (...) {
                    Throw("Could not parse floating point value '%s'", s);
                }
            }
        } else {
            size_t size = props.size_("size");
            const ScalarFloat *values = (ScalarFloat *) props.pointer("values");
            m_data = FloatStorage(values, values + size);
        }

        if (m_data.size() < 2)
            Throw("InterpolatedSpectrum must have at least 2 entries!");

        size_t size = m_data.size();
        m_interval_size = ScalarFloat((double(m_lambda_max) - double(m_lambda_min)) / (size - 1));

        if (m_interval_size <= 0)
            Throw("InterpolatedSpectrum: interval size must be positive!");

        bool expanded = false;
        while (m_lambda_min > MTS_WAVELENGTH_MIN) {
            m_data.insert(m_data.begin(), m_data[0]);
            m_lambda_min -= m_interval_size;
            size += 1;
            expanded = true;
        }

        while (m_lambda_max < MTS_WAVELENGTH_MAX) {
            m_data.push_back(m_data[m_data.size() - 1]);
            m_lambda_max += m_interval_size;
            size += 1;
            expanded = true;
        }

        if (expanded)
            Log(Debug, "InterpolatedSpectrum was expanded to cover wavelength range [%.1f, %.1f]",
                MTS_WAVELENGTH_MIN, MTS_WAVELENGTH_MAX);

        m_inv_interval_size = ScalarFloat((size - 1) / (double(m_lambda_max) - double(m_lambda_min)));
        m_size_minus_2 = uint32_t(size - 2);

        m_cdf.resize(size);

        parameters_changed();
    }

    /// Note: this assumes that the wavelengths and number of entries have not changed.
    void parameters_changed() override {
        // TODO: copy values to CPU to compute CDF?

        // Update CDF, normalization and integral from the new values
        m_cdf[0] = 0.f;
        size_t size = m_data.size();

        // Compute a probability mass function for each interval
        double scale = 0.5 * (double(m_lambda_max) - double(m_lambda_min)) / (size - 1),
               accum = 0.0;
        for (size_t i = 1; i < size; ++i) {
            accum += scale * (double(m_data[i - 1]) + double(m_data[i]));
            m_cdf[i] = ScalarFloat(accum);
        }

        // Store the normalization factor
        m_integral = ScalarFloat(accum);
        m_normalization = ScalarFloat(1.0 / accum);
    }

    template <typename Wavelength>
    Wavelength data_gather(const Index &index, const mask_t<Index> &active) const {
        return gather<Wavelength>(m_data.data(), index, active);
    }
    template <typename Wavelength>
    Wavelength cdf_gather(const Index &index, const mask_t<Index> &active) const {
        return gather<Wavelength>(m_cdf.data(), index, active);
    }

    UnpolarizedSpectrum eval(const SurfaceInteraction3f &si, Mask active_) const override {
        if constexpr (is_spectral_v<Spectrum>) {
            Wavelength t = (si.wavelengths - m_lambda_min) * m_inv_interval_size;
            mask_t<Wavelength> active = active_;
            active &= (si.wavelengths >= m_lambda_min) && (si.wavelengths <= m_lambda_max);

            Index i0 = clamp(Index(t), zero<Index>(), Index(m_size_minus_2)),
                  i1 = i0 + Index(1);

            Wavelength v0 = data_gather<Wavelength>(i0, active),
                       v1 = data_gather<Wavelength>(i1, active);

            Wavelength w1 = t - Wavelength(i0),
                       w0 = 1.f - w1;

            return (w0 * v0 + w1 * v1) & active;
        } else {
            Throw("Not implemented for non-spectral modes"); // TODO
        }
    }

    Wavelength pdf(const SurfaceInteraction3f &si, Mask active) const override {
        return eval(si, active) * m_normalization;
    }

    std::pair<Wavelength, UnpolarizedSpectrum> sample(const SurfaceInteraction3f & /*si*/,
                                                      const Wavelength &sample_,
                                                      Mask active) const override {
        if constexpr (is_spectral_v<Spectrum>) {
            Wavelength sample = sample_ * m_integral;

            // TODO: find_interval on differentiable m_cdf?
            Index i0 = math::find_interval(m_cdf.size(),
                [&](const Index &idx, const mask_t<Index> &active) {
                    return cdf_gather<Wavelength>(idx, active) <= sample;
                },
                active
            );
            Index i1 = i0 + Index(1);

            Wavelength f0 = data_gather<Wavelength>(i0, active),
                    f1 = data_gather<Wavelength>(i1, active);

            // Reuse the sample
            sample = (sample - cdf_gather<Wavelength>(i0, active)) * m_inv_interval_size;

            // Importance sample the linear interpolant
            Wavelength t_linear = (f0 - safe_sqrt(f0 * f0 + 2 * sample * (f1 - f0))) / (f0 - f1),
                       t_const  = sample / f0,
                       t        = select(eq(f0, f1), t_const, t_linear);

            return {
                m_lambda_min + (Wavelength(i0) + t) * m_interval_size,
                m_integral
            };
        } else {
            Throw("Not implemented for non-spectral modes"); // TODO
        }
    }

    ScalarFloat mean() const override {
        return m_integral / (MTS_WAVELENGTH_MAX - MTS_WAVELENGTH_MIN);
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "InterpolatedSpectrum[" << std::endl
            << "  size = " << m_data.size() << "," << std::endl
            << "  lambda_min = " << m_lambda_min << "," << std::endl
            << "  lambda_max = " << m_lambda_max << "," << std::endl
            << "  interval_size = " << m_interval_size << "," << std::endl
            << "  integral = " << m_integral << "," << std::endl
            << "  normalization = " << m_normalization << "," << std::endl
            << "  data = " << m_data << std::endl
            << "]";
        return oss.str();
    }

private:
    FloatStorage m_data, m_cdf;
    uint32_t m_size_minus_2;
    ScalarFloat m_lambda_min;
    ScalarFloat m_lambda_max;
    ScalarFloat m_interval_size;
    ScalarFloat m_inv_interval_size;
    ScalarFloat m_integral;
    ScalarFloat m_normalization;
};

MTS_EXPORT_PLUGIN(InterpolatedSpectrum, "Interpolated spectrum")
NAMESPACE_END(mitsuba)
