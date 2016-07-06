#pragma once

#include <mitsuba/mitsuba.h>
#include <memory>
#include <string>
#include <vector>

NAMESPACE_BEGIN(mitsuba)

/// Wrapper object used to represent named references to Object instances
class NamedReference {
public:
    NamedReference(const std::string &value) : m_value(value) { }
    operator const std::string&() const { return m_value; }
    bool operator==(const NamedReference &r) const { return r.m_value == m_value; }
    bool operator!=(const NamedReference &r) const { return r.m_value != m_value; }

private:
    std::string m_value;
};

/** \brief Associative parameter map for constructing
 * subclasses of \ref Object.
 *
 * Note that the Python bindings for this class do not implement
 * the various type-dependent getters and setters. Instead, they
 * are accessed just like a normal Python map, e.g:
 *
 * \code
 * myProps = mitsuba.core.Properties("pluginName")
 * myProps["stringProperty"] = "hello"
 * myProps["spectrumProperty"] = mitsuba.core.Spectrum(1.0)
 * \endcode
 */
class MTS_EXPORT_CORE Properties {
public:
	/// Supported types of properties
	enum EPropertyType {
		/// Boolean value (true/false)
		EBool = 0,
		/// 64-bit signed integer
		ELong,
		/// Floating point value
		EFloat,
		/// 3D point
		EPoint3f,
		/// 3D vector
		EVector3f,
		/// 4x4 transform for homogeneous coordinates
		ETransform,
		/// An animated 4x4 transformation
		EAnimatedTransform,
		/// Discretized color spectrum
		ESpectrum,
		/// String
		EString,
		/// Named reference to another named object
		ENamedReference,
		/// Arbitrary object
		EObject
	};

	/// Construct an empty property container
    Properties();

	/// Construct an empty property container with a specific plugin name
    Properties(const std::string &pluginName);

    /// Copy constructor
    Properties(const Properties &props);

    /// Assignment operator
    void operator=(const Properties &props);

    /// Release all memory
    ~Properties();

    /// Get the associated plugin name
    const std::string &pluginName() const;

    /// Set the associated plugin name
    void setPluginName(const std::string &name);

    /// Verify if a value with the specified name exists
    bool hasProperty(const std::string &name) const;

    /** \brief Returns the type of an existing property.
     * If no property exists under that name, an error is logged
     * and type <tt>void</tt> is returned.
     */
    EPropertyType propertyType(const std::string &name) const;

    /**
     * \brief Remove a property with the specified name
     * \return \c true upon success
     */
    bool removeProperty(const std::string &name);

    /**
     * \brief Manually mark a certain property as queried
     * \return \c true upon success
     */
    bool markQueried(const std::string &name) const;

    /// Check if a certain property was queried
    bool wasQueried(const std::string &name) const;

    /// Returns a unique identifier associated with this instance (or an empty string)
    const std::string &id() const;

    /// Set the unique identifier associated with this instance
    void setID(const std::string &id);

    /// Copy a single attribute from another Properties object and potentially rename it
    void copyAttribute(const Properties &properties,
                       const std::string &sourceName,
                       const std::string &targetName);

    /// Return an array containing the names of all stored properties
    std::vector<std::string> propertyNames() const;

    /// Return an array containing all named references and their destinations
    std::vector<std::pair<std::string, NamedReference>> namedReferences() const;

    /// Return the list of un-queried attributed
    std::vector<std::string> unqueried() const;

    /**
     * Merge another properties record into the current one.
     * Existing properties will be overwritten with the values
     * from <tt>props</tt> if they have the same name.
     */
    void merge(const Properties &props);

    /// Equality comparison operator
    bool operator==(const Properties &props) const;

    /// Inequality comparison operator
    bool operator!=(const Properties &props) const {
        return !operator==(props);
    }

    MTS_EXPORT_CORE friend
    std::ostream &operator<<(std::ostream &os, const Properties &p);

public:  // Type-specific getters and setters ----------------------------------

    /// Store a boolean value in the Properties instance
    void setBool(const std::string &name, const bool &value, bool warnDuplicates = true);
    /// Retrieve a boolean value
    const bool& bool_(const std::string &name) const;
    /// Retrieve a boolean value (use default value if no entry exists)
    const bool& bool_(const std::string &name, const bool &defVal) const;

    /// Set an integer value in the Properties instance
    void setInt(const std::string &name, const int &value, bool warnDuplicates = true) {
        setLong(name, (int64_t) value, warnDuplicates);
    }
    /// Retrieve an integer value
    int int_(const std::string &name) const { return (int) long_(name); }
    /// Retrieve a boolean value (use default value if no entry exists)
	int int_(const std::string &name, const int &defVal) const {
        return (int) long_(name, (int64_t) defVal);
	}

    /// Store an integer value in the Properties instance
    void setLong(const std::string &name, const int64_t &value, bool warnDuplicates = true);
    /// Retrieve an integer value
    const int64_t& long_(const std::string &name) const;
    /// Retrieve an integer value (use default value if no entry exists)
    const int64_t& long_(const std::string &name, const int64_t &defVal) const;

    /// Store a floating point value in the Properties instance
    void setFloat(const std::string &name, const Float &value, bool warnDuplicates = true);
    /// Retrieve a floating point value
    const Float& float_(const std::string &name) const;
    /// Retrieve a floating point value (use default value if no entry exists)
    const Float& float_(const std::string &name, const Float &defVal) const;

    /// Store a string in the Properties instance
    void setString(const std::string &name, const std::string &value, bool warnDuplicates = true);
    /// Retrieve a string value
    const std::string& string(const std::string &name) const;
    /// Retrieve a string value (use default value if no entry exists)
    const std::string& string(const std::string &name, const std::string &defVal) const;

    /// Store a named reference in the Properties instance
    void setNamedReference(const std::string &name, const NamedReference &value, bool warnDuplicates = true);
    /// Retrieve a named reference value
    const NamedReference& namedReference(const std::string &name) const;
    /// Retrieve a named reference value (use default value if no entry exists)
    const NamedReference& namedReference(const std::string &name, const NamedReference &defVal) const;

    /// Store a 3D vector in the Properties instance
    void setVector3f(const std::string &name, const Vector3f &value, bool warnDuplicates = true);
    /// Retrieve a 3D vector
    const Vector3f& vector3f(const std::string &name) const;
    /// Retrieve a 3D vector (use default value if no entry exists)
    const Vector3f& vector3f(const std::string &name, const Vector3f &defVal) const;

    /// Store a 3D point in the Properties instance
    void setPoint3f(const std::string &name, const Point3f &value, bool warnDuplicates = true);
    /// Retrieve a 3D point
    const Point3f& point3f(const std::string &name) const;
    /// Retrieve a 3D point (use default value if no entry exists)
    const Point3f& point3f(const std::string &name, const Point3f &defVal) const;

    /// Store an arbitrary object in the Properties instance
    void setObject(const std::string &name, const ref<Object> &value, bool warnDuplicates = true);
    /// Retrieve an arbitrary object
    const ref<Object>& object(const std::string &name) const;
    /// Retrieve an arbitrary object (use default value if no entry exists)
    const ref<Object>& object(const std::string &name, const ref<Object> &defVal) const;
private:
    struct PropertiesPrivate;
    std::unique_ptr<PropertiesPrivate> d;
};

NAMESPACE_END(mitsuba)
