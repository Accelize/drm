/*
Copyright (C) 2022, Accelize

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _H_ACCELIZE_DRM_LOG
#define _H_ACCELIZE_DRM_LOG

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>

// SPDLOG_ACTIVE_LEVEL must be declared before the spdlog.h include
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/null_sink.h"
#include "spdlog/sinks/basic_file_sink.h"       // support for basic file logging
#include "spdlog/sinks/rotating_file_sink.h"    // support for rotating file logging
#pragma GCC diagnostic pop

#include "accelize/drm/error.h"


namespace Accelize {
namespace DRM {

    // Remove path from filename
    #ifdef _WIN32
    #define __SHORT_FILE__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
    #else
    #define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #endif

    extern std::shared_ptr<spdlog::logger> sLogger;

    #define Debug2(...) SPDLOG_TRACE( __VA_ARGS__ )

    #define Debug(...) SPDLOG_DEBUG( __VA_ARGS__ )

    #define Info(...) SPDLOG_INFO( __VA_ARGS__ )

    #define Warning(...) SPDLOG_WARN( __VA_ARGS__ )

    #define Error(...) SPDLOG_ERROR( __VA_ARGS__ )

    #define Fatal(...) SPDLOG_CRITICAL( __VA_ARGS__ )

}
}

#endif // _H_ACCELIZE_DRM_LOG
