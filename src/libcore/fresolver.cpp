#include <mitsuba/core/fresolver.h>
#include <sstream>

NAMESPACE_BEGIN(mitsuba)

FileResolver::FileResolver() {
    m_paths.push_back(fs::current_path());
}

fs::path FileResolver::resolve(const fs::path &path) const {
    for (auto const &base : m_paths) {
        fs::path combined = base / path;
        if (fs::exists(combined))
            return combined;
    }
    return path;
}

std::string FileResolver::toString() const {
    std::ostringstream oss;
    oss << "FileResolver[" << std::endl;
    for (size_t i = 0; i < m_paths.size(); ++i) {
        oss << "  \"" << m_paths[i] << "\"";
        if (i + 1 < m_paths.size())
            oss << ",";
        oss << std::endl;
    }
    oss << "]";
    return oss.str();
}

MTS_IMPLEMENT_CLASS(FileResolver, Object)
NAMESPACE_END(mitsuba)
