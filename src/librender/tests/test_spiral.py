import numpy as np
import pytest

from mitsuba.scalar_rgb.core.xml import load_string
from mitsuba.scalar_rgb.render import Spiral

@pytest.fixture(scope="module")
def film():
    return make_film()
def make_film(width = 156, height = 232):
    f = load_string("""<film type="hdrfilm" version="2.0.0">
            <integer name="width" value="{}"/>
            <integer name="height" value="{}"/>
        </film>""".format(width, height))
    assert f is not None
    assert np.all(f.size() == [width, height])
    return f

def extract_blocks(spiral, max_blocks = 1000):
    blocks = []
    b = spiral.next_block()
    while np.prod(b[1]) > 0:
        blocks.append(b)
        b = spiral.next_block()

        assert len(blocks) <= max_blocks,\
               "Too many blocks produced, implementation is probably wrong."
    return blocks

def check_first_blocks(blocks, expected, n_total = None):
    n_total = n_total or len(expected)
    assert len(blocks) == n_total
    for i in range(len(expected)):
        assert np.all(blocks[i][0] == expected[i][0])
        assert np.all(blocks[i][1] == expected[i][1])


def test01_construct(film):
    s = Spiral(film)
    assert s is not None
    assert s.max_block_size() == 32

def test02_small_film():
    f = make_film(15, 12)
    s = Spiral(f)

    (bo, bs) = s.next_block()
    assert np.all(bo == [0, 0])
    assert np.all(bs == [15, 12])
    # The whole image is covered by a single block
    assert np.all(s.next_block()[1] == 0)


def test03_normal_film():
    # Check the first few blocks' size and location
    center = np.array([160, 160])
    w = 32
    expected = [
        [center               , (w, w)], [center + [ 1*w,  0*w], (w, w)],
        [center + [ 1*w,  1*w], (w, w)], [center + [ 0*w,  1*w], (w, w)],
        [center + [-1*w,  1*w], (w, w)], [center + [-1*w,  0*w], (w, w)],
        [center + [-1*w, -1*w], (w, w)], [center + [ 0*w, -1*w], (w, w)],
        [center + [ 1*w, -1*w], (w, w)], [center + [ 2*w, -1*w], (w, w)],
        [center + [ 2*w,  0*w], (w, w)], [center + [ 2*w,  1*w], (w, w)]
    ]

    f = make_film(318, 322)
    s = Spiral(f)

    check_first_blocks(extract_blocks(s), expected, n_total=110)
    # Resetting and re-querying the blocks should yield the exact same results.
    s.reset()
    check_first_blocks(extract_blocks(s), expected, n_total=110)
