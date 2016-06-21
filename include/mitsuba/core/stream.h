#pragma once

#include <mitsuba/core/object.h>
#include <vector>

NAMESPACE_BEGIN(mitsuba)

NAMESPACE_BEGIN(detail)
template <typename T> struct serialization_helper;
NAMESPACE_END(detail)

/** \brief Abstract seekable stream class
 *
 * Specifies all functions to be implemented by stream
 * subclasses and provides various convenience functions
 * layered on top of on them.
 *
 * All read<b>X</b>() and write<b>X</b>() methods support transparent
 * conversion based on the endianness of the underlying system and the
 * value passed to \ref setByteOrder(). Whenever \ref getHostByteOrder()
 * and \ref getByteOrder() disagree, the endianness is swapped.
 *
 * TODO: explain Table of Contents feature.
 * TODO: explain write-only / read-only modes.
 *
 * \sa FileStream, MemoryStream, DummyStream
 */
class MTS_EXPORT_CORE Stream : public Object {

protected:
    template <typename T> friend struct detail::serialization_helper;

public:
    /// Defines the byte order to use in this Stream
    enum EByteOrder {
        EBigEndian = 0,                ///< PowerPC, SPARC, Motorola 68K
        ELittleEndian = 1,             ///< x86, x86_64
        ENetworkByteOrder = EBigEndian ///< Network byte order (an alias for big endian)
    };

    /**
     * \brief Creates a new stream.
     * By default, it assumes the byte order of the underlying system,
     * i.e. no endianness conversion is performed.
     *
     * \param writeEnabled If true, the stream will be write-only, otherwise
     *                     it will be read-only.
     * \param tableOfContents If true, the stream will function by generating
     *                        a "table of contents" (TOC), using hierarchical
     *                        prefixes for fields being written.
     *                        TODO: enable / disable some methods (use templates?)
     */
    Stream(bool writeEnabled, bool tableOfContents = true);

    /// Returns a string representation of the stream
    virtual std::string toString() const override;

    // =========================================================================
    //! @{ \name Abstract methods that need to be implemented by subclasses
    // =========================================================================
protected:

    /**
     * \brief Reads a specified amount of data from the stream.
     *
     * Throws an exception when the stream ended prematurely.
     * Implementations need to handle endianness swap when appropriate.
     */
    virtual void read(void *p, size_t size) = 0;

    /**
     * \brief Writes a specified amount of data into the stream.
     *
     * Throws an exception when not all data could be written.
     * Implementations need to handle endianness swap when appropriate.
     */
    virtual void write(const void *p, size_t size) = 0;

public:
    /// Seeks to a position inside the stream
    virtual void seek(size_t pos) = 0;

    /// Truncates the stream to a given size
    virtual void truncate(size_t size) = 0;

    /// Gets the current position inside the stream
    virtual size_t getPos() const = 0;

    /// Returns the size of the stream
    virtual size_t getSize() const = 0;

    /// Flushes the stream's buffers, if any
    virtual void flush() = 0;

    /// Can we write to the stream?
    virtual bool canWrite() const {
        return m_writeMode;
    }

    /// Can we read from the stream?
    virtual bool canRead() const {
        return !m_writeMode;
    }

    /// @}
    // =========================================================================

    // =========================================================================
    //! @{ \name Table of Contents (TOC)
    // =========================================================================

    /**
     * \brief Reads one object of type T from the stream at the current position
     * by delegating to the appropriate <tt>serialization_helper</tt>.
     */
    template <typename T>
    void readValue(T &value) {
        using helper = detail::serialization_helper<T>;
        helper::read(*this, &value, 1);
    }

    /**
     * \brief Reads one object of type T from the stream at the current position
     * by delegating to the appropriate <tt>serialization_helper</tt>.
     */
    template <typename T>
    void writeValue(const T &value) {
        using helper = detail::serialization_helper<T>;
        helper::write(*this, &value, 1);
    }

    /// @}
    // =========================================================================

    // =========================================================================
    //! @{ \name Endianness handling
    // =========================================================================

    // TODO

    /// @}
    // =========================================================================

    MTS_DECLARE_CLASS();

protected:
    /// Destructor
    virtual ~Stream() { };

    /// Copy is disallowed.
    // TODO: refactor non-copyable feature to a mixin or macro (?)
    Stream(const Stream&) = delete;
    void operator=(const Stream&) = delete;

    bool m_writeMode;

private:
    bool m_tocEnabled;
    static const EByteOrder m_hostByteOrder;
    EByteOrder m_byteOrder;
    // TODO: data members (ToC, endianness, etc).
};

extern MTS_EXPORT_CORE std::ostream
    &operator<<(std::ostream &os, const Stream::EByteOrder &value);

NAMESPACE_BEGIN(detail)

/**
 * \brief The <tt>serialization_trait</tt> templated structure provides
 * type-specific information relevant to serialization.
 * <tt>type_id</tt> is a unique, prefix-free code identifying the type.
 */
template <typename T, typename SFINAE = void> struct serialization_traits { };
template <> struct serialization_traits<int8_t>           { const char *type_id = "u8";  };
template <> struct serialization_traits<uint8_t>          { const char *type_id = "s8";  };

template <typename T> struct serialization_traits<T> :
    serialization_traits<typename std::underlying_type<T>::type,
                         typename std::enable_if<std::is_enum<T>::value>::type> { };

template <typename T> struct serialization_helper {
    static std::string type_id() { return serialization_traits<T>().type_id; }

    /** \brief Writes <tt>count</tt> values of type T into stream <tt>s</tt>
     * starting at its current position.
     * Note: <tt>count</tt> is the number of values, <b>not</b> a size in bytes.
     *
     * Support for additional types can be added in any header file by
     * declaring a template specialization for your type.
     */
    static void write(Stream &s, const T *value, size_t count) {
        s.write(value, sizeof(T) * count);
    }

    /** \brief Reads <tt>count</tt> values of type T from stream <tt>s</tt>,
     * starting at its current position.
     * Note: <tt>count</tt> is the number of values, <b>not</b> a size in bytes.
     *
     * Support for additional types can be added in any header file by
     * declaring a template specialization for your type.
     */
    static void read(Stream &s, T *value, size_t count) {
        s.read(value, sizeof(T) * count);
    }
};

// TODO: `serialization_helper` template specializations to support various types

NAMESPACE_END(detail)

NAMESPACE_END(mitsuba)

