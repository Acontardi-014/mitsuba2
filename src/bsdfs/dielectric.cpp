#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/fresnel.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/render/ior.h>

NAMESPACE_BEGIN(mitsuba)

/**!

.. _bsdf-dielectric:

Smooth dielectric material (:monosp:`dielectric`)
-------------------------------------------------


.. list-table::
 :widths: 20 15 65
 :header-rows: 1
 :class: paramstable

 * - Parameter
   - Type
   - Description
 * - int_ior
   - |float| or |string|
   - Interior index of refraction specified numerically or using a known material name. (Default: bk7 / 1.5046)
 * - ext_ior
   - |float| or |string|
   - Exterior index of refraction specified numerically or using a known material name.  (Default: air / 1.000277)
 * - specular_reflectance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular reflection component. Note that for physical realism, this parameter should never be touched. (Default: 1.0)
 * - specular_transmittance
   - |spectrum| or |texture|
   - Optional factor that can be used to modulate the specular transmission component. Note that for physical realism, this parameter should never be touched. (Default: 1.0)


.. subfigstart::
.. _fig-dielectric-plain:

.. figure:: ../../resources/data/docs/images/render/bsdf_dielectric_glass.jpg
    :alt: Homogeneous reflectance
    :width: 95%
    :align: center

    Air ↔ Water (IOR: 1.33) interface.

.. _fig-dielectric-textured:

.. figure:: ../../resources/data/docs/images/render/bsdf_dielectric_diamond.jpg
    :alt: Textured reflectance
    :width: 95%
    :align: center

    Air ↔ Diamond (IOR: 2.419)

.. subfigend::
    :width: 0.49
    :alt: Example dielectric appearances
    :label: fig-dielectric-bsdf

This plugin models an interface between two dielectric materials having mismatched
indices of refraction (for instance, water and air). Exterior and interior IOR values
can be specified independently, where "exterior" refers to the side that contains
the surface normal. When no parameters are given, the plugin activates the defaults, which
describe a borosilicate glass BK7/air interface.

In this model, the microscopic structure of the surface is assumed to be perfectly
smooth, resulting in a degenerate BSDF described by a Dirac delta distribution.
This means that for any given incoming ray of light, the model always scatters into a discrete set of directions, as opposed to a continuum.
For a similar model that instead describes a rough surface microstructure, take a look at the :ref:`roughdielectric <bsdf-roughdielectric>` plugin.

This snippet describes a simple air-to-water interface

.. code-block:: xml
    :name: dielectric-water

    <shape type="...">
        <bsdf type="dielectric">
            <string name="int_ior" value="water"/>
            <string name="ext_ior" value="air"/>
        </bsdf>
    <shape>

When using this model, it is crucial that the scene contains
meaningful and mutually compatible indices of refraction changes---see
Figure |nbsp| :num:`fig-glass-explanation` for a description of what this entails.

In many cases, we will want to additionally describe the *medium* within a
dielectric material. This requires the use of a rendering technique that is
aware of media (e.g. the volumetric path tracer). An example of how one might
describe a slightly absorbing piece of glass is shown below:

.. code-block:: xml
    :name: dielectric-glass

    <shape type="...">
        <bsdf type="dielectric">
            <float name="int_ior" value="1.504"/>
            <float name="ext_ior" value="1.0"/>
        </bsdf>

        <medium type="homogeneous" name="interior">
            <rgb name="sigma_s" value="0, 0, 0"/>
            <rgb name="sigma_a" value="4, 4, 2"/>
        </medium>
    <shape>

.. note::

    Dispersion is currently unsupported but will be enabled in a future release.


.. figtable::
    :label: table-list
    :caption: This table lists all supported material names
       along with along with their associated index of re-fraction at standard conditions.
       These material names can be used with the plugins :ref:`dielectric <bsdf-dielectric>`,
       :ref:`roughdielectric <bsdf-roughdielectric>`, :ref:`plastic <bsdf-plastic>`,
       :ref:`roughplastic <bsdf-roughplastic>`, as well as :ref:`coating <bsdf-coating>`.
    :alt: List table


    .. list-table::
        :widths: 30 25 30 15
        :header-rows: 1

        * - Name
          - Value
          - Name
          - Value
        * - vacuum
          - 1.0
          - acetone
          - 1.36
        * - bromine
          - 1.661
          - bk7
          - 1.5046
        * - helium
          - 1.00004
          - ethanol
          - 1.361
        * - water ice
          - 1.31
          - sodium chloride
          - 1.544
        * - hydrogen
          - 1.00013
          - carbon tetrachloride
          - 1.461
        * - fused quartz
          - 1.458
          - amber
          - 1.55
        * - air
          - 1.00028
          - glycerol
          - 1.4729
        * - pyrex
          - 1.470
          - pet
          - 1.575
        * - carbon dioxide
          - 1.00045
          - benzene
          - 1.501
        * - acrylic glass
          - 1.49
          - diamond
          - 2.419
        * - water
          - 1.3330
          - silicone oil
          - 1.52045
        * - polypropylene
          - 1.49
          -
          -


 */

template <typename Float, typename Spectrum>
class SmoothDielectric final : public BSDF<Float, Spectrum> {
public:
    MTS_DECLARE_CLASS_VARIANT(SmoothDielectric, BSDF);
    MTS_IMPORT_BASE(BSDF, m_flags, m_components)
    MTS_IMPORT_TYPES(Texture)

    SmoothDielectric(const Properties &props) : Base(props) {

        // Specifies the internal index of refraction at the interface
        ScalarFloat int_ior = lookup_ior(props, "int_ior", "bk7");

        // Specifies the external index of refraction at the interface
        ScalarFloat ext_ior = lookup_ior(props, "ext_ior", "air");

        if (int_ior < 0 || ext_ior < 0)
            Throw("The interior and exterior indices of refraction must"
                  " be positive!");

        m_eta = int_ior / ext_ior;

        m_specular_reflectance   = props.texture<Texture>("specular_reflectance", 1.f);
        m_specular_transmittance = props.texture<Texture>("specular_transmittance", 1.f);

        m_components.push_back(BSDFFlags::DeltaReflection | BSDFFlags::FrontSide |
                               BSDFFlags::BackSide);
        m_components.push_back(BSDFFlags::DeltaTransmission | BSDFFlags::FrontSide |
                               BSDFFlags::BackSide | BSDFFlags::NonSymmetric);

        m_flags = m_components[0] | m_components[1];
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                                           Float sample1, const Point2f & /*sample2*/,
                                           Mask active) const override {
        bool has_reflection   = ctx.is_enabled(BSDFFlags::DeltaReflection, 0),
             has_transmission = ctx.is_enabled(BSDFFlags::DeltaTransmission, 1);

        // Evaluate the Fresnel equations for unpolarized illumination
        Float cos_theta_i = Frame3f::cos_theta(si.wi);

        auto [r_i, cos_theta_t, eta_it, eta_ti] = fresnel(cos_theta_i, Float(m_eta));
        Float t_i = 1.f - r_i;

        // Lobe selection
        BSDFSample3f bs;
        Spectrum weight;
        Mask selected_r;
        if (likely(has_reflection && has_transmission)) {
            selected_r = sample1 <= r_i && active;
            weight = 1.f;
            bs.pdf = select(selected_r, r_i, t_i);
        } else {
            if (has_reflection || has_transmission) {
                selected_r = Mask(has_reflection) && active;
                weight = has_reflection ? r_i : t_i;
                bs.pdf = 1.f;
            } else {
                return { bs, 0.f };
            }
        }

        bs.sampled_component = select(selected_r, UInt32(0), UInt32(1));
        bs.sampled_type      = select(selected_r, UInt32(+BSDFFlags::DeltaReflection),
                                                  UInt32(+BSDFFlags::DeltaTransmission));

        bs.wo = select(selected_r,
                       reflect(si.wi),
                       refract(si.wi, cos_theta_t, eta_ti));

        bs.eta = select(selected_r, Float(1.f), eta_it);

        if (any_or<true>(selected_r))
            weight[selected_r] *=
                m_specular_reflectance->eval(si, selected_r);

        Mask selected_t = !selected_r && active;
        if (any_or<true>(selected_t)) {
            /* For transmission, radiance must be scaled to account for the solid
               angle compression that occurs when crossing the interface. */
            Float factor = (ctx.mode == TransportMode::Radiance) ? eta_ti : Float(1.f);

            weight[selected_t] *=
                m_specular_transmittance->eval(si, selected_t) * sqr(factor);
        }

        return { bs, select(active, weight, 0.f) };
    }

    Spectrum eval(const BSDFContext & /*ctx*/, const SurfaceInteraction3f & /*si*/,
                  const Vector3f & /*wo*/, Mask /*active*/) const override {
        return 0.f;
    }

    Float pdf(const BSDFContext & /*ctx*/, const SurfaceInteraction3f & /*si*/,
              const Vector3f & /*wo*/, Mask /*active*/) const override {
        return 0.f;
    }

    void traverse(TraversalCallback *callback) override {
        callback->put_parameter("eta", m_eta);
        callback->put_object("specular_reflectance", m_specular_reflectance.get());
        callback->put_object("specular_transmittance", m_specular_transmittance.get());
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "SmoothDielectric[" << std::endl
            << "  eta = " << m_eta << "," << std::endl
            << "  specular_reflectance = " << string::indent(m_specular_reflectance) << "," << std::endl
            << "  specular_transmittance = " << string::indent(m_specular_transmittance) << std::endl
            << "]";
        return oss.str();
    }

private:
    ScalarFloat m_eta;
    ref<Texture> m_specular_reflectance;
    ref<Texture> m_specular_transmittance;
};

MTS_EXPORT_PLUGIN(SmoothDielectric, "Smooth dielectric")
NAMESPACE_END(mitsuba)
