#pragma once

#include <mitsuba/core/logger.h>
#include <mitsuba/core/simd.h>
#include <mitsuba/core/traits.h>
#include <cmath>
#include <algorithm>

NAMESPACE_BEGIN(mitsuba)
NAMESPACE_BEGIN(math)

// -----------------------------------------------------------------------
//! @{ \name Useful constants in various precisions
// -----------------------------------------------------------------------

static const double E_d               = 2.71828182845904523536;
static const double Pi_d              = 3.14159265358979323846;
static const double InvPi_d           = 0.31830988618379067154;
static const double InvTwoPi_d        = 0.15915494309189533577;
static const double InvFourPi_d       = 0.07957747154594766788;
static const double SqrtPi_d          = 1.77245385090551602793;
static const double InvSqrtPi_d       = 0.56418958354775628695;
static const double SqrtTwo_d         = 1.41421356237309504880;
static const double InvSqrtTwo_d      = 0.70710678118654752440;
static const double SqrtTwoPi_d       = 2.50662827463100050242;
static const double InvSqrtTwoPi_d    = 0.39894228040143267794;
static const double Epsilon_d         = 1e-7;
#if defined(MTS_CPP17)
static const double OneMinusEpsilon_d = 0x1.fffffffffffffp-1;
static const double RecipOverflow_d   = 0x1p-1024;
#else
static const double OneMinusEpsilon_d = 0.999999999999999888;
static const double RecipOverflow_d   = 5.56268464626800345e-309;
#endif
static const double Infinity_d        = std::numeric_limits<double>::infinity();
static const double MaxFloat_d        = std::numeric_limits<double>::max();
static const double MachineEpsilon_d  = std::numeric_limits<double>::epsilon() / 2;

static const float  E_f               = (float) E_d;
static const float  Pi_f              = (float) Pi_d;
static const float  InvPi_f           = (float) InvPi_d;
static const float  InvTwoPi_f        = (float) InvTwoPi_d;
static const float  InvFourPi_f       = (float) InvFourPi_d;
static const float  SqrtPi_f          = (float) SqrtPi_d;
static const float  InvSqrtPi_f       = (float) InvSqrtPi_d;
static const float  SqrtTwo_f         = (float) SqrtTwo_d;
static const float  InvSqrtTwo_f      = (float) InvSqrtTwo_d;
static const float  SqrtTwoPi_f       = (float) SqrtTwoPi_d;
static const float  InvSqrtTwoPi_f    = (float) InvSqrtTwoPi_d;
static const float  Epsilon_f         = 1e-4f;
#if defined(MTS_CPP17)
static const float  OneMinusEpsilon_f = 0x1.fffffep-1f;
static const float  RecipOverflow_f   = 0x1p-128f;
#else
static const float  OneMinusEpsilon_f = 0.999999940395355225f;
static const float  RecipOverflow_f   = 2.93873587705571876e-39f;
#endif
static const float  MaxFloat_f        = std::numeric_limits<float>::max();
static const float  Infinity_f        = std::numeric_limits<float>::infinity();
static const float  MachineEpsilon_f  = std::numeric_limits<float>::epsilon() / 2;

static const Float  E                 = (Float) E_d;
static const Float  Pi                = (Float) Pi_d;
static const Float  InvPi             = (Float) InvPi_d;
static const Float  InvTwoPi          = (Float) InvTwoPi_d;
static const Float  InvFourPi         = (Float) InvFourPi_d;
static const Float  SqrtPi            = (Float) SqrtPi_d;
static const Float  InvSqrtPi         = (Float) InvSqrtPi_d;
static const Float  SqrtTwo           = (Float) SqrtTwo_d;
static const Float  InvSqrtTwo        = (Float) InvSqrtTwo_d;
static const Float  SqrtTwoPi         = (Float) SqrtTwoPi_d;
static const Float  InvSqrtTwoPi      = (Float) InvSqrtTwoPi_d;
static const Float  OneMinusEpsilon   = Float(sizeof(Float) == sizeof(double) ?
                                        OneMinusEpsilon_d : (double) OneMinusEpsilon_f);
static const Float  RecipOverflow     = Float(sizeof(Float) == sizeof(double) ?
                                        RecipOverflow_d : (double) RecipOverflow_f);
static const Float  Epsilon           = Float(sizeof(Float) == sizeof(double) ?
                                        Epsilon_d : (double) Epsilon_f);
static const Float  MaxFloat          = std::numeric_limits<Float>::max();
static const Float  Infinity          = std::numeric_limits<Float>::infinity();
static const Float  MachineEpsilon    = std::numeric_limits<Float>::epsilon() / 2;

//! @}
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
//! @{ \name Complete and incomplete elliptic integrals
//! Caution: the 'k' factor is squared in the elliptic integral, which
//! differs from the convention of Mathematica's EllipticK etc.
// -----------------------------------------------------------------------

/// Complete elliptic integral of the first kind (double precision)
extern MTS_EXPORT_CORE double comp_ellint_1(double k);
/// Complete elliptic integral of the second kind (double precision)
extern MTS_EXPORT_CORE double comp_ellint_2(double k);
/// Complete elliptic integral of the third kind (double precision)
extern MTS_EXPORT_CORE double comp_ellint_3(double k, double nu);
/// Incomplete elliptic integral of the first kind (double precision)
extern MTS_EXPORT_CORE double ellint_1(double k, double phi);
/// Incomplete elliptic integral of the second kind (double precision)
extern MTS_EXPORT_CORE double ellint_2(double k, double phi);
/// Incomplete elliptic integral of the third kind (double precision)
extern MTS_EXPORT_CORE double ellint_3(double k, double nu, double phi);

/// Complete elliptic integral of the first kind (single precision)
extern MTS_EXPORT_CORE float comp_ellint_1(float k);
/// Complete elliptic integral of the second kind (single precision)
extern MTS_EXPORT_CORE float comp_ellint_2(float k);
/// Complete elliptic integral of the third kind (single precision)
extern MTS_EXPORT_CORE float comp_ellint_3(float k, float nu);
/// Incomplete elliptic integral of the first kind (single precision)
extern MTS_EXPORT_CORE float ellint_1(float k, float phi);
/// Incomplete elliptic integral of the second kind (single precision)
extern MTS_EXPORT_CORE float ellint_2(float k, float phi);
/// Incomplete elliptic integral of the first kind (single precision)
extern MTS_EXPORT_CORE float ellint_3(float k, float nu, float phi);

//! @}
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
//! @{ \name Bessel functions
// -----------------------------------------------------------------------

/*
   A subset of cephes math routines
   Redistributed under the BSD license with permission of the author, see
   https://github.com/deepmind/torch-cephes/blob/master/LICENSE.txt
*/

template <typename Scalar> Scalar chbevl(Scalar x, const Scalar *array, int n) {
    const Scalar *p = array;
    Scalar b0 = *p++;
    Scalar b1 = 0;
    Scalar b2;
    int i = n - 1;

    do {
        b2 = b1;
        b1 = b0;
        b0 = x * b1  -  b2  + *p++;
    } while (--i);

    return (Scalar) 0.5 * (b0-b2);
}

template <typename Scalar> Scalar i0e(Scalar x) {
    /* Chebyshev coefficients for exp(-x) I0(x)
     * in the interval [0,8].
     *
     * lim(x->0) { exp(-x) I0(x) } = 1.
     */
    static const Scalar A[] = {
        -4.41534164647933937950E-18, 3.33079451882223809783E-17,
        -2.43127984654795469359E-16, 1.71539128555513303061E-15,
        -1.16853328779934516808E-14, 7.67618549860493561688E-14,
        -4.85644678311192946090E-13, 2.95505266312963983461E-12,
        -1.72682629144155570723E-11, 9.67580903537323691224E-11,
        -5.18979560163526290666E-10, 2.65982372468238665035E-9,
        -1.30002500998624804212E-8,  6.04699502254191894932E-8,
        -2.67079385394061173391E-7,  1.11738753912010371815E-6,
        -4.41673835845875056359E-6,  1.64484480707288970893E-5,
        -5.75419501008210370398E-5,  1.88502885095841655729E-4,
        -5.76375574538582365885E-4,  1.63947561694133579842E-3,
        -4.32430999505057594430E-3,  1.05464603945949983183E-2,
        -2.37374148058994688156E-2,  4.93052842396707084878E-2,
        -9.49010970480476444210E-2,  1.71620901522208775349E-1,
        -3.04682672343198398683E-1,  6.76795274409476084995E-1
    };

    /* Chebyshev coefficients for exp(-x) sqrt(x) I0(x)
     * in the inverted interval [8,infinity].
     *
     * lim(x->inf) { exp(-x) sqrt(x) I0(x) } = 1/sqrt(2pi).
     */
    static const Scalar B[] = {
        -7.23318048787475395456E-18, -4.83050448594418207126E-18,
         4.46562142029675999901E-17,  3.46122286769746109310E-17,
        -2.82762398051658348494E-16, -3.42548561967721913462E-16,
         1.77256013305652638360E-15,  3.81168066935262242075E-15,
        -9.55484669882830764870E-15, -4.15056934728722208663E-14,
         1.54008621752140982691E-14,  3.85277838274214270114E-13,
         7.18012445138366623367E-13, -1.79417853150680611778E-12,
        -1.32158118404477131188E-11, -3.14991652796324136454E-11,
         1.18891471078464383424E-11,  4.94060238822496958910E-10,
         3.39623202570838634515E-9,   2.26666899049817806459E-8,
         2.04891858946906374183E-7,   2.89137052083475648297E-6,
         6.88975834691682398426E-5,   3.36911647825569408990E-3,
         8.04490411014108831608E-1
    };

    if (x < 0)
        x = -x;
    if (x <= 8) {
        Scalar y = (x * (Scalar) 0.5) - (Scalar) 2.0;
        return chbevl(y, A, 30);
    } else {
        return chbevl((Scalar) 32.0/x - (Scalar) 2.0, B, 25) / std::sqrt(x);
    }
}

extern template MTS_EXPORT_CORE Float i0e(Float);

//! @}
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
//! @{ \name Legendre functions
// -----------------------------------------------------------------------

/// Evaluate the l-th Legendre polynomial using recurrence
template <typename Value>
Value legendre_p(int l, Value x) {
    using Scalar = scalar_t<Value>;
    Value l_cur = 0.f;

    assert(l >= 0);

    if (likely(l > 1)) {
        Value l_p_pred = 1.f, l_pred = x;
        Scalar k0 = 3.f, k1 = 2.f, k2 = 1.f;

        for (int ki = 2; ki <= l; ++ki) {
            l_cur = (k0 * x * l_pred - k2  * l_p_pred) / k1;
            l_p_pred = l_pred; l_pred = l_cur;
            k2 = k1; k0 += 2.f; k1 += 1.f;
        }

        return l_cur;
    } else {
        l_cur = (l == 0) ? 1 : x;
    }
    return l_cur;
}

/// Evaluate an associated Legendre polynomial using recurrence
template <typename Scalar>
Scalar legendre_p(int l, int m, Scalar x) {
    Scalar p_mm = 1.f;

    if (likely(m > 0)) {
        Scalar somx2 = sqrt((1.f - x) * (1.f + x));
        Scalar fact = 1.f;
        for (int i = 1; i <= m; i++) {
            p_mm *= (-fact) * somx2;
            fact += 2.f;
        }
    }

    if (unlikely(l == m))
        return p_mm;

    Scalar p_mmp1 = x * (2 * m + 1.f) * p_mm;
    if (unlikely(l == m + 1))
        return p_mmp1;

    Scalar p_ll = 0.f;
    for (int ll = m + 2; ll <= l; ++ll) {
        p_ll = ((2 * ll - 1) * x * p_mmp1 - (ll + m - 1) * p_mm) / (ll - m);
        p_mm = p_mmp1;
        p_mmp1 = p_ll;
    }

    return p_ll;
}

/// Evaluate the l-th Legendre polynomial and its derivative using recurrence
template <typename Value>
std::pair<Value, Value> legendre_pd(int l, Value x) {
    using Scalar = scalar_t<Value>;

    assert(l >= 0);
    Value l_cur = 0.f, d_cur = 0.f;

    if (likely(l > 1)) {
        Value l_p_pred = 1.f, l_pred = x,
              d_p_pred = 0.f, d_pred = 1.f;
        Scalar k0 = 3.f, k1 = 2.f, k2 = 1.f;

        for (int ki = 2; ki <= l; ++ki) {
            l_cur = (k0 * x * l_pred - k2 * l_p_pred) / k1;
            d_cur = d_p_pred + k0 * l_pred;
            l_p_pred = l_pred; l_pred = l_cur;
            d_p_pred = d_pred; d_pred = d_cur;
            k2 = k1; k0 += 2.f; k1 += 1.f;
        }
    } else {
        if (l == 0) {
            l_cur = 1.f; d_cur = 0.f;
        } else {
            l_cur = x; d_cur = 1.f;
        }
    }

    return std::make_pair(l_cur, d_cur);
}

/// Evaluate the function legendre_pd(l+1, x) - legendre_pd(l-1, x)
template <typename Value>
std::pair<Value, Value> legendre_pd_diff(int l, Value x) {
    using Scalar = scalar_t<Value>;
    assert(l >= 1);

    if (likely(l > 1)) {
        Value l_p_pred = 1.f, l_pred = x, l_cur = 0.f,
              d_p_pred = 0.f, d_pred = 1.f, d_cur = 0.f;
        Scalar k0 = 3.f, k1 = 2.f, k2 = 1.f;

        for (int ki = 2; ki <= l; ++ki) {
            l_cur = (k0 * x * l_pred - k2 * l_p_pred) / k1;
            d_cur = d_p_pred + k0 * l_pred;
            l_p_pred = l_pred; l_pred = l_cur;
            d_p_pred = d_pred; d_pred = d_cur;
            k2 = k1; k0 += 2.f; k1 += 1.f;
        }

        Value l_next = (k0 * x * l_pred - k2 * l_p_pred) / k1;
        Value d_next = d_p_pred + k0 * l_pred;

        return std::make_pair(l_next - l_p_pred, d_next - d_p_pred);
    } else {
        return std::make_pair(Scalar(0.5f) * (3.f*x*x-1.f) - 1.f, 3.f*x);
    }
}

//! @}
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
//! @{ \name Miscellaneous mathematical helper functions
// -----------------------------------------------------------------------

//// Convert radians to degrees
template <typename T> T rad_to_deg(T value) { return value * T(180 / Pi_d); }

/// Convert degrees to radians
template <typename T> T deg_to_rad(T value) { return value * T(Pi_d / 180); }

/**
 * \brief Compare the difference in ULPs between a reference value and another
 * given floating point number
 */
template <typename T> T ulpdiff(T ref, T val) {
    constexpr T eps = std::numeric_limits<T>::epsilon() / 2;

    /* Express mantissa wrt. same exponent */
    int e_ref, e_val;
    T m_ref = std::frexp(ref, &e_ref);
    T m_val = std::frexp(val, &e_val);

    T diff;
    if (e_ref == e_val)
        diff = m_ref - m_val;
    else
        diff = m_ref - std::ldexp(m_val, e_val-e_ref);

    return std::abs(diff) / eps;
}

/// Always-positive modulo function
template <typename T> T modulo(T a, T b) {
    T result = a - (a / b) * b;
    return select(result < 0, result + b, result);
}

/// Check whether the provided integer is a power of two
template <typename T> bool is_power_of_two(T i) {
    return i > 0 && (i & (i-1)) == 0;
}

/// Round an unsigned integer to the next integer power of two
template <typename T> T round_to_power_of_two(T i) {
    if (i <= 1)
        return 1;
    return T(1) << (log2i<T>(i - 1) + 1);
}

/// Apply the sRGB gamma curve to a floating point scalar
template <typename T>
static T gamma(T value) {
    auto branch1 = T(12.92) * value;
    auto branch2 = T(1.055) * pow(value, T(1.0/2.4)) - T(0.055);

    return select(value <= T(0.0031308), branch1, branch2);
}

/// Apply the inverse of the sRGB gamma curve to a floating point scalar
template <typename T>
static T inv_gamma(T value) {
    auto branch1 = value * T(1.0 / 12.92);
    auto branch2 = pow((value + T(0.055)) * T(1.0 / 1.055), T(2.4));

    return select(value <= T(0.04045), branch1, branch2);
}
/**
* \brief Find an interval in an ordered set
*
* This function is very similar to \c std::upper_bound, but it uses a functor
* rather than an actual array to permit working with procedurally defined
* data. It returns the index \c i such that pred(i) is \c true and pred(i+1)
* is \c false.
*
* This function is primarily used to locate an interval (i, i+1) for linear
* interpolation, hence its name. To avoid issues out of bounds accesses, and
* to deal with predicates that evaluate to \c true or \c false on the entire
* domain, the returned left interval index is clamped to the range <tt>[left,
* right-2]</tt>.
*
*/
template <typename Size, typename Predicate,
          typename Args = typename function_traits<Predicate>::Args,
          typename Index = std::tuple_element_t<0, Args>,
          typename IndexMask = mask_t<Index>>
Index find_interval(Size left, Size right, const Predicate &pred) {
    Size initial_size = right - left;

    if (unlikely(initial_size == 0))
        return zero<Index>();

    IndexMask active(true);
    Index first(left), size(initial_size);

    do {
        Index half   = size >> 1,
              middle = first + half;

        /* Evaluate the predicate */
        IndexMask pred_result = reinterpret_array<IndexMask>(pred(middle)) & active;

        /* .. and recurse into the left or right */
        first = select(pred_result, middle + 1, first);

        /* Update the remaining interval size */
        size = select(pred_result, size - (half + 1), half);

        /* Disable converged entries */
        active &= size > 0;
    } while (any_nested(active));

    using SignedIndex = signed_array_t<Index>;

    return Index(clamp(
        SignedIndex(first - 1),
        SignedIndex(left),
        SignedIndex(right - 2)
    ));
}

template <typename Size, typename Predicate,
          typename Args = typename function_traits<Predicate>::Args,
          typename Index = std::tuple_element_t<0, Args>,
          typename Mask = enoki::mask_t<Index>>
Index find_interval(Size size, const Predicate &pred) {
    return find_interval(Size(0), size, pred);
}

/**
 * \brief Find an interval in an ordered set
 *
 * This function is very similar to \c std::upper_bound, but it uses a functor
 * rather than an actual array to permit working with procedurally defined
 * data. It returns the index \c i such that pred(i) is \c true and pred(i+1)
 * is \c false. See below for special cases.
 *
 * This function is primarily used to locate an interval (i, i+1) for linear
 * interpolation, hence its name. To avoid issues out of bounds accesses, and
 * to deal with predicates that evaluate to \c true or \c false on the entire
 * domain, the returned left interval index is clamped to the range <tt>[left,
 * right-2]</tt>.
 * In particular:
 * If there is no index such that pred(i) is true, we return (left).
 * If there is no index such that pred(i+1) is false, we return (right-2).
 *
 * \remark This function is intended for vectorized predicates and additionally
 * accepts a mask as an input. This mask can be used to disable some of the
 * array entries. The mask is passed to the predicate as a second parameter.
 */
template <typename Size, typename Predicate,
          typename Args = typename function_traits<Predicate>::Args,
          typename Index = std::tuple_element_t<0, Args>,
          typename Mask = std::tuple_element_t<1, Args>>
Index find_interval(Size left, Size right, const Predicate &pred, Mask active_in) {
    using IndexMask = mask_t<Index>;

    Size initial_size = right - left;

    if (initial_size == 0)
        return zero<Index>();

    Index first(left), size(initial_size);

    IndexMask active = reinterpret_array<IndexMask>(active_in);

    do {
        Index half   = size >> 1,
              middle = first + half;

        /* Evaluate the predicate */
        IndexMask pred_result = reinterpret_array<IndexMask>(
            pred(middle, reinterpret_array<Mask>(active))) & active;

        /* .. and recurse into the left or right */
        first = select(pred_result, middle + 1, first);

        /* Update the remaining interval size */
        size = select(pred_result, size - (half + 1), half);

        /* Disable converged entries */
        active &= size > 0;
    } while (any_nested(active));

    using SignedIndex = signed_array_t<Index>;

    return Index(clamp(
        SignedIndex(first - 1),
        SignedIndex(left),
        SignedIndex(right - 2)
    ));
}

template <typename Size, typename Predicate,
          typename Args = typename function_traits<Predicate>::Args,
          typename Index = std::tuple_element_t<0, Args>,
          typename Mask = enoki::mask_t<Index>>
Index find_interval(Size size, const Predicate &pred, Mask active) {
    return find_interval(Size(0), size, pred, active);
}

/**
 * \brief Bisect a floating point interval given a predicate function
 *
 * This function takes an interval [\c left, \c right] and a predicate \c pred
 * as inputs. It assumes that <tt>pred(left)==true</tt> and
 * <tt>pred(right)==false</tt>. It also assumes that there is a single floating
 * point number \c t such that \c pred is \c true for all values in the range
 * [\c left, \c t] and \c false for all values in the range (\c t, \c right].
 *
 * The bisection search then finds and returns \c t by repeatedly splitting the
 * input interval. The number of iterations is roughly bounded by the number of
 * bits of the underlying floating point representation.
 */
template <typename Scalar, typename Predicate>
Scalar bisect(Scalar left, Scalar right, const Predicate &pred) {
    int it = 0;
    while (true) {
        Scalar middle = (left + right) * 0.5f;

        /* Paranoid stopping criterion */
        if (middle <= left || middle >= right) {
            middle = std::nextafter(left, right);
            if (middle == right)
                break;
        }

        if (pred(middle))
            left = middle;
        else
            right = middle;
        it++;
        if (it > 100)
            throw std::runtime_error("Internal error in util::bisect!");
    }

    return left;
}

/**
 * \brief Compute the Chi^2 statistic and degrees of freedom of the given
 * arrays while pooling low-valued entries together
 *
 * Given a list of observations counts (``obs[i]``) and expected observation
 * counts (``exp[i]``), this function accumulates the Chi^2 statistic, that is,
 * ``(obs-exp)^2 / exp`` for each element ``0, ..., n-1``.
 *
 * Minimum expected cell frequency. The Chi^2 test statistic is not useful when
 * when the expected frequency in a cell is low (e.g. less than 5), because
 * normality assumptions break down in this case. Therefore, the implementation
 * will merge such low-frequency cells when they fall below the threshold
 * specified here. Specifically, low-valued cells with ``exp[i] < pool_threshold``
 * are pooled into larger groups that are above the threshold before their
 * contents are added to the Chi^2 statistic.
 *
 * The function returns the statistic value, degrees of freedom, below-treshold
 * entries and resulting number of pooled regions.
 *
 */
template <typename Scalar> std::tuple<Scalar, size_t, size_t, size_t>
        chi2(const Scalar *obs, const Scalar *exp, Scalar pool_threshold, size_t n) {
    Scalar chsq = 0, pooled_obs = 0, pooled_exp = 0;
    size_t dof = 0, n_pooled_in = 0, n_pooled_out = 0;

    for (size_t i = 0; i<n; ++i) {
        if (exp[i] == 0 && obs[i] == 0)
            continue;

        if (exp[i] < pool_threshold) {
            pooled_obs += obs[i];
            pooled_exp += exp[i];
            n_pooled_in++;

            if (pooled_exp > pool_threshold) {
                Scalar diff = pooled_obs - pooled_exp;
                chsq += (diff*diff) / pooled_exp;
                pooled_obs = pooled_exp = 0;
                ++n_pooled_out; ++dof;
            }
        } else {
            Scalar diff = obs[i] - exp[i];
            chsq += (diff*diff) / exp[i];
            ++dof;
        }
    }

    return std::make_tuple(chsq, dof - 1, n_pooled_in, n_pooled_out);
}

/**
 * \brief Solve a quadratic equation of the form a*x^2 + b*x + c = 0.
 * \return \c true if a solution could be found
 */
template <typename Value>
std::tuple<mask_t<Value>, Value, Value> solve_quadratic(Value a, Value b, Value c) {
    using Scalar = scalar_t<Value>;
    using Mask = mask_t<Value>;

    /* Is this perhaps a linear equation? */
    Mask linear_case = eq(a, Scalar(0));

    /* If so, we require b > 0 */
    Mask active = (!linear_case) | (b > Scalar(0));

    /* Initialize solution with that of linear equation */
    Value x0, x1;
    x0 = x1 = -c * rcp(b);

    /* Check if the quadratic equation is solvable */
    Value discrim = b * b - Scalar(4) * a * c;
    active &= linear_case | discrim >= 0;

    if (likely(any(active))) {
        Value sqrt_discrim = sqrt(discrim);

        /* Numerically stable version of (-b (+/-) sqrt_discrim) / (2 * a)
         *
         * Based on the observation that one solution is always
         * accurate while the other is not. Finds the solution of
         * greater magnitude which does not suffer from loss of
         * precision and then uses the identity x1 * x2 = c / a
         */
        Value temp = -Scalar(0.5) * (b + copysign(sqrt_discrim, b));
        Value x0p = temp * rcp(a);
        Value x1p = c * rcp(temp);

        /* Order the results so that x0 < x1 */
        Value x0m = enoki::min(x0p, x1p);
        Value x1m = enoki::max(x0p, x1p);

        x0 = select(linear_case, x0, x0m);
        x1 = select(linear_case, x0, x1m);
    }

    return std::make_tuple(active, x0, x1);
}

//! @}
// -----------------------------------------------------------------------

NAMESPACE_END(math)
NAMESPACE_END(mitsuba)
