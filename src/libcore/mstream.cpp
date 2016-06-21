#include <mitsuba/core/mstream.h>
#include <sstream>

NAMESPACE_BEGIN(mitsuba)

std::string MemoryStream::toString() const {
    std::ostringstream oss;
    oss << "MemoryStream[" << Stream::toString()
        // TODO: complete with implementation-specific information
        << "]";
    return oss.str();
}

MemoryStream::~MemoryStream() {
    // TODO: release resources
}

MTS_IMPLEMENT_CLASS(MemoryStream, Stream)

NAMESPACE_END(mitsuba)
