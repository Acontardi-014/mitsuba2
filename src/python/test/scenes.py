"""Test fixtures containing common scenes."""
import pytest
from mitsuba.core.xml import load_string
from mitsuba.test.util import fresolver_append_path

@pytest.fixture
def empty_scene():
    return make_empty_scene()
def make_empty_scene():
    scene = load_string("""
        <scene version='2.0.0'>
            <sensor type="perspective">
                <film type="hdrfilm">
                    <integer name="width" value="151"/>
                    <integer name="height" value="146"/>
                </film>

                <sampler type="independent"/>
            </sensor>
        </scene>
    """)
    assert scene is not None
    return scene

@pytest.fixture
def teapot_scene():
    return make_teapot_scene()
@fresolver_append_path
def make_teapot_scene(spp = 16):
    scene = load_string("""
        <scene version='2.0.0'>
            <sensor type="perspective">
                <float name="near_clip" value="1"/>
                <float name="far_clip" value="1000"/>

                <transform name="to_world">
                    <lookat target="0.0, 0.0, 0.2"
                            origin="1.0, -12.0, 2"
                            up    ="0.0, 0.0, 1.0"/>
                </transform>

                <film type="hdrfilm">
                    <integer name="width" value="100"/>
                    <integer name="height" value="72"/>
                </film>

                <sampler type="independent">
                    <integer name="sample_count" value="{spp}"/>
                </sampler>
            </sensor>

            <shape type="ply">
                <string name="filename"
                        value="resources/data/ply/teapot.ply"/>

                <bsdf type="diffuse">
                    <rgb name="reflectance" value="0.1f, 0.1f, 0.8f"/>
                </bsdf>
            </shape>

            <emitter type="point">
                <point name="position" x="2.0" y="-6.0" z="4.5"/>
                <spectrum name="intensity" value="300.0f"/>
            </emitter>
            <emitter type="point">
                <point name="position" x="-3.0" y="-3.0" z="-0.5"/>
                <spectrum name="intensity" value="100.0f"/>
            </emitter>
        </scene>
    """.format(spp=spp))
    assert scene is not None
    return scene

@pytest.fixture
def box_scene():
    return make_box_scene()
@fresolver_append_path
def make_box_scene(spp = 16):
    scene = load_string("""
        <scene version='2.0.0'>
            <integrator type="path"/>

            <sensor type="perspective">
                <float name="near_clip" value="1"/>
                <float name="far_clip" value="1000"/>

                <transform name="to_world">
                    <lookat target="0.0,   0.0, 3.5"
                            origin="0.0, -14.0, 3.0"
                            up    ="0.0,   0.0, 1.0"/>
                </transform>

                <film type="hdrfilm">
                    <integer name="width" value="100"/>
                    <integer name="height" value="100"/>
                    <string name="pixel_format" value="rgb" />
                </film>

                <sampler type="independent">
                    <integer name="sample_count" value="{spp}"/>
                </sampler>
            </sensor>

            <bsdf type="diffuse" id="neutral">
                <rgb name="reflectance" value="1.0f, 1.0f, 1.0f"/>
            </bsdf>

            <shape type="ply">
                <string name="filename" value="resources/data/ply/teapot.ply"/>
                <transform name="to_world">
                    <translate x="0" y="0" z="0.15"/>
                </transform>
                <ref id="neutral"/>
            </shape>

            <shape type="rectangle"> <!-- Bottom -->
                <transform name="to_world">
                    <scale x="5" y="5" z="5"/>
                </transform>
                <ref id="neutral"/>
            </shape>

            <shape type="rectangle">  <!-- Left -->
                <transform name="to_world">
                    <scale x="5" y="5" z="5"/>
                    <rotate x="0" y="1" z="0" angle="90"/>
                    <translate x="-5" y="0" z="5"/>
                </transform>
                <bsdf type="diffuse">
                    <rgb name="reflectance" value="0.61, 0.53, 1.00"/>
                </bsdf>
            </shape>

            <shape type="rectangle">  <!-- Back -->
                <transform name="to_world">
                    <scale x="5" y="5" z="5"/>
                    <rotate x="1" y="0" z="0" angle="90"/>
                    <translate x="0" y="5" z="5"/>
                </transform>
                <ref id="neutral"/>
            </shape>

            <shape type="rectangle">  <!-- Right -->
                <transform name="to_world">
                    <scale x="5" y="5" z="5"/>
                    <rotate x="0" y="1" z="0" angle="-90"/>
                    <translate x="5" y="0" z="5"/>
                </transform>
                <bsdf type="diffuse">
                    <rgb name="reflectance" value="0.98, 0.77, 0.19"/>
                </bsdf>
            </shape>

            <shape type="rectangle">  <!-- Top -->
                <transform name="to_world">
                    <scale x="5" y="5" z="5"/>
                    <rotate x="0" y="1" z="0" angle="180"/>
                    <translate x="0" y="0" z="10"/>
                </transform>
                <ref id="neutral"/>
            </shape>

            <shape type="rectangle">  <!-- Small emitter -->
                <transform name="to_world">
                    <scale x="3" y="3" z="3"/>
                    <rotate x="0" y="1" z="0" angle="180"/>
                    <translate x="0" y="0" z="9"/>
                </transform>
                <ref id="neutral"/>

                <emitter type="area">
                    <spectrum name="radiance" value="3.0f"/>
                </emitter>
            </shape>
        </scene>
    """.format(spp=spp))
    assert scene is not None
    return scene

@pytest.fixture
def museum_plane_scene():
    return make_museum_plane_scene()
@fresolver_append_path
def make_museum_plane_scene(spp = 16, roughness = 0.01):
    scene = load_string("""
        <scene version="2.0.0">
            <sensor type="perspective">
                <float name="fov" value="50"/>
                <float name="near_clip" value="0.1"/>
                <float name="far_clip" value="1000.0"/>
                <transform name="to_world">
                    <lookat origin="0, 26, 0"
                            target="0.01, 0, 0"
                            up    ="0, 1, 0" />
                </transform>

                <sampler type="independent">
                    <integer name="sample_count" value="{spp}" />
                </sampler>

                <film type="hdrfilm">
                    <string name="pixel_format" value="rgb" />
                    <integer name="width" value="128"/>
                    <integer name="height" value="128"/>
                    <rfilter type="box" />
                </film>
            </sensor>

            <bsdf type="roughconductor" id="glossy">
                <float name="alpha" value="{roughness}"/>
                <rgb name="eta" value="0.8, 1.3, 1.4"/>
                <rgb name="k" value="1.0, 1.0, 1.0"/>
            </bsdf>

            <shape type="rectangle">
                <transform name="to_world">
                    <scale x="10" y="10" z="10"/>
                    <lookat origin="0,   0, 0"
                            target="0,   1, 0"
                            up    ="0,   0, 1" />
                </transform>
                <ref id="glossy"/>
            </shape>

            <emitter type="envmap">
                <string name="filename" value="resources/data/envmap/museum.exr"/>
                <transform name="to_world">
                    <rotate x="0" y="0" z="1" angle="90"/>
                    <rotate x="1" y="0" z="0" angle="90"/>
                    <rotate x="0" y="1" z="0" angle="180"/>
                </transform>
            </emitter>
        </scene>
    """.format(spp=spp, roughness=roughness))
    assert scene is not None
    return scene

def make_integrator(kind, xml = ""):
    integrator = load_string("<integrator version='2.0.0' type='%s'>"
                             "%s</integrator>" % (kind, xml))
    assert integrator is not None
    return integrator
