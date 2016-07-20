try:
    import unittest2 as unittest
except:
    import unittest

from mitsuba.core import math, BoundingBox3f
from mitsuba.core.warp import *

class WarpTest(unittest.TestCase):
    # Statistical tests parameters
    # TODO: consider fixing seed
    minExpFrequency = 5
    significanceLevel = 0.01

    def test01_pdfs(self):
        p = [0.5, 0.25]
        unit = [0.0, 1.0, 0.0]
        ten = [10.0, 10.0, 10.0]
        zero2D = [0.0, 0.0]
        ten2D = [10.0, 10.0]

        # The PDF functions are > 0 inside of the warping function's target
        # domain, null outide.
        self.assertAlmostEqual(squareToUniformSpherePdf(unit), math.InvFourPi, places = 6)
        self.assertAlmostEqual(squareToUniformSpherePdf(ten), 0, places = 6)

        self.assertAlmostEqual(squareToUniformHemispherePdf(unit), math.InvTwoPi, places = 6)
        self.assertAlmostEqual(squareToUniformHemispherePdf(ten), 0, places = 6)

        self.assertAlmostEqual(squareToUniformConePdf([0.0, 0.0, 1.0], 0.5), math.InvTwoPi / 0.5, places = 6)
        self.assertAlmostEqual(squareToUniformConePdf(unit, 0.5), 0, places = 6)
        self.assertAlmostEqual(squareToUniformConePdf(ten, 0.5), 0, places = 6)

        self.assertAlmostEqual(squareToUniformDiskPdf(zero2D), math.InvPi, places = 6)
        self.assertAlmostEqual(squareToUniformDiskPdf(ten2D), 0, places = 6)

        self.assertAlmostEqual(squareToUniformDiskConcentricPdf(zero2D), math.InvPi, places = 6)
        self.assertAlmostEqual(squareToUniformDiskConcentricPdf(ten2D), 0, places = 6)

        self.assertAlmostEqual(squareToStdNormalPdf(zero2D), math.InvTwoPi, places = 6)
        self.assertTrue(squareToStdNormalPdf(ten2D) < 0.0001)

        self.assertAlmostEqual(squareToTentPdf(zero2D), 1, places = 6)
        self.assertAlmostEqual(squareToTentPdf(ten2D), 0, places = 6)

        # Just checking that these are not crashing, the actual results
        # are tested statistically.
        _ = squareToUniformSphere(p)
        _ = squareToUniformHemisphere(p)
        _ = squareToCosineHemisphere(p)
        _ = squareToUniformCone(p, 0.5)
        _ = squareToUniformDisk(p)
        _ = squareToUniformDiskConcentric(p)
        _ = uniformDiskToSquareConcentric(p)
        _ = squareToUniformTriangle(p)
        _ = squareToStdNormal(p)
        _ = squareToStdNormalPdf(p)
        _ = squareToTent(p)
        _ = squareToTentPdf(p)
        _ = intervalToNonuniformTent(0.25, 0.5, 1.0, 0.75)

    def test02_indentity(self):
        w = IdentityWarpAdapter()
        self.assertTrue(w.isIdentity())
        self.assertEqual(w.inputDimensionality(), 2)
        self.assertEqual(w.domainDimensionality(), 2)
        p1 = [0.5, 0.3]
        (p2, weight) = w.warpSample(p1)
        self.assertAlmostEqual(weight, 1.0, places=6)
        self.assertAlmostEqual(p1[0], p2[0], places=6)
        self.assertAlmostEqual(p1[1], p2[1], places=6)
        self.assertEqual(w.name(), "Identity")

    def test03_statistical_tests(self):
        def warpWithUnitWeight(f):
            return lambda p: (f(p), 1.0)

        def constantValue(v): #, length as parameter
            length = 4
            if (v >= 0) and (v <= length):
                return 1 / float(length)
            return 0

        # TODO: use the "warp factory" Python class (currently residing in `warp_visualize.py`)
        warps = [
            # Identity warping (no-op)
            IdentityWarpAdapter(),

            # 2D -> 2D warps
            PlaneWarpAdapter("Square to uniform disk",
                warpWithUnitWeight(squareToUniformDisk),
                squareToUniformDiskPdf),
            PlaneWarpAdapter("Square to uniform disk concentric",
                warpWithUnitWeight(squareToUniformDiskConcentric),
                squareToUniformDiskConcentricPdf),
            PlaneWarpAdapter("Square to uniform triangle",
                warpWithUnitWeight(squareToUniformTriangle),
                squareToUniformTrianglePdf,
                bbox = WarpAdapter.kUnitSquareBoundingBox),
            PlaneWarpAdapter("Square to tent",
                warpWithUnitWeight(squareToTent),
                squareToTentPdf),

            # TODO: manage the case of infinite support (need inverse mapping?)
            PlaneWarpAdapter("Square to 2D gaussian",
                warpWithUnitWeight(squareToStdNormal),
                squareToStdNormalPdf,
                bbox = BoundingBox3f([-5, -5, -5], [5, 5, 5])),

            # 2D -> 3D warps
            SphereWarpAdapter("Square to uniform sphere",
                warpWithUnitWeight(squareToUniformSphere),
                squareToUniformSpherePdf),
            SphereWarpAdapter("Square to uniform hemisphere",
                warpWithUnitWeight(squareToUniformHemisphere),
                squareToUniformHemispherePdf),
            SphereWarpAdapter("Square to cosine hemisphere",
                warpWithUnitWeight(squareToCosineHemisphere),
                squareToCosineHemispherePdf),
            SphereWarpAdapter("Square to uniform cone",
                warpWithUnitWeight(squareToUniformCone),
                squareToUniformConePdf,
                [WarpAdapter.Argument("cosCutoff", -1, 1)]),

            # 1D -> 1D warps (simple test)
            LineWarpAdapter("Constant value",
                warpWithUnitWeight(constantValue),
                constantValue,
                [WarpAdapter.Argument("length", 0, 100)])
        ]

        # TODO: cover all sampling types
        samplingType = SamplingType.Independent
        # TODO: also cover several parameter values when relevant (argument range is specified)
        parameterValue = 0.5

        # TODO: increase sampling resolution and sample count if needed
        samplingResolution = 31
        (gridWidth, gridHeight) = (samplingResolution, samplingResolution)
        nBins = gridWidth * gridHeight
        sampleCount = 100 * nBins

        for warpAdapter in warps:
            with self.subTest("Warp: " + str(warpAdapter)):
                (result, reason) = runStatisticalTest(
                    sampleCount, gridWidth, gridHeight,
                    samplingType, warpAdapter,
                    WarpTest.minExpFrequency, WarpTest.significanceLevel)

                self.assertTrue(result, reason)

if __name__ == '__main__':
    unittest.main()
