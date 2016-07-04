#pragma once

#include <mitsuba/core/class.h>
#include <atomic>
#include <stdexcept>

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Reference counted object base class
 * \ingroup libcore
 */
class MTS_EXPORT_CORE Object {
public:
    /// Default constructor
    Object() { }

    /// Copy constructor
    Object(const Object &) { }

    /// Return the current reference count
    int getRefCount() const { return m_refCount; };

    /// Increase the object's reference count by one
    void incRef() const { ++m_refCount; }

    /** \brief Decrease the reference count of the object and possibly
     * deallocate it.
     *
     * The object will automatically be deallocated once the reference count
     * reaches zero.
     */
    void decRef(bool dealloc = true) const noexcept;

    /**
     * \brief Return a \ref Class instance containing run-time type information
     * about this Object
     * \sa Class
     */
    virtual const Class *class_() const;

    /**
     * \brief Return a human-readable string representation of the object's
     * contents.
     *
     * This function is mainly useful for debugging purposes and should ideally
     * be implemented by all subclasses. The default implementation simply
     * returns <tt>MyObject[<address of 'this' pointer>]</tt>, where
     * <tt>MyObject</tt> is the name of the class.
     */
    virtual std::string toString() const;

protected:
    /** \brief Virtual protected deconstructor.
     * (Will only be called by \ref ref)
     */
    virtual ~Object();

private:
    mutable std::atomic<int> m_refCount { 0 };
    static Class *m_class;
};

/**
 * \brief Reference counting helper
 *
 * The \a ref template is a simple wrapper to store a pointer to an object. It
 * takes care of increasing and decreasing the object's reference count as
 * needed. When the last reference goes out of scope, the associated object
 * will be deallocated.
 *
 * The advantage over C++ solutions such as <tt>std::shared_ptr</tt> is that
 * the reference count is very compactly integrated into the base object
 * itself.
 *
 * \ingroup libcore
 */
template <typename T> class ref {
public:
    /// Create a <tt>nullptr</tt>-valued reference
    ref() { }

    /// Construct a reference from a pointer
    ref(T *ptr) : m_ptr(ptr) {
        if (m_ptr)
            ((Object *) m_ptr)->incRef();
    }

    /// Copy constructor
    ref(const ref &r) : m_ptr(r.m_ptr) {
        if (m_ptr)
            ((Object *) m_ptr)->incRef();
    }

    /// Move constructor
    ref(ref &&r) noexcept : m_ptr(r.m_ptr) {
        r.m_ptr = nullptr;
    }

    /// Destroy this reference
    ~ref() {
        if (m_ptr)
            ((Object *) m_ptr)->decRef();
    }

    /// Move another reference into the current one
    ref& operator=(ref&& r) noexcept {
        if (&r != this) {
            if (m_ptr)
                ((Object *) m_ptr)->decRef();
            m_ptr = r.m_ptr;
            r.m_ptr = nullptr;
        }
        return *this;
    }

    /// Overwrite this reference with another reference
    ref& operator=(const ref& r) noexcept {
        if (m_ptr != r.m_ptr) {
            if (r.m_ptr)
                ((Object *) r.m_ptr)->incRef();
            if (m_ptr)
                ((Object *) m_ptr)->decRef();
            m_ptr = r.m_ptr;
        }
        return *this;
    }

    /// Overwrite this reference with a pointer to another object
    ref& operator=(T *ptr) noexcept {
        if (m_ptr != ptr) {
            if (ptr)
                ((Object *) ptr)->incRef();
            if (m_ptr)
                ((Object *) m_ptr)->decRef();
            m_ptr = ptr;
        }
        return *this;
    }

    /// Compare this reference to another reference
    bool operator==(const ref &r) const { return m_ptr == r.m_ptr; }

    /// Compare this reference to another reference
    bool operator!=(const ref &r) const { return m_ptr != r.m_ptr; }

    /// Compare this reference to a pointer
    bool operator==(const T* ptr) const { return m_ptr == ptr; }

    /// Compare this reference to a pointer
    bool operator!=(const T* ptr) const { return m_ptr != ptr; }

    /// Access the object referenced by this reference
    T* operator->() { return m_ptr; }

    /// Access the object referenced by this reference
    const T* operator->() const { return m_ptr; }

    /// Return a C++ reference to the referenced object
    T& operator*() { return *m_ptr; }

    /// Return a const C++ reference to the referenced object
    const T& operator*() const { return *m_ptr; }

    /// Return a pointer to the referenced object
    operator T* () { return m_ptr; }

    /// Return a const pointer to the referenced object
    T* get() { return m_ptr; }

    /// Return a pointer to the referenced object
    const T* get() const { return m_ptr; }

    /// Check if the object is defined
    operator bool() const { return m_ptr != nullptr; }
private:
    T *m_ptr = nullptr;
};

/// Prints the canonical string representation of an object instance
MTS_EXPORT_CORE std::ostream& operator<<(std::ostream &os, const Object *object);

/// Prints the canonical string representation of an object instance
template <typename T>
std::ostream& operator<<(std::ostream &os, const ref<T> &object) {
    return operator<<(os, object.get());
}

NAMESPACE_END(mitsuba)
