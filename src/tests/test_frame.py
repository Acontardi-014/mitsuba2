import unittest
from mitsuba import Frame

class FrameTest(unittest.TestCase):
    def assertVectorsEqual(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
            self.assertAlmostEqual(a[i], b[i], places=6)

    def test01_construction(self):
        # Uninitialized frame
        _ = Frame()

        # Frame from the 3 vectors: no normalization should be performed
        f = Frame([0.005, 50, -6], [0.01, -13.37, 1], [0.5, 0, -6.2])
        self.assertVectorsEqual(f.s, [0.005, 50, -6])
        self.assertVectorsEqual(f.t, [0.01, -13.37, 1])
        self.assertVectorsEqual(f.n, [0.5, 0, -6.2])

        # Frame from the Normal component only
        f = Frame([0, 0, 1])
        self.assertVectorsEqual(f.s, [1, 0, 0])
        self.assertVectorsEqual(f.t, [0, 1, 0])
        self.assertVectorsEqual(f.n, [0, 0, 1])


    def test02_unit_frame(self):
        f = Frame([1, 0, 0], [0, 1, 0], [0, 0, 1])
        v = [0.5, 2.7, -3.12]
        lv = f.toLocal(v)

        self.assertVectorsEqual(lv, v)
        self.assertVectorsEqual(f.toWorld(v), v)
        self.assertAlmostEqual(f.cosTheta2(lv), v[2] * v[2], places=5)
        self.assertAlmostEqual(f.cosTheta(lv), v[2], places=5)
        self.assertAlmostEqual(f.sinTheta2(lv), 1 - f.cosTheta2(v), places=5)

        self.assertVectorsEqual(f.uv(f.toLocal(v)), v[0:2])

    def test03_frame_equality(self):
        f1 = Frame([1, 0, 0], [0, 1, 0], [0, 0, 1])
        f2 = Frame([0, 0, 1])
        f3 = Frame([0, 0, 1], [0, 1, 0], [1, 0, 0])

        self.assertEqual(f1, f2)
        self.assertEqual(f2, f1)
        self.assertNotEqual(f1, f3)
        self.assertNotEqual(f2, f3)

if __name__ == '__main__':
    unittest.main()
