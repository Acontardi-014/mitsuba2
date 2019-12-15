#include <mitsuba/render/integrator.h>
#include <mitsuba/render/records.h>

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Integrator that returns one or more AOVs (Arbitrary Output Variables)
 * describing the visible surfaces.
 */
template <typename Float, typename Spectrum>
class AOVIntegrator final : public SamplingIntegrator<Float, Spectrum> {
public:
    MTS_DECLARE_CLASS_VARIANT(AOVIntegrator, SamplingIntegrator)
    MTS_IMPORT_BASE(SamplingIntegrator)
    MTS_IMPORT_TYPES(Scene, Sampler, SamplingIntegrator)

    enum class Type {
        Depth,
        Position,
        UV,
        GeometricNormal,
        ShadingNormal,
        IntegratorRGBA
    };

    AOVIntegrator(const Properties &props) : Base(props) {
        std::vector<std::string> tokens = string::tokenize(props.string("aovs"));

        for (const std::string &token: tokens) {
            std::vector<std::string> item = string::tokenize(token, ":");

            if (item.size() != 2 || item[0].empty() || item[1].empty())
                Log(Warn, "Invalid AOV specification: require <name>:<type> pair");

            if (item[1] == "depth") {
                m_aov_types.push_back(Type::Depth);
                m_aov_names.push_back(item[0]);
            } else if (item[1] == "position") {
                m_aov_types.push_back(Type::Position);
                m_aov_names.push_back(item[0] + ".x");
                m_aov_names.push_back(item[0] + ".y");
                m_aov_names.push_back(item[0] + ".z");
            } else if (item[1] == "uv") {
                m_aov_types.push_back(Type::UV);
                m_aov_names.push_back(item[0] + ".u");
                m_aov_names.push_back(item[0] + ".v");
            } else if (item[1] == "geo_normal") {
                m_aov_types.push_back(Type::GeometricNormal);
                m_aov_names.push_back(item[0] + ".x");
                m_aov_names.push_back(item[0] + ".y");
                m_aov_names.push_back(item[0] + ".z");
            } else if (item[1] == "sh_normal") {
                m_aov_types.push_back(Type::ShadingNormal);
                m_aov_names.push_back(item[0] + ".x");
                m_aov_names.push_back(item[0] + ".y");
                m_aov_names.push_back(item[0] + ".z");
            } else {
                Throw("Invalid AOV type \"%s\"!", item[1]);
            }
        }

        for (auto &kv : props.objects()) {
            SamplingIntegrator *integrator = dynamic_cast<SamplingIntegrator *>(kv.second.get());
            if (!integrator)
                Throw("Child objects must be of type 'SamplingIntegrator'!");
            m_aov_types.push_back(Type::IntegratorRGBA);
            std::vector<std::string> aovs = integrator->aov_names();
            for (auto name: aovs)
                m_aov_names.push_back(kv.first + "." + name);
            m_integrators.push_back({ integrator, aovs.size() });
            m_aov_names.push_back(kv.first + ".r");
            m_aov_names.push_back(kv.first + ".g");
            m_aov_names.push_back(kv.first + ".b");
            m_aov_names.push_back(kv.first + ".a");
        }

        if (m_aov_names.empty())
            Log(Warn, "No AOVs were specified!");
    }

    std::pair<Spectrum, Mask> sample(const Scene *scene,
                                     Sampler * sampler,
                                     const RayDifferential3f &ray,
                                     Float *aovs,
                                     Mask active) const override {
        ScopedPhase sp(ProfilerPhase::SamplingIntegratorSample);

        std::pair<Spectrum, Mask> result { 0.f, false };

        SurfaceInteraction3f si = scene->ray_intersect(ray, active);
        active = si.is_valid();
        si[!active] = zero<SurfaceInteraction3f>();
        size_t ctr = 0;

        for (size_t i = 0; i < m_aov_types.size(); ++i) {
            switch (m_aov_types[i]) {
                case Type::Depth:
                    *aovs++ = si.t;
                    break;

                case Type::Position:
                    *aovs++ = si.p.x();
                    *aovs++ = si.p.y();
                    *aovs++ = si.p.z();
                    break;

                case Type::UV:
                    *aovs++ = si.uv.x();
                    *aovs++ = si.uv.y();
                    break;

                case Type::GeometricNormal:
                    *aovs++ = si.n.x();
                    *aovs++ = si.n.y();
                    *aovs++ = si.n.z();
                    break;

                case Type::ShadingNormal:
                    *aovs++ = si.sh_frame.n.x();
                    *aovs++ = si.sh_frame.n.y();
                    *aovs++ = si.sh_frame.n.z();
                    break;

                case Type::IntegratorRGBA: {
                        std::pair<Spectrum, Mask> result_sub =
                            m_integrators[ctr].first->sample(scene, sampler, ray, aovs, active);
                        aovs += m_integrators[ctr].second;

                        UnpolarizedSpectrum spec_u = depolarize(result_sub.first);

                        Color3f xyz;
                        if constexpr (is_monochromatic_v<Spectrum>) {
                            xyz = spec_u.x();
                        } else if constexpr (is_rgb_v<Spectrum>) {
                            xyz = srgb_to_xyz(spec_u, active);
                        } else {
                            static_assert(is_spectral_v<Spectrum>);
                            xyz = spectrum_to_xyz(spec_u, ray.wavelengths, active);
                        }

                        Color3f rgb = xyz_to_srgb(xyz);

                        *aovs++ = rgb.r(); *aovs++ = rgb.g(); *aovs++ = rgb.b();
                        *aovs++ = select(result_sub.second, Float(1.f), Float(0.f));

                        if (ctr == 0)
                            result = result_sub;

                        ctr++;
                    }
                    break;
            }
        }

        return result;
    }

    std::vector<std::string> aov_names() const override {
        return m_aov_names;
    }

    void traverse(TraversalCallback *callback) override {
        for (size_t i = 0; i < m_integrators.size(); ++i)
            callback->put_object("integrator_" + std::to_string(i), m_integrators[i].first.get());
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Scene[" << std::endl
            << "  aovs = " << m_aov_names << "," << std::endl
            << "  integrators = [" << std::endl;
        for (size_t i = 0; i < m_integrators.size(); ++i) {
            oss << "    " << string::indent(m_integrators[i].first->to_string(), 4);
            if (i + 1 < m_integrators.size())
                oss << ",";
            oss << std::endl;
        }
        oss << "  ]"<< std::endl
            << "]";
        return oss.str();
    }

private:
    std::vector<Type> m_aov_types;
    std::vector<std::string> m_aov_names;
    std::vector<std::pair<ref<SamplingIntegrator>, size_t>> m_integrators;
};

MTS_EXPORT_PLUGIN(AOVIntegrator, "AOV integrator");
NAMESPACE_END(mitsuba)
