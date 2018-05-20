#pragma once

#include <mitsuba/mitsuba.h>
#include <string>
#include <vector>

/// Max level of nested <include> directives
#define MTS_XML_INCLUDE_MAX_RECURSION 15

NAMESPACE_BEGIN(mitsuba)
NAMESPACE_BEGIN(xml)

/// Used to pass key=value pairs to the parser
using ParameterList = std::vector<std::pair<std::string, std::string>>;

/// Load a Mitsuba scene from an XML file
extern MTS_EXPORT_CORE ref<Object> load_file(const fs::path &path,
                                             ParameterList parameters = ParameterList());

/// Load a Mitsuba scene from an XML string
extern MTS_EXPORT_CORE ref<Object> load_string(const std::string &string,
                                               ParameterList parameters = ParameterList());

NAMESPACE_END(xml)
NAMESPACE_END(mitsuba)
