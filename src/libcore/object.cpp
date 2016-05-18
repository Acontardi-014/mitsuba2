#include <mitsuba/core/object.h>
#include <cstdlib>
#include <cstdio>
#include <sstream>

NAMESPACE_BEGIN(mitsuba)

void Object::decRef(bool dealloc) const noexcept {
    --m_refCount;
    if (m_refCount == 0 && dealloc) {
        delete this;
    } else if (m_refCount < 0) {
        fprintf(stderr, "Internal error: Object reference count < 0!\n");
        abort();
    }
}

std::string Object::toString() const {
    std::ostringstream oss;
    oss << getClass()->getName() << "[" << (void *) this << "]";
    return oss.str();
}

Object::~Object() { }

std::ostream& operator<<(std::ostream &os, const Object *object) {
    os << object->toString();
    return os;
}

MTS_IMPLEMENT_CLASS(Object,)
NAMESPACE_END(mitsuba)
