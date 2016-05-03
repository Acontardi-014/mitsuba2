#include <mitsuba/core/util.h>
#include <mitsuba/core/logger.h>
#include <sstream>
#include <cmath>

#if defined(__OSX__)
#  include <sys/sysctl.h>
#elif defined(__WINDOWS__)
#  include <windows.h>
#endif

NAMESPACE_BEGIN(mitsuba)
NAMESPACE_BEGIN(util)

#if defined(__WINDOWS__)
std::string getLastError() {
    DWORD errCode = GetLastError();
    char *errorText = nullptr;
    if (!FormatMessageA(
          FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &errorText,
        0,
        nullptr)) {
        return "Internal error while looking up an error code";
    }
    std::string result(errorText);
    LocalFree(errorText);
    return result;
}
#endif

int getCoreCount() {
#if defined(__WINDOWS__)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwNumberOfProcessors;
#elif defined(__OSX__)
    int nprocs;
    size_t nprocsSize = sizeof(int);
    if (sysctlbyname("hw.activecpu", &nprocs, &nprocsSize, NULL, 0))
        Log(EError, "Could not detect the number of processors!");
    return (int)nprocs;
#else
    return sysconf(_SC_NPROCESSORS_CONF);
#endif
}

std::string timeString(Float value, bool precise) {
    struct Order { Float factor; const char* suffix; };
    const Order orders[] = {
        { 0,    "ms"}, { 1000, "s"},
        { 60,   "m"},  { 60,   "h"},
        { 24,   "d"},  { 7,    "w"},
        { (Float) 52.1429,     "y"}
    };

    if (std::isnan(value))
        return "nan";
    else if (std::isinf(value))
        return "inf";
    else if (value < 0)
        return "-" + timeString(-value, precise);

    int i = 0;
    for (i = 0; i < 6 && value > orders[i+1].factor; ++i)
        value /= orders[i+1].factor;

    std::ostringstream os;
    os.precision(precise ? 4 : 1);
    os << value << orders[i].suffix;
    return os.str();
}

std::string memString(size_t size, bool precise) {
    const char *orders[] = {
        "B", "KiB", "MiB", "GiB",
        "TiB", "PiB", "EiB"
    };
    Float value = (Float) size;

    int i = 0;
    for (i = 0; i < 6 && value > 1024.0f; ++i)
        value /= 1024.0f;

    std::ostringstream os;
    os.precision(precise ? 4 : 1);
    os << value << " " << orders[i];
    return os.str();
}

NAMESPACE_END(util)
NAMESPACE_END(mitsuba)
