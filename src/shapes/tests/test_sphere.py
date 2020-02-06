import mitsuba
import pytest
import enoki as ek
from enoki.dynamic import Float32 as Float

from mitsuba.python.test import variant_scalar


def example_sphere(radius = 1.0):
    from mitsuba.core.xml import load_string

    return load_string("""<shape version='2.0.0' type='sphere'>
        <float name="radius" value="{}"/>
    </shape>""".format(radius))

def example_scene(radius = 1.0, extra = ""):
    from mitsuba.core.xml import load_string

    return load_string("""<scene version='2.0.0'>
        <shape version='2.0.0' type='sphere'>
            <float name="radius" value="{}"/>
            {}
        </shape>
    </scene>""".format(radius, extra))


def test01_create(variant_scalar):
    if mitsuba.core.MTS_ENABLE_EMBREE:
        pytest.skip("EMBREE enabled")

    s = example_sphere()
    assert s is not None
    assert s.primitive_count() == 1
    assert ek.allclose(s.surface_area(), 4 * ek.pi)


def test02_bbox(variant_scalar):
    if mitsuba.core.MTS_ENABLE_EMBREE:
        pytest.skip("EMBREE enabled")

    for r in [1, 2, 4]:
        s = example_sphere(r)
        b = s.bbox()

        assert b.valid()
        assert ek.allclose(b.center(), [0, 0, 0])
        assert ek.all(b.min == -r)
        assert ek.all(b.max == r)
        assert ek.allclose(b.extents(), [2 * r] * 3)


def test03_ray_intersect_transform(variant_scalar):
    if mitsuba.core.MTS_ENABLE_EMBREE:
        pytest.skip("EMBREE enabled")

    from mitsuba.core import Ray3f

    for r in [1, 3]:
        s = example_scene(radius=r,
                          extra="""<transform name="to_world">
                                       <rotate y="1.0" angle="30"/>
                                       <translate x="0.0" y="1.0" z="0.0"/>
                                   </transform>""")
        # grid size
        n = 21
        inv_n = 1.0 / n

        for x in range(n):
            for y in range(n):
                x_coord = r * (2 * (x * inv_n) - 1)
                y_coord = r * (2 * (y * inv_n) - 1)

                ray = Ray3f(o=[x_coord, y_coord + 1, -8], d=[0.0, 0.0, 1.0],
                            time=0.0, wavelengths=[])
                si_found = s.ray_test(ray)

                assert si_found == (x_coord ** 2 + y_coord ** 2 <= r * r) \
                    or ek.abs(x_coord ** 2 + y_coord ** 2 - r * r) < 1e-8

                if si_found:
                    ray = Ray3f(o=[x_coord, y_coord + 1, -8], d=[0.0, 0.0, 1.0],
                                time=0.0, wavelengths=[])
                    si = s.ray_intersect(ray)
                    ray_u = Ray3f(ray)
                    ray_v = Ray3f(ray)
                    eps = 1e-4
                    ray_u.o += si.dp_du * eps
                    ray_v.o += si.dp_dv * eps
                    si_u = s.ray_intersect(ray_u)
                    si_v = s.ray_intersect(ray_v)
                    if si_u.is_valid():
                        du = (si_u.uv - si.uv) / eps
                        assert ek.allclose(du, [1, 0], atol=2e-2)
                    if si_v.is_valid():
                        dv = (si_v.uv - si.uv) / eps
                        assert ek.allclose(dv, [0, 1], atol=2e-2)


def test04_sample_direct(variant_scalar):
    from mitsuba.core.xml import load_string
    from mitsuba.core import Ray3f
    from mitsuba.render import Interaction3f

    if mitsuba.core.MTS_ENABLE_EMBREE:
        pytest.skip("EMBREE enabled")

    sphere = load_string('<shape type="sphere" version="2.0.0"/>')

    def sample_cone(sample, cos_theta_max):
        cos_theta = (1 - sample[1]) + sample[1] * cos_theta_max
        sin_theta = ek.sqrt(1 - cos_theta * cos_theta)
        phi = 2 * ek.pi * sample[0]
        s, c = ek.sin(phi), ek.cos(phi)
        return [c * sin_theta, s * sin_theta, cos_theta]

    it = Interaction3f()
    it.p = [0, 0, -3]
    it.t = 0
    sin_cone_angle = 1.0 / it.p[2]
    cos_cone_angle = ek.sqrt(1 - sin_cone_angle**2)

    for xi_1 in ek.linspace(Float, 0, 1, 10):
        for xi_2 in ek.linspace(Float, 1e-3, 1 - 1e-3, 10):
            sample = sphere.sample_direction(it, [xi_2, 1 - xi_1])
            d = sample_cone([xi_1, xi_2], cos_cone_angle)
            its = sphere.ray_intersect(Ray3f(it.p, d, 0, []))
            assert ek.allclose(d, sample.d, atol=1e-5, rtol=1e-5)
            assert ek.allclose(its.t, sample.dist, atol=1e-5, rtol=1e-5)
            assert ek.allclose(its.p, sample.p, atol=1e-5, rtol=1e-5)
