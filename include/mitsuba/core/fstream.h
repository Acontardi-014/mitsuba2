#pragma once

#include <mitsuba/core/filesystem.h>
#include <mitsuba/core/stream.h>
#include <iosfwd>
#include "logger.h"

namespace fs = mitsuba::filesystem;
using path = fs::path;

NAMESPACE_BEGIN(mitsuba)

/** \brief Simple \ref Stream implementation backed-up by a file.
 * The underlying file abstraction is std::fstream, and so most
 * operations can be expected to behave similarly.
 */
class MTS_EXPORT_CORE FileStream : public Stream {
public:

    /** \brief Constructs a new FileStream by opening the file pointed by <tt>p</tt>.
     * The file is opened in read-only or read/write mode as specified by <tt>write_enabled</tt>.
     *
     * If <tt>write_enabled</tt> and the file did not exist before, it is
     * created.
     * Throws if trying to open a non-existing file in with write disabled.
     * Throws an exception if the file cannot be opened / created.
     */
    FileStream(const fs::path &p, bool write_enabled);

    /// Returns a string representation
    std::string to_string() const override;

    /** \brief Closes the stream and the underlying file.
     * No further read or write operations are permitted.
     *
     * This function is idempotent.
     * It is called automatically by the destructor.
     */
    virtual void close() override;

    /// Whether the stream is closed (no read or write are then permitted).
    virtual bool is_closed() const override;

    /// Convenience function for reading a line of text from an ASCII file
    virtual std::string read_line() override;

    /// Return the "native" std::fstream associated with this FileStream
    std::fstream *native() { return m_file.get(); }

    /// Return the path descriptor associated with this FileStream
    const fs::path &path() const { return m_path; }

    // =========================================================================
    //! @{ \name Implementation of the Stream interface
    // Most methods can be delegated directly to the underlying
    // standard file stream, avoiding having to deal with portability.
    // =========================================================================
    /**
     * \brief Reads a specified amount of data from the stream.
     * Throws an exception when the stream ended prematurely.
     */
    virtual void read(void *p, size_t size) override;

    /**
     * \brief Writes a specified amount of data into the stream.
     * Throws an exception when not all data could be written.
     */
    virtual void write(const void *p, size_t size) override;

    /// Seeks to a position inside the stream. May throw if the resulting state is invalid.
    virtual void seek(size_t pos) override;

    /** \brief Truncates the file to a given size.
     * Automatically flushes the stream before truncating the file.
     * The position is updated to <tt>min(old_position, size)</tt>.
     *
     * Throws an exception if in read-only mode.
     */
    virtual void truncate(size_t size) override;

    /// Gets the current position inside the file
    virtual size_t tell() const override;

    /** \brief Returns the size of the file.
     * \note After a write, the size may not be updated
     * until a \ref flush is performed.
     */
    virtual size_t size() const override;

    /// Flushes any buffered operation to the underlying file.
    virtual void flush() override;

    /// Whether the field was open in write-mode (and was not closed)
    virtual bool can_write() const override { return m_write_enabled && !is_closed(); }

    /// True except if the stream was closed.
    virtual bool can_read() const override { return !is_closed(); }

    //! @}
    // =========================================================================

    MTS_DECLARE_CLASS()

protected:

    /// Protected destructor
    virtual ~FileStream();

private:

    fs::path m_path;
    mutable std::unique_ptr<std::fstream> m_file;
    bool m_write_enabled;
};

NAMESPACE_END(mitsuba)
