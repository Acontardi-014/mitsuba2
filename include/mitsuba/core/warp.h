#pragma once

#include <mitsuba/core/fwd.h>
#include <mitsuba/core/frame.h>
#include <mitsuba/core/math.h>
#include <mitsuba/core/vector.h>
#include <Eigen/Core>

namespace {
  mitsuba::Float kDomainEpsilon = 1e-5;
}  // end anonymous namespace

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Implements common warping techniques that map from the unit
 * square [0, 1]^2 to other domains such as spheres, hemispheres, etc.
 *
 * The main application of this class is to generate uniformly
 * distributed or weighted point sets in certain common target domains.
 */
NAMESPACE_BEGIN(warp)

class WarpAdapter;

// TODO: proper support for:
// - uniformDiskToSquareConcentric  (starting sample != uniform square)
// - intervalToNonuniformTent       (float to float)

/// Enum of available warping types
// TODO: shouldn't be needed anymore
enum WarpType {
    NoWarp = 0,
    UniformSphere,
    UniformHemisphere,
    CosineHemisphere,
    UniformCone,
    UniformDisk,
    UniformDiskConcentric,
    UniformTriangle,
    StandardNormal,
    UniformTent,
    NonUniformTent
};

/// Enum of available point sampling strategies
enum SamplingType {
    Independent = 0,
    Grid,
    Stratified
};

// =============================================================
//! @{ \name Warping techniques related to spheres and subsets
// =============================================================

/// Uniformly sample a vector on the unit sphere with respect to solid angles
extern MTS_EXPORT_CORE Vector3f squareToUniformSphere(const Point2f &sample);

/// Density of \ref squareToUniformSphere() with respect to solid angles
inline MTS_EXPORT_CORE Float squareToUniformSpherePdf(const Vector3f &v) {
    if (std::abs(v.x() * v.x() + v.y() * v.y() + v.z() * v.z() - 1) < kDomainEpsilon)
        return math::InvFourPi;
    return 0.0;
}

/// Uniformly sample a vector on the unit hemisphere with respect to solid angles
extern MTS_EXPORT_CORE Vector3f squareToUniformHemisphere(const Point2f &sample);

/// Density of \ref squareToUniformHemisphere() with respect to solid angles
inline MTS_EXPORT_CORE Float squareToUniformHemispherePdf(const Vector3f &v) {
    if (std::abs(v.x() * v.x() + v.y() * v.y() + v.z() * v.z() - 1) < kDomainEpsilon
        && (v.z() >= 0))
        return math::InvTwoPi;
    return 0.0;
}

/// Sample a cosine-weighted vector on the unit hemisphere with respect to solid angles
extern MTS_EXPORT_CORE Vector3f squareToCosineHemisphere(const Point2f &sample);

/// Density of \ref squareToCosineHemisphere() with respect to solid angles
inline MTS_EXPORT_CORE Float squareToCosineHemispherePdf(const Vector3f &v) {
    if (std::abs(v.x() * v.x() + v.y() * v.y() + v.z() * v.z() - 1) < kDomainEpsilon
        && (v.z() >= 0))
        return math::InvPi * Frame::cosTheta(v);
    return 0.0;
}

/**
 * \brief Uniformly sample a vector that lies within a given
 * cone of angles around the Z axis
 *
 * \param cosCutoff Cosine of the cutoff angle
 * \param sample A uniformly distributed sample on \f$[0,1]^2\f$
 */
extern MTS_EXPORT_CORE Vector3f squareToUniformCone(const Point2f &sample, Float cosCutoff);

/**
 * \brief Density of \ref squareToUniformCone per unit area.
 *
 * \param cosCutoff Cosine of the cutoff angle
 */
inline MTS_EXPORT_CORE Float squareToUniformConePdf(const Vector3f &v, Float cosCutoff) {
    if (std::abs(v.x() * v.x() + v.y() * v.y() + v.z() * v.z() - 1) < kDomainEpsilon
        && Frame::cosTheta(v) - cosCutoff >= kDomainEpsilon)
        return math::InvTwoPi / (1 - cosCutoff);
    return 0.0;
}

//! @}
// =============================================================

// =============================================================
//! @{ \name Warping techniques that operate in the plane
// =============================================================

/// Uniformly sample a vector on a 2D disk
extern MTS_EXPORT_CORE Point2f squareToUniformDisk(const Point2f &sample);

/// Density of \ref squareToUniformDisk per unit area
inline MTS_EXPORT_CORE Float squareToUniformDiskPdf(const Point2f &p) {
    if (p.x() * p.x() + p[1] * p[1] <= 1)
        return math::InvPi;
    return 0.0;
}

/// Low-distortion concentric square to disk mapping by Peter Shirley (PDF: 1/PI)
extern MTS_EXPORT_CORE Point2f squareToUniformDiskConcentric(const Point2f &sample);

/// Density of \ref squareToUniformDisk per unit area
inline MTS_EXPORT_CORE Float squareToUniformDiskConcentricPdf(const Point2f &p) {
    if (p.x() * p.x() + p[1] * p[1] <= 1)
        return math::InvPi;
    return 0.0;
}

/// Inverse of the mapping \ref squareToUniformDiskConcentric
extern MTS_EXPORT_CORE Point2f uniformDiskToSquareConcentric(const Point2f &p);

inline MTS_EXPORT_CORE Point2f uniformDiskToSquareConcentricPdf(const Point2f &p) {
    // TODO: verify this is correct
    if (p.x() >= 0 && p.x() <= 1 && p[1] >= 0 && p[1] <= 1)
        return 1.0;
    return 0.0;
}

/// Convert an uniformly distributed square sample into barycentric coordinates
extern MTS_EXPORT_CORE Point2f squareToUniformTriangle(const Point2f &sample);

/// Density of \ref squareToUniformTriangle per unit area.
inline MTS_EXPORT_CORE Float squareToUniformTrianglePdf(const Point2f &p) {
    if (p.x() >= 0 && p[1] >= 0 && p.x() <= 1 && p[1] <= 1
        && p.x() + p[1] <= 1)
        return 0.5;
    return 0.0;
}

/** \brief Sample a point on a 2D standard normal distribution
 * Internally uses the Box-Muller transformation */
extern MTS_EXPORT_CORE Point2f squareToStdNormal(const Point2f &sample);

/// Density of \ref squareToStdNormal per unit area
extern MTS_EXPORT_CORE Float squareToStdNormalPdf(const Point2f &p);

/// Warp a uniformly distributed square sample to a 2D tent distribution
extern MTS_EXPORT_CORE Point2f squareToTent(const Point2f &sample);

/// Density of \ref squareToTent per unit area.
extern MTS_EXPORT_CORE Float squareToTentPdf(const Point2f &p);

/** \brief Warp a uniformly distributed sample on [0, 1] to a nonuniform
 * tent distribution with nodes <tt>{a, b, c}</tt>
 */
extern MTS_EXPORT_CORE Float intervalToNonuniformTent(Float sample, Float a, Float b, Float c);

//! @}
// =============================================================

// =============================================================
//! @{ \name Extra functions related to sampling and testing
//!    of those warping functions.
// =============================================================
NAMESPACE_BEGIN(detail)

/**
 * For a given warping method captured by the passed \p warpAdater, and for
 * a given sampling strategy, runs a Chi^2  statistical test to check that the
 * warping function matches its associated PDF.
 * Also outputs the observed and expected histograms computed for the test.
 *
 * \return Pair (whether the test succeeded, an explanatory text).
 */
extern MTS_EXPORT_CORE std::pair<bool, std::string>
runStatisticalTestAndOutput(size_t pointCount, size_t gridWidth, size_t gridHeight,
    SamplingType samplingType, WarpAdapter *warpAdapter,
    double minExpFrequency, double significanceLevel,
    std::vector<double> &observedHistogram, std::vector<double> &expectedHistogram);

/**
 * For a given warping method captured by the passed \p warpAdater, and for
 * a given sampling strategy, runs a Chi^2 statistical test to check that
 * the warping function matches its associated PDF.
 *
 * \return Pair (whether the test succeeded, an explanatory text).
 */
inline MTS_EXPORT_CORE std::pair<bool, std::string>
runStatisticalTest(size_t pointCount, size_t gridWidth, size_t gridHeight,
                   SamplingType samplingType, WarpAdapter *warpAdapter,
                   double minExpFrequency, double significanceLevel) {
    std::vector<double> observedHistogram, expectedHistogram;
    return runStatisticalTestAndOutput(pointCount,
        gridWidth, gridHeight, samplingType, warpAdapter,
        minExpFrequency, significanceLevel, observedHistogram, expectedHistogram);
}

NAMESPACE_END(detail)
//! @}
// =============================================================

NAMESPACE_END(warp)
NAMESPACE_END(mitsuba)
