import numpy as np

from mitsuba.core import Frame3f, MTS_WAVELENGTH_SAMPLES
from mitsuba.core.math import Pi
from mitsuba.core.xml import load_string
from mitsuba.render import BSDF, BSDFContext, SurfaceInteraction3f


def test01_create():
    b = load_string("<bsdf version='2.0.0' type='diffuse'></bsdf>")
    assert b is not None
    assert b.component_count() == 1
    assert b.flags(0) == BSDF.EDiffuseReflection | BSDF.EFrontSide
    assert b.flags() == b.flags(0)


def test02_eval_pdf():
    bsdf = load_string("<bsdf version='2.0.0' type='diffuse'></bsdf>")

    si    = SurfaceInteraction3f()
    si.p  = [0, 0, 0]
    si.n  = [0, 0, 1]
    si.wi = [0, 0, 1]
    si.sh_frame = Frame3f(si.n)
    si.wavelengths = [532] * MTS_WAVELENGTH_SAMPLES

    ctx = BSDFContext()

    for theta in np.linspace(0, np.pi / 2, 20):
        wo = [np.sin(theta), 0, np.cos(theta)]

        v_pdf  = bsdf.pdf(ctx, si, wo=wo)
        v_eval = bsdf.eval(ctx, si, wo=wo)[0]
        assert np.allclose(v_pdf, wo[2] / Pi)
        assert np.allclose(v_eval, 0.5 * wo[2] / Pi)
