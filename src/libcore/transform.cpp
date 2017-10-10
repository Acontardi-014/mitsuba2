#include <mitsuba/core/transform.h>

NAMESPACE_BEGIN(mitsuba)

AnimatedTransform::~AnimatedTransform() { }

void AnimatedTransform::append(const Keyframe &keyframe) {
    if (!m_keyframes.empty() && keyframe.time <= m_keyframes.back().time)
        Throw("AnimatedTransform::append(): time values must be "
              "strictly monotonically increasing!");

    if (m_keyframes.empty())
        m_transform = Transform4f(
            enoki::transform_compose(keyframe.scale, keyframe.quat, keyframe.trans),
            enoki::transform_compose_inverse(keyframe.scale, keyframe.quat, keyframe.trans));

    m_keyframes.push_back(keyframe);
}

void AnimatedTransform::append(Float time, const Transform4f &trafo) {
    if (!m_keyframes.empty() && time <= m_keyframes.back().time)
        Throw("AnimatedTransform::append(): time values must be "
              "strictly monotonically increasing!");

    Matrix3f M; Quaternion4f Q; Vector3f T;

    /* Perform a polar decomposition into a 3x3 scale/shear matrix,
       a rotation quaternion, and a translation vector. These will
       all be interpolated independently. */
    std::tie(M, Q, T) = enoki::transform_decompose(trafo.matrix);

    if (m_keyframes.empty())
        m_transform = trafo;

    m_keyframes.push_back(Keyframe { time, M, Q, T });
}

template <typename Value>
Transform<Value> ENOKI_INLINE AnimatedTransform::lookup_impl(const Value &time, const mask_t<Value> &active_) const {
    static_assert(std::is_same<scalar_t<Value>, Float>::value, "Expected a 'float'-valued time parameter");

    auto active = disable_mask_if_scalar(active_);

    /* Compute constants describing the layout of the 'Keyframe' data structure */
    constexpr size_t Stride      = sizeof(Keyframe);
    constexpr size_t ScaleOffset = offsetof(Keyframe, scale) / sizeof(Float);
    constexpr size_t QuatOffset  = offsetof(Keyframe, quat)  / sizeof(Float);
    constexpr size_t TransOffset = offsetof(Keyframe, trans) / sizeof(Float);

    using Index       = uint32_array_t<Value>;
    using Matrix3     = Matrix<Value, 3>;
    using Vector3     = Vector<Value, 3>;
    using Quaternion4 = Quaternion<Value>;

    /* Perhaps the transformation isn't animated */
    if (likely(size() <= 1))
        return m_transform;

    /* Look up the interval containing 'time' */
    Index idx0 = math::find_interval(
        (uint32_t) size(),
        [&](Index idx, mask_t<Value> active) {
            constexpr size_t Stride_ = sizeof(Keyframe); // MSVC: have to redeclare constexpr variable in lambda scope :(
            return gather<Value, Stride_>(m_keyframes.data(), idx, active) <= time;
        },
        active);

    Index idx1 = idx0 + 1;

    /* Compute the relative time value in [0, 1] */
    Value t0 = gather<Value, Stride>(m_keyframes.data(), idx0, active);
    Value t1 = gather<Value, Stride>(m_keyframes.data(), idx1, active);
    Value t = min(max((time - t0) / (t1 - t0), 0.f), 1.f);

    /* Interpolate the scale matrix */
    Matrix3 scale0 = gather<Matrix3, Stride>((Float *) m_keyframes.data() + ScaleOffset, idx0, active);
    Matrix3 scale1 = gather<Matrix3, Stride>((Float *) m_keyframes.data() + ScaleOffset, idx1, active);
    Matrix3 scale = scale0 * (1 - t) + scale1 * t;

    /* Interpolate the rotation quaternion */
    Quaternion4 quat0 = gather<Quaternion4, Stride>((Float *) m_keyframes.data() + QuatOffset, idx0, active);
    Quaternion4 quat1 = gather<Quaternion4, Stride>((Float *) m_keyframes.data() + QuatOffset, idx1, active);
    Quaternion4 quat = enoki::slerp(quat0, quat1, t);

    /* Interpolate the translation component */
    Vector3 trans0 = gather<Vector3, Stride>((Float *) m_keyframes.data() + TransOffset, idx0, active);
    Vector3 trans1 = gather<Vector3, Stride>((Float *) m_keyframes.data() + TransOffset, idx1, active);
    Vector3 trans = trans0 * (1 - t) + trans1 * t;

    return Transform<Value>(
        enoki::transform_compose(scale, quat, trans),
        enoki::transform_compose_inverse(scale, quat, trans)
    );
}

/// Look up an interpolated transformation at the given time
Transform4f AnimatedTransform::lookup(const Float &time, const bool &active) const {
    return lookup_impl(time, active);

}

/// Vectorized version of \ref lookup
Transform4fP AnimatedTransform::lookup(const FloatP &time, const mask_t<FloatP> &active) const {
    return lookup_impl(time, active);
}

std::string AnimatedTransform::to_string() const {
    std::ostringstream oss;
    oss << "AnimatedTransform[" << std::endl
        << "  m_transform = " << string::indent(m_transform, 16) << "," << std::endl
        << "  m_keyframes = " << string::indent(m_keyframes, 16) << std::endl
        << "]";

    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const AnimatedTransform::Keyframe &frame) {
    os << "Keyframe[" << std::endl
       << "  time = " << frame.time << "," << std::endl
       << "  scale = " << frame.scale << "," << std::endl
       << "  quat = " << frame.quat << "," << std::endl
       << "  trans = " << frame.trans
       << "]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const AnimatedTransform &t) {
    os << t.to_string();
    return os;
}

template Point3f   MTS_EXPORT_CORE Transform4f::operator*(const Point3f&) const;
template Point3fP  MTS_EXPORT_CORE Transform4f::operator*(const Point3fP&) const;
template Vector3f  MTS_EXPORT_CORE Transform4f::operator*(const Vector3f&) const;
template Vector3fP MTS_EXPORT_CORE Transform4f::operator*(const Vector3fP&) const;
template Normal3f  MTS_EXPORT_CORE Transform4f::operator*(const Normal3f&) const;
template Normal3fP MTS_EXPORT_CORE Transform4f::operator*(const Normal3fP&) const;
template Ray3f     MTS_EXPORT_CORE Transform4f::operator*(const Ray3f&) const;
template Ray3fP    MTS_EXPORT_CORE Transform4f::operator*(const Ray3fP&) const;

template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Point3f&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Point3fP&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Vector3f&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Vector3fP&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Normal3f&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Normal3fP&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Ray3f&) const;
template auto      MTS_EXPORT_CORE Transform4f::transform_affine(const Ray3fP&) const;

MTS_IMPLEMENT_CLASS(AnimatedTransform, Object)
NAMESPACE_END(mitsuba)
