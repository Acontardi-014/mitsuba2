#pragma once

#include <mitsuba/core/fwd.h>
#include <mitsuba/core/object.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/core/timer.h>
#include <mitsuba/core/tls.h>
#include <mitsuba/core/vector.h>
#include <mitsuba/render/fwd.h>
#include <mitsuba/render/imageblock.h>
#include <mitsuba/render/interaction.h>
#include <mitsuba/render/records.h>
#include <mitsuba/render/scene.h>
#include <mitsuba/render/shape.h>

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Abstract integrator base-class; does not make any assumptions on
 * how radiance is computed.
 *
 * In Mitsuba, the different rendering techniques are collectively referred to
 * as \a integrators, since they perform integration over a high-dimensional
 * space. Each integrator represents a specific approach for solving the light
 * transport equation---usually favored in certain scenarios, but at the same
 * time affected by its own set of intrinsic limitations. Therefore, it is
 * important to carefully select an integrator based on user-specified accuracy
 * requirements and properties of the scene to be rendered.
 *
 * This is the base class of all integrators; it does not make any assumptions on
 * how radiance is computed, which allows for many different kinds of implementations
 * ranging from software-based path tracing and Markov-Chain based techniques such
 * as Metropolis Light Transport up to hardware-accelerated rasterization.
 */
class MTS_EXPORT_RENDER Integrator : public Object {
public:
    /// Perform the main rendering job
    virtual bool render(Scene *scene, bool vectorize) = 0;

    /**
     * \brief Cancel a running render job
     *
     * This function can be called asynchronously to cancel a running render
     * job. In this case, \ref render() will quit with a return value of
     * \c false.
     */
    virtual void cancel() = 0;


    // =========================================================================
    //! @{ \name Progress reporting / callbacks
    // =========================================================================
    /**
     * Signature of callbacks that may be registered. Called with:
     * - Thread ID
     * - Pointer to this thread's bitmap, used to accumulate the results
     *   locally. This should typically point to the same object across calls.
     * - Extra information (e.g. an enumeration value indicating a type of
     *   event).
     */
    using CallbackFunction = std::function<void(size_t, Bitmap*, Float)>;

    /**
     * Adds a callback function, which will be called with the desired
     * frequency.
     * Not thread-safe.
     *
     * @param cb     Callback function to be called.
     * @param period Minimum time elapsed (in seconds) between two calls. May
     *               be zero, in which case the callback is called at each
     *               iteration of the algorithm.
     * @return The index of this callback (used to de-register this callback).
     */
    size_t register_callback(CallbackFunction cb, Float period);

    /**
     * De-registers callback at the specified index.
     * If (size_t) -1 is passed, all callbacks are cleared.
     */
    void remove_callback(size_t cb_index = (size_t) -1);

    size_t callback_count() const {
        return m_callbacks.size();
    }

    /**
     * Maybe trigger a call to the callbacks. This will be rate-limited so that
     * the period specified by the caller is respected *per-thread* (i.e. each
     * thread will count time independently).
     *
     * Callbacks are called in the order the were registered.
     */
    virtual void notify(Bitmap *bitmap, Float extra = 1.0f);

    //! @}
    // =========================================================================


    MTS_DECLARE_CLASS()
protected:
    /// Create an integrator
    Integrator(const Properties &props);

    /// Virtual destructor
    virtual ~Integrator() { }

protected:
    struct CallbackInfo {
        CallbackFunction f;
        Float period;
        ThreadLocal<Float> last_called;

        CallbackInfo(const CallbackFunction &f, Float period)
            : f(f), period(period) {}
    };

    /// List of registered callback functions.
    std::vector<CallbackInfo> m_callbacks;
    /// Timer used to enforce the callback rate limits.
    Timer m_cb_timer;
};

/** \brief Abstract base class, which describes integrators
 * capable of computing samples of the scene's radiance function.
 */
class MTS_EXPORT_RENDER SamplingIntegrator : public Integrator {
public:
    /**
     * \brief Sample the incident radiance along a ray. The record passed is
     * used to store additional information about the result.
     */
    virtual Spectrumf eval(const RayDifferential3f &ray,
                           RadianceSample3f &rs) const = 0;

    /// Vectorized variant of \ref eval.
    virtual SpectrumfP eval(const RayDifferential3fP &ray,
                            RadianceSample3fP &rs,
                            MaskP active = true) const = 0;

    /// Compatibility wrapper, which strips the mask argument and invokes \ref eval()
    Spectrumf eval(const RayDifferential3f &ray, RadianceSample3f &rs,
                   bool /* unused */) const {
        return eval(ray, rs);
    }

    // =========================================================================
    //! @{ \name Integrator interface implementation
    // =========================================================================

    bool render(Scene *scene, bool vectorize) override;
    void cancel() override;

    /**
     * Indicates whether \ref cancel() or a timeout have occured. Should be
     * checked often in each integrator's main loop so that timeout is
     * enforced accurately.
     *
     * Note that accurate timeouts rely on \ref m_render_timer, which needs
     * to be reset at the beginning of the rendering phase.
     */
    bool should_stop() const {
        return m_stop || (m_timeout > 0.0f &&
                          m_render_timer.value() > 1000.0f * m_timeout);
    }

    //! @}
    // =========================================================================

    MTS_DECLARE_CLASS()
protected:
    SamplingIntegrator(const Properties &props);
    virtual ~SamplingIntegrator();

    virtual void render_block_scalar(const Scene *scene, Sampler *sampler,
                                     ImageBlock *block) const;

    virtual void render_block_vector(const Scene *scene, Sampler *sampler,
                                     ImageBlock *block, Point2fX &points) const;

protected:
    /// Integrators should stop computing when this flag is set to true.
    bool m_stop;
    /// Size of (square) image blocks to render per core.
    size_t m_block_size;

    /** Maximum amount of time to spend rendering (excluding scene parsing).
     * Specified in seconds. A negative values indicates no timeout. */
    Float m_timeout;
    /// Timer used to enforce the timeout.
    Timer m_render_timer;
};

/*
 * \brief Base class of all recursive Monte Carlo integrators, which compute
 * unbiased solutions to the rendering equation (and optionally the radiative
 * transfer equation).
 */
class MTS_EXPORT_RENDER MonteCarloIntegrator : public SamplingIntegrator {
public:
    MTS_DECLARE_CLASS()
protected:
    /// Create an integrator
    MonteCarloIntegrator(const Properties &props);

    /// Virtual destructor
    virtual ~MonteCarloIntegrator();
protected:
    int m_max_depth;
    int m_rr_depth;
    bool m_strict_normals;
};

/// Instantiates concrete scalar and packet versions of the endpoint plugin API
#define MTS_IMPLEMENT_INTEGRATOR()                                             \
    using SamplingIntegrator::eval;                                            \
    Spectrumf eval(const RayDifferential3f &ray, RadianceSample3f &rs)         \
        const override {                                                       \
        return eval_impl(ray, rs, true);                                       \
    }                                                                          \
    SpectrumfP eval(const RayDifferential3fP &ray, RadianceSample3fP &rs,      \
                    MaskP active) const override {                             \
        return eval_impl(ray, rs, active);                                     \
    }

NAMESPACE_END(mitsuba)
