/*
Copyright (C) 2018, Accelize

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
#include <sstream>
#include <cstdlib>
#include <stdio.h>
#include <string.h>

#include "accelize/drm/error.h"

namespace Accelize {
namespace DRM {

// Remove path from filename
#ifdef _WIN32
#define __SHORT_FILE__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif


static const char* s_ShortLogFormat = "%-7s: ";
static const char* s_LongLogFormat  = "%-7s [DRM-Lib]: %s, %s:%u - ";

static std::ostream& gLog() {
    return std::cout;
}

enum class eLogLevel {
    NONE = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4,
    DEBUG2 = 5,
    __SIZE
};

enum class eLogFormat {
    SHORT = 0,
    LONG = 1,
    __SIZE
};

__attribute__((unused)) static eLogLevel getLogLevel() {
    eLogLevel ret = eLogLevel::INFO;
    const char* verbose_env = std::getenv("ACCELIZE_DRM_VERBOSE");
    if (verbose_env) {
        try {
            unsigned long uint_verbose_env = std::stoul(verbose_env);
            if (uint_verbose_env < static_cast<unsigned long>(eLogLevel::__SIZE))
                ret = static_cast<eLogLevel>(uint_verbose_env);
        } catch(...) {} //fall back to default
    }
    return ret;
}

static const char* getLogFormat() {
    std::string ret = s_ShortLogFormat;
    const char* logformat_env = std::getenv("ACCELIZE_DRM_LOG_FORMAT");
    if (logformat_env) {
        try {
            unsigned long uint_logformat_env = std::stoul(logformat_env);
            if (uint_logformat_env == static_cast<unsigned long>(eLogFormat::LONG))
                ret = s_LongLogFormat;
        } catch(...) {} //fall back to default
    }
    return ret.c_str();
}

static void ssAddToStream(std::ostream& a_stream) {(void)a_stream;}

template<typename T, typename... Args>
static void ssAddToStream(std::ostream& a_stream, T&& a_value, Args&&... a_args)
{
    a_stream << std::forward<T>(a_value);
    ssAddToStream(a_stream, std::forward<Args>(a_args)...);
}

template<typename... Args>
static std::string stringConcat(Args&&... a_args)
{
    std::ostringstream ss;
    ssAddToStream(ss, std::forward<Args>(a_args)...);
    return ss.str();
}

static char* getFormattedTime(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Must be static, otherwise won't work
    static char _retval[20];
    strftime(_retval, sizeof(_retval), "%Y-%m-%d/%H:%M:%S", timeinfo);

    return _retval;
}

template <typename... Args>
void logTrace(const char* level, const char* file, const unsigned long noline, Args&&... args) {
    char logPrefix [128];
    sprintf(logPrefix, getLogFormat(), level, getFormattedTime(), file, noline);
    ssAddToStream(gLog(), logPrefix, std::forward<Args>( args )...);
    gLog() << std::endl;
}

#define Debug2(...) \
    do { if (getLogLevel() >= eLogLevel::DEBUG2) \
        logTrace("DEBUG2", __SHORT_FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define Debug(...) \
    do { if (getLogLevel() >= eLogLevel::DEBUG) \
        logTrace("DEBUG", __SHORT_FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define Info(...) \
    do { if (getLogLevel() >= eLogLevel::INFO) \
        logTrace("INFO", __SHORT_FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define Warning(...) \
    do { if (getLogLevel() >= eLogLevel::WARNING) \
        logTrace("WARNING", __SHORT_FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define Error(...) \
    do { if (getLogLevel() >= eLogLevel::ERROR) \
        logTrace("ERROR", __SHORT_FILE__, __LINE__, ##__VA_ARGS__); } while(0)


template <typename... Args>
[[noreturn]] void Throw(DRM_ErrorCode errcode, Args&&... args ) {
    throw Exception(errcode, stringConcat(std::forward<Args>( args )...));
}

}
}

#define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)

#define _WarningAssert(expr, ...) \
    if(expr) {} else {Warning("An assertion in DRM was not verified in ", __FILENAME__, ":", __LINE__, " (", __func__, ") : ", __VA_ARGS__, ". Please contact support, the library may not work as expected.");}
#define _Assert(expr, ...) \
    if(expr) {} else {Throw(DRM_Assert, "An assertion failed in DRM Library in ", __FILENAME__, ":", __LINE__, " (", __func__, ") : ", __VA_ARGS__, ". Please contact support.");}

#define __Assert(F, expr) F(expr, #expr)
#define __Assert2op(F, a, b, _op_) \
    do { auto _a=a; auto _b=b; \
      F(_a _op_ _b, #a, " (= ", _a, ") " , #_op_, " ", #b " (= ", _b, ")"); \
  } while(0);

#define WarningAssert(expr)             __Assert(_WarningAssert, expr)
#define WarningAssertEqual(a, b)        __Assert2op(_WarningAssert, a, b, ==)
#define WarningAssertGreater(a, b)      __Assert2op(_WarningAssert, a, b, >)
#define WarningAssertGreaterEqual(a, b) __Assert2op(_WarningAssert, a, b, >=)
#define WarningAssertLess(a, b)         __Assert2op(_WarningAssert, a, b, <)
#define WarningAssertLessEqual(a, b)    __Assert2op(_WarningAssert, a, b, <=)
#define Assert(expr)                    __Assert(_Assert, expr)
#define AssertEqual(a, b)               __Assert2op(_Assert, a, b, ==)
#define AssertGreater(a, b)             __Assert2op(_Assert, a, b, >)
#define AssertGreaterEqual(a, b)        __Assert2op(_Assert, a, b, >=)
#define AssertLess(a, b)                __Assert2op(_Assert, a, b, <)
#define AssertLessEqual(a, b)           __Assert2op(_Assert, a, b, <=)

#define Unreachable( msg ) \
    Throw(DRM_Assert, "Reached an unexpected part of code in ", __FILENAME__, ":", __LINE__, " (", __func__, ") because ", msg, ": Please contact support.")


#endif // _H_ACCELIZE_DRM_LOG
