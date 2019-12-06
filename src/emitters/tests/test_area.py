import numpy as np
import os
import pytest

from mitsuba.scalar_rgb.core import warp
from mitsuba.scalar_rgb.core.math import Pi
from mitsuba.scalar_rgb.core.xml import load_string
from mitsuba.scalar_rgb.render import PositionSample3f, Interaction3f
from mitsuba.test.util import fresolver_append_path

@fresolver_append_path
def example_shape(filename = "data/triangle.ply", has_emitter = True):
    return load_string("""
        <shape version='2.0.0' type='ply'>
            <string name='filename' value='{filename}'/>
            {emitter}
            <transform name='to_world'>
                <translate x='10' y='-1' z='2'/>
            </transform>
        </shape>
    """.format(filename=filename,
               emitter="<emitter type='area'><spectrum name='radiance' value='1'/></emitter>" if has_emitter else ""))

def test01_area_construct():
    e = load_string("""<emitter version="2.0.0" type="area">
            <spectrum name="radiance" value="1000"/>
        </emitter>""")
    assert e is not None

    shape = example_shape()
    assert e is not None
    e = shape.emitter()
    ref_shape = example_shape(has_emitter=False)
    assert e.bbox() == ref_shape.bbox()

    with pytest.raises(RuntimeError):
        # Should not allow specifying a to_world transform directly in the emitter.
        e = load_string("""<emitter version="2.0.0" type="area">
                <transform name="to_world">
                    <translate x="5"/>
                </transform>
            </emitter>""")

def test02_area_sample_direction():
    shape = example_shape()
    e = shape.emitter()
    # Direction sampling is conditioned on a sampled position
    it = Interaction3f()
    it.wavelengths = [1, 1, 1]
    it.p = [-5, 3, -1] # Some position
    it.time = 1.0

    sample = np.array([0.5, 0.5])
    local = warp.square_to_cosine_hemisphere(sample)

    (d_rec, res) = e.sample_direction(it, sample)
    d = (d_rec.p - it.p) / np.linalg.norm(d_rec.p - it.p)

    assert np.all(res > 0)
    assert np.allclose(d_rec.d, d)
    assert d_rec.pdf > 1.0

@fresolver_append_path
def test03_area_sample_ray():
    from mitsuba.scalar_rgb.core import Frame3f

    try:
        from mitsuba.scalar_spectral.core import MTS_WAVELENGTH_SAMPLES
        from mitsuba.scalar_spectral.core.xml import load_string as load_string_spectral
        from mitsuba.scalar_spectral.render import SurfaceInteraction3f as SurfaceInteraction3fS
    except ImportError:
        pytest.skip("scalar_spectral mode not enabled")

    shape = load_string_spectral("""
                <shape version='2.0.0' type='ply'>
                    <string name='filename' value='{filename}'/>
                    {emitter}
                    <transform name='to_world'>
                        <translate x='10' y='-1' z='2'/>
                    </transform>
                </shape>
            """.format(filename="data/triangle.ply",
                    emitter="<emitter type='area'><spectrum name='radiance' value='1'/></emitter>"))

    e = shape.emitter()

    radiance = load_string_spectral("<spectrum type='d65' version='2.0.0'/>").expand()[0]
    # Shifted wavelength sample
    wav_sample = 0.44 + np.arange(MTS_WAVELENGTH_SAMPLES) / float(MTS_WAVELENGTH_SAMPLES)
    wav_sample[wav_sample >= 1.0] -= 1.0

    (wavs, wav_weight) = radiance.sample(SurfaceInteraction3fS(), wav_sample)

    (ray, weight) = e.sample_ray(time=0.98, sample1=wav_sample[0],
                                 sample2=[0.4, 0.6], sample3=[0.1, 0.2])
    assert np.allclose(ray.time, 0.98)
    assert np.allclose(ray.wavelengths, wavs)
    # Position on the light source
    assert np.allclose(ray.o, [10, -0.53524196, 2.22540331])
    # Direction pointing out from the light source, in world coordinates.
    warped = warp.square_to_cosine_hemisphere([0.1, 0.2])
    assert np.allclose(ray.d, Frame3f([-1, 0, 0]).to_world(warped))
    assert np.allclose(weight, wav_weight * shape.surface_area() * Pi)

@fresolver_append_path
def example_shape_vec(filename = "data/triangle.ply", has_emitter = True):
    try:
        from mitsuba.packet_rgb.core.xml import load_string as load_string_packet
    except ImportError:
        pass

    return load_string_packet("""
        <shape version='2.0.0' type='ply'>
            <string name='filename' value='{filename}'/>
            {emitter}
            <transform name='to_world'>
                <translate x='10' y='-1' z='2'/>
            </transform>
        </shape>
    """.format(filename=filename,
               emitter="<emitter type='area'><spectrum name='radiance' value='1'/></emitter>" if has_emitter else ""))

def test04_area_sample_direction_vec():
    try:
        from mitsuba.packet_rgb.render import Interaction3f as Interaction3fX
        from mitsuba.packet_rgb.core import PacketSize
    except ImportError:
        pytest.skip("packet_rgb mode not enabled")

    shape = example_shape_vec()
    e = shape.emitter([True] * PacketSize) # TODO this shouldn't be needed

    # Direction sampling is conditioned on a sampled position
    it = Interaction3fX()
    it.wavelengths = [[1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1]]
    it.p = [[-5, 3, -1], [0, 3, -1], [-5, 0, -1], [-5, 3, 0]] # Some positions
    it.time = [1.0, 1.0, 1.0, 1.0]

    sample = np.array([[0.5, 0.5], [0.5, 0.5], [0.5, 0.5], [0.5, 0.5]])

    (d_rec, res) = e.sample_direction(it, sample)
    d = [(d_rec.p[i] - it.p[i]) / np.linalg.norm(d_rec.p[i] - it.p[i]) for i in range(len(it.time))]

    assert np.all(res > 0)
    assert np.allclose(d_rec.d, d)
    assert np.all(d_rec.pdf > 1.0)
