#pragma once

#include <mitsuba/mitsuba.h>
#include <enoki/array.h>
#include <enoki/dynamic.h>

NAMESPACE_BEGIN(mitsuba)

template <typename T>
using float_array_t = replace_scalar_t<T, Float>;

/// Convenience function which computes an array size/type suffix (like '2u' or '3fP')
template <typename T> std::string type_suffix() {
    using B = scalar_t<T>;

    std::string id = std::to_string(array_size_v<T>);

    if (std::is_floating_point_v<B>) {
        if (std::is_same_v<B, enoki::half>) {
            id += 'h';
        } else {
            #if defined(SINGLE_PRECISION)
                id += std::is_same_v<B, float> ? 'f' : 'd';
            #else
                id += std::is_same_v<B, double> ? 'f' : 's';
            #endif
        }
    } else {
        if (std::is_signed_v<B>)
            id += 'i';
        else
            id += 'u';
    }

    if (is_static_array_v<value_t<T>>)
        id += 'P';
    else if (is_dynamic_array_v<value_t<T>>)
        id += 'X';

    return id;
}

/// Round an integer to a multiple of the current packet size
inline size_t round_to_packet_size(size_t size) {
    return (size + PacketSize - 1) / PacketSize * PacketSize;
}

NAMESPACE_END(mitsuba)
