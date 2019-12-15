#include <mitsuba/core/thread.h>
#include <mitsuba/core/tls.h>
#include <mitsuba/python/python.h>
#include <mitsuba/render/integrator.h>

#if defined(__APPLE__) || defined(__linux__)
#  define MTS_HANDLE_SIGINT 1
#else
#  define MTS_HANDLE_SIGINT 0
#endif

#if defined(MTS_HANDLE_SIGINT)
#include <signal.h>

/// Current signal handler
static std::function<void()> sigint_handler;

/// Previously installed signal handler
static void (*sigint_handler_prev)(int) = nullptr;
#endif

MTS_PY_EXPORT(Integrator) {
    MTS_IMPORT_TYPES()
    MTS_IMPORT_OBJECT_TYPES()
    MTS_PY_CHECK_ALIAS(Integrator, m) {
        MTS_PY_CLASS(Integrator, Object)
            .def("render",
                [&](Integrator *integrator, Scene *scene, Sensor *sensor) {
                    py::gil_scoped_release release;

#if defined(MTS_HANDLE_SIGINT)
                    // Install new signal handler
                    sigint_handler = [integrator]() {
                        integrator->cancel();
                    };

                    sigint_handler_prev = signal(SIGINT, [](int) {
                        Log(Warn, "Received interrupt signal, winding down..");
                        if (sigint_handler) {
                            sigint_handler();
                            sigint_handler = std::function<void()>();
                            signal(SIGINT, sigint_handler_prev);
                            raise(SIGINT);
                        }
                    });
#endif

                    bool res = integrator->render(scene, sensor);

#if defined(MTS_HANDLE_SIGINT)
                    // Restore previous signal handler
                    signal(SIGINT, sigint_handler_prev);
#endif

                    return res;
                },
                D(Integrator, render), "scene"_a, "sensor"_a)
            .def_method(Integrator, cancel);
    }

    MTS_PY_CHECK_ALIAS(SamplingIntegrator, m) {
        auto integrator = MTS_PY_CLASS(SamplingIntegrator, Integrator)
            .def_method(SamplingIntegrator, aov_names)
            .def_method(SamplingIntegrator, should_stop);


        if constexpr (is_dynamic_array_v<Float> || is_scalar_v<Float>) {
            integrator
                .def("sample",
                     [](const SamplingIntegrator *integrator,
                        const Scene *scene, Sampler *sampler,
                        const RayDifferential3f &ray, Mask active) {
                          std::vector<Float> aovs(integrator->aov_names().size(), 0.f);
                          auto [spec, mask] = integrator->sample(scene, sampler,
                                                                 ray, aovs.data(), active);
                          return std::make_tuple(spec, mask, aovs);
                     },
                     "scene"_a, "sampler"_a, "ray"_a, "active"_a = true,
                     D(SamplingIntegrator, sample));
        } else {
            using FloatX = make_dynamic_t<Float>;
            using SpectrumX = make_dynamic_t<Spectrum>;
            using MaskX = make_dynamic_t<Mask>;
            using RayDifferential3fX = make_dynamic_t<RayDifferential3f>;

            integrator
                .def("sample",
                     [](const SamplingIntegrator *integrator,
                        const Scene *scene, Sampler *sampler,
                        const RayDifferential3fX &ray,
                        MaskX active) {
                          std::vector<FloatX> result_aovs(integrator->aov_names().size());
                          std::vector<Float> aovs_packet(result_aovs.size());
                          SpectrumX result_spec;
                          MaskX result_mask;
                          set_slices(result_spec, slices(ray));
                          set_slices(result_mask, slices(ray));
                          set_slices(active, slices(ray));
                          for (size_t j = 0; j < result_aovs.size(); ++j)
                            set_slices(result_aovs[j], slices(ray));

                          for (size_t i = 0; i < packets(ray); ++i) {
                              auto [spec, mask] =
                                  integrator->sample(scene, sampler, packet(ray, i),
                                                     aovs_packet.data(), packet(active, i));
                              packet(result_spec, i) = spec;
                              packet(result_mask, i) = mask;
                              for (size_t j = 0; j < result_aovs.size(); ++i)
                                  packet(result_aovs[j], i) = aovs_packet[j];
                          }
                          return std::make_tuple(result_spec, result_mask, result_aovs);
                     },
                     "scene"_a, "sampler"_a, "ray"_a, "active"_a = true,
                     D(SamplingIntegrator, sample));

        }
    }

    MTS_PY_CHECK_ALIAS(MonteCarloIntegrator, m) {
        MTS_PY_CLASS(MonteCarloIntegrator, SamplingIntegrator);
    }
}
