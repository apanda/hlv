// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <map>
#include <string>
#include <cstdlib>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>
#ifndef __LOGGING_COMMON_H__

/// This header file is not like the others. It is more generic than just HLV,
/// setting up logging etc.
namespace logging = boost::log;
void init_logging () {
    std::map<std::string, logging::trivial::severity_level> translate = {
        {"trace", logging::trivial::trace},
        {"debug", logging::trivial::debug},
        {"info",  logging::trivial::info},
        {"warning", logging::trivial::warning},
        {"fatal", logging::trivial::fatal},
        {"error", logging::trivial::error}
    };

    char* var = std::getenv ("LOG_LEVEL");
    logging::trivial::severity_level level = logging::trivial::info;
    if (var != NULL) {
        level = translate[std::string(var)];
    }
    logging::core::get()->set_filter
    (
       logging::trivial::severity >= level
    );
}
#endif
