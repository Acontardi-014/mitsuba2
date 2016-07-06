#pragma once

#include <mitsuba/mitsuba.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ostream>

NAMESPACE_BEGIN(mitsuba)
NAMESPACE_BEGIN(string)

/// Check if the given string starts with a specified prefix
inline bool startsWith(const std::string &string, const std::string &prefix) {
    if (prefix.size() > string.size())
        return false;
    return std::equal(prefix.begin(), prefix.end(), string.begin());
}

/// Check if the given string ends with a specified suffix
inline bool endsWith(const std::string &string, const std::string &suffix) {
    if (suffix.size() > string.size())
        return false;
    return std::equal(suffix.rbegin(), suffix.rend(), string.rbegin());
}

/// Return a lower-case version of the given string (warning: not unicode compliant)
inline std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

/// Return a upper-case version of the given string (warning: not unicode compliant)
inline std::string toUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

/// Chop up the string given a set of delimiters (warning: not unicode compliant)
inline std::vector<std::string> tokenize(const std::string &string,
                                         const std::string &delim = ", ",
                                         bool includeEmpty = false) {
    std::string::size_type lastPos = 0, pos = string.find_first_of(delim, lastPos);
    std::vector<std::string> tokens;

    while (lastPos != std::string::npos) {
        if (pos != lastPos || includeEmpty)
            tokens.push_back(string.substr(lastPos, pos - lastPos));
        lastPos = pos;
        if (lastPos != std::string::npos) {
            lastPos += 1;
            pos = string.find_first_of(delim, lastPos);
        }
    }

    return tokens;
}

/// Indent every line of a string by some number of spaces
inline std::string indent(const std::string &string, int amount = 2) {
    std::string result;
    result.reserve(string.size());
    for (size_t i = 0; i<string.length(); ++i) {
        char ch = string[i];
        result += ch;
        if (ch == '\n') {
            for (int j = 0; j < amount; ++j)
                result += ' ';
        }
    }
    return result;
}

/// Turns a vector of elements into a human-readable representation
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    auto it = v.begin();
    while (it != v.end()) {
        os << *it;
        it++;
        if (it != v.end())
            os << ", ";
    }
    return os;
}

inline std::string trim(const std::string &str,
                        const std::string &whitespace = " \t") {
    auto it1 = str.find_first_not_of(whitespace);
    if (it1 == std::string::npos)
        return "";
    auto it2 = str.find_last_not_of(whitespace);
    return str.substr(it1, it2 - it1 + 1);
}

NAMESPACE_END(string)
NAMESPACE_END(mitsuba)

/// Turns an array into a human-readable representation
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    auto it = v.begin();
    while (it != v.end()) {
        os << *it;
        it++;
        if (it != v.end())
            os << ", ";
    }
    return os;
}

