#include <mitsuba/core/stream.h>
#include <sstream>

NAMESPACE_BEGIN(mitsuba)

NAMESPACE_BEGIN(detail)
static Stream::EByteOrder getByteOrder() {
    union {
        uint8_t  charValue[2];
        uint16_t shortValue;
    };
    charValue[0] = 1;
    charValue[1] = 0;

    if (shortValue == 1)
        return Stream::ELittleEndian;
    else
        return Stream::EBigEndian;
}
NAMESPACE_END(detail)

const Stream::EByteOrder Stream::m_hostByteOrder = detail::getByteOrder();

// -----------------------------------------------------------------------------

Stream::Stream()
    : m_byteOrder(m_hostByteOrder) {

};

void Stream::setByteOrder(EByteOrder value) {
    m_byteOrder = value;
}

// -----------------------------------------------------------------------------

std::string Stream::toString() const {
    std::ostringstream oss;

    oss << "hostByteOrder="
        << m_hostByteOrder
        << ", byteOrder="
        << m_byteOrder;

    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const Stream::EByteOrder &value) {
    switch (value) {
        case Stream::ELittleEndian: os << "little-endian"; break;
        case Stream::EBigEndian: os << "big-endian"; break;
        default: os << "invalid"; break;
    }
    return os;
}

MTS_IMPLEMENT_CLASS(Stream, Object)

NAMESPACE_END(mitsuba)
