#pragma once

#include <mitsuba/core/object.h>
#include <mitsuba/core/stream.h>
#include <unordered_map>
#include <vector>

NAMESPACE_BEGIN(mitsuba)

/** \brief An AnnotatedStream adds Table of Contents capabilities to an
 * underlying stream. A Stream instance must first be created and passed to
 * to the constructor. The underlying stream should either be empty or a
 * stream that was previously written with an AnnotatedStream, so that it
 * contains a proper Table of Contents.
 *
 * Table of Contents: objects and variables written to the stream are
 * prepended by a field name. Contents can then be queried by field name, as
 * if using a map.
 * A hierarchy can be created by <tt>push</tt>ing and <tt>pop</tt>ing prefixes.
 * The root of this hierarchy is the empty prefix "".
 *
 * The table of contents is automatically read from the underlying stream on
 * creation and written back on destruction.
 */
class MTS_EXPORT_CORE AnnotatedStream : public Object {
public:

    /** \brief Creates an AnnotatedStream based on the given Stream (decorator pattern).
     * Anything written to the AnnotatedStream is ultimately passed down to the
     * given Stream instance.
     * The given Stream instance should not be destructed before this.
     *
     * Throws if the underlying stream has read capabilities and is not empty
     * but does not correspond to a valid AnnotatedStream (i.e. it does not
     * start with the \ref kSerializedHeaderId sentry).
     */
    AnnotatedStream(ref<Stream> &stream, bool throwOnMissing = false);

    // =========================================================================
    //! @{ \name Table of Contents (TOC)
    // =========================================================================

    /** \brief Push a name prefix onto the stack (use this to isolate
     * identically-named data fields).
     */
    void push(const std::string &name);

    /// Pop a name prefix from the stack
    void pop();

    /// Return all field names under the current name prefix
    std::vector<std::string> keys() const;

    /** \brief Retrieve a field from the serialized file (only valid in read mode)
     *
     * Throws if the field exists but has the wrong type.
     * Throws if the field is not found and <tt>throwOnMissing</tt> is true.
     */
    template <typename T> bool get(const std::string &name, T &value) {
        using helper = detail::serialization_helper<T>;
        if (!getBase(name, helper::type_id()))
            return false;
        if (!name.empty())
            push(name);
        m_stream->readValue(value);
        if (!name.empty())
            pop();
        return true;
    }

    /// Store a field in the serialized file (only valid in write mode)
    template <typename T> void set(const std::string &name, const T &value) {
        using helper = detail::serialization_helper<T>;
        setBase(name, helper::type_id());
        if (!name.empty())
            push(name);
        m_stream->writeValue(value);
        if (!name.empty())
            pop();
    }

    /// @}
    // =========================================================================

    // =========================================================================
    //! @{ \name Underlying stream
    // =========================================================================

    /// Returns the current size of the underlying stream
    size_t getSize() { return m_stream->getSize(); }

    /// Whether the underlying stream has read capabilities
    bool canRead() { return m_stream->canRead(); }

    /// Whether the underlying stream has write capabilities
    bool canWrite() { return m_stream->canWrite(); }

    /// @}
    // =========================================================================

    MTS_DECLARE_CLASS();

protected:

    /// Destructor
    virtual ~AnnotatedStream();

    /** \brief Attempts to seek to the position of the given field.
     * The active prefix (from previous \ref push operations) is prepended
     * to the given <tt>name</tt>.
     *
     * Throws if the field exists but has the wrong type.
     * Throws if the field is not found and <tt>m_throwOnMissing</tt> is true.
     */
    bool getBase(const std::string &name, const std::string &type_id);

    /** \brief Attempts to associate the current position of the stream to
     * the given field. The active prefix (from previous \ref push operations)
     * is prepended to the <tt>name</tt> of the field.
     *
     * Throws if a value was already set with that name (including prefix).
     */
    void setBase(const std::string &name, const std::string &type_id);

    /** \brief Read back the table of contents from the underlying stream and
     * update the in-memory <tt>m_table</tt> accordingly.
     * Should be called on construction.
     *
     * Throws if the underlying stream does not have read capabilities.
     * Throws if the underlying stream does not have start with the
     * AnnotatedStream sentry (\ref kSerializedHeaderId).
     */
    void readTOC();
    /** \brief Write back the table of contents to the underlying stream.
     * Should be called on destruction.
     */
    void writeTOC();

private:

    /// Underlying stream where the names and contents are written
    ref<Stream> m_stream;
    /// Maintains the mapping: full field name -> (type, position in the stream)
    std::unordered_map<std::string, std::pair<std::string, uint64_t>> m_table;
    /** \brief Stack of accumulated prefixes,
     * i.e. <tt>m_prefixStack.back</tt> is the full prefix path currently applied.
     */
    std::vector<std::string> m_prefixStack;

    bool m_throwOnMissing;
};

NAMESPACE_END(mitsuba)
