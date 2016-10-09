from mitsuba.core import BoundingBox3f as BBox
import numpy as np


def test01_basics():
    bbox1 = BBox()
    bbox2 = BBox([0, 1, 2])
    bbox3 = BBox([1, 2, 3], [2, 3, 5])
    assert bbox1 != bbox2
    assert bbox2 == bbox2
    assert not bbox1.valid()
    assert not bbox1.collapsed()
    assert bbox2.valid()
    assert bbox2.collapsed()
    assert bbox3.valid()
    assert not bbox3.collapsed()
    assert bbox2.volume() == 0
    assert bbox2.majorAxis() == 0
    assert bbox2.minorAxis() == 0
    assert (bbox2.center() == [0, 1, 2]).all()
    assert (bbox2.extents() == [0, 0, 0]).all()
    assert bbox2.surfaceArea() == 0
    assert bbox3.volume() == 2
    assert bbox3.surfaceArea() == 10
    assert (bbox3.center() == [1.5, 2.5, 4]).all()
    assert (bbox3.extents() == [1, 1, 2]).all()
    assert bbox3.majorAxis() == 2
    assert bbox3.minorAxis() == 0
    assert (bbox3.min == [1, 2, 3]).all()
    assert (bbox3.max == [2, 3, 5]).all()
    assert (bbox3.corner(0) == [1, 2, 3]).all()
    assert (bbox3.corner(1) == [2, 2, 3]).all()
    assert (bbox3.corner(2) == [1, 3, 3]).all()
    assert (bbox3.corner(3) == [2, 3, 3]).all()
    assert (bbox3.corner(4) == [1, 2, 5]).all()
    assert (bbox3.corner(5) == [2, 2, 5]).all()
    assert (bbox3.corner(6) == [1, 3, 5]).all()
    assert (bbox3.corner(7) == [2, 3, 5]).all()
    assert str(bbox1) == "BoundingBox3[invalid]"
    assert str(bbox3) == "BoundingBox3[min = [1, 2, 3]," \
                         " max = [2, 3, 5]]"
    bbox4 = BBox.merge(bbox2, bbox3)
    assert (bbox4.min == [0, 1, 2]).all()
    assert (bbox4.max == [2, 3, 5]).all()
    bbox3.reset()
    assert not bbox3.valid()
    bbox3.expand([0, 0, 0])
    assert BBox([0, 0, 0]) == bbox3
    bbox3.expand([1, 1, 1])
    assert BBox([0, 0, 0], [1, 1, 1]) == bbox3
    bbox3.expand(BBox([-1, -2, -3], [4, 5, 6]))
    assert BBox([-1, -2, -3], [4, 5, 6]) == bbox3
    bbox3.clip(bbox2)
    assert bbox2 == bbox3


def test02_contains_variants():
    bbox = BBox([1, 2, 3], [2, 3, 5])
    assert bbox.contains([1.5, 2.5, 3.5])
    assert bbox.contains([1.5, 2.5, 3.5], strict=True)
    assert bbox.contains([1, 2, 3])
    assert not bbox.contains([1, 2, 3], strict=True)
    assert bbox.contains(BBox([1.5, 2.5, 3.5], [1.8, 2.8, 3.8]))
    assert bbox.contains(BBox([1.5, 2.5, 3.5], [1.8, 2.8, 3.8]),
                         strict=True)
    assert bbox.contains(BBox([1, 2, 3], [1.8, 2.8, 3.8]))
    assert not bbox.contains(BBox([1, 2, 3], [1.8, 2.8, 3.8]),
                             strict=True)
    assert bbox.overlaps(BBox([0, 1, 2], [1.5, 2.5, 3.5]))
    assert bbox.overlaps(BBox([0, 1, 2], [1.5, 2.5, 3.5]),
                         strict=True)
    assert bbox.overlaps(BBox([0, 1, 2], [1, 2, 3]))
    assert not bbox.overlaps(BBox([0, 1, 2], [1, 2, 3]),
                             strict=True)


def test03_distanceTo():
    assert BBox([1, 2, 3], [2, 3, 5]).distance(
        BBox([4, 2, 3], [5, 3, 5])) == 2

    assert np.abs(BBox([1, 2, 3], [2, 3, 5]).distance(
        BBox([3, 4, 6], [7, 7, 7])) - np.sqrt(3)) < 1e-6

    assert BBox([1, 2, 3], [2, 3, 5]).distance(
        BBox([1.1, 2.2, 3.3], [1.8, 2.8, 3.8])) == 0

    assert BBox([1, 2, 3], [2, 3, 5]).distance([1.5, 2.5, 3.5]) == 0

    assert BBox([1, 2, 3], [2, 3, 5]).distance([3, 2.5, 3.5]) == 1

    assert np.abs(BBox([1, 2, 3], [2, 3, 5]).distance(
        [3, 4, 6]) == np.sqrt(3)) < 1e-6
