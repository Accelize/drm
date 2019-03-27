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
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>

#include "accelize/drm/error.h"

namespace Accelize {
namespace DRM {

// Remove path from filename
#ifdef _WIN32
#define __SHORT_FILE__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define STRINGIFY( name ) #name

enum class eLogLevel: int {
    QUIET = 0,
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    DEBUG2,
    __SIZE
};

static const char* cLogLevelString[] = {
        STRINGIFY( QUIET ),
        STRINGIFY( ERROR ),
        STRINGIFY( WARNING ),
        STRINGIFY( INFO ),
        STRINGIFY( DEBUG ),
        STRINGIFY( DEBUG2 )
};

enum class eLogFormat {
    SHORT = 0,
    LONG = 1,
    __SIZE
};


extern eLogLevel sLogVerbosity;
extern eLogFormat sLogFormat;
extern std::string sLogFilePath;
//extern std::unique_ptr<std::ostream> sLogStream;
extern std::ostream* sLogStream;

static std::recursive_mutex mLogMutex;



static std::string getFormattedTime() {

    time_t rawtime;
    struct tm* timeinfo;
    char _retval[32];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(_retval, sizeof(_retval), "%Y-%m-%d, %H:%M:%S", timeinfo);

    return std::string( _retval );
}


__attribute__((unused)) static void ssAddToStream(std::ostream& a_stream) { (void)a_stream; }

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


template <typename... Args>
[[noreturn]] void Throw(DRM_ErrorCode errcode, Args&&... args ) {
    throw Exception(errcode, stringConcat(std::forward<Args>( args )...));
}


template <typename... Args>
void logTrace(const eLogLevel& level, const std::string& file, const unsigned long noline, Args&&... args) {
    std::stringstream ss;
/*    ss << std::left << std::setfill(' ') << std::setw(7) << cLogLevelString[(int)level];
    if ( sLogFormat == eLogFormat::LONG ) {
        ss << " [DRM-Lib] " << getFormattedTime() << ", " << file << ", " << noline;
    }
    ss << ": [Thread " << std::this_thread::get_id() << "] ";
*/
    if ( sLogFormat == eLogFormat::LONG ) {
        ss << getFormattedTime() << " - DRM-Lib, " << std::left << std::setfill(' ') << std::setw(16) << file;
        ss << ", " << std::right << std::setfill(' ') << std::setw(4) << noline << ": ";
    }
    ss << "Thread " << std::this_thread::get_id() << " - ";
    ss << std::left << std::setfill(' ') << std::setw(7) << cLogLevelString[(int)level] << ", ";
    std::lock_guard<std::recursive_mutex> lock(mLogMutex);
//    if ( sLogStream.get() != nullptr ) {
    if ( sLogStream != nullptr ) {
        ssAddToStream(*sLogStream, ss.str(), std::forward<Args>(args)...);
        *sLogStream << std::endl;
    }
}

#define Debug2(...) \
    do { if ( sLogVerbosity >= eLogLevel::DEBUG2 ) \
        logTrace( eLogLevel::DEBUG2, __SHORT_FILE__, __LINE__, ##__VA_ARGS__ ); } while(0)

#define Debug(...) \
    do { if ( sLogVerbosity >= eLogLevel::DEBUG ) \
        logTrace( eLogLevel::DEBUG, __SHORT_FILE__, __LINE__, ##__VA_ARGS__ ); } while(0)

#define Info(...) \
    do { if ( sLogVerbosity >= eLogLevel::INFO ) \
        logTrace( eLogLevel::INFO, __SHORT_FILE__, __LINE__, ##__VA_ARGS__ ); } while(0)

#define Warning(...) \
    do { if ( sLogVerbosity >= eLogLevel::WARNING ) \
        logTrace( eLogLevel::WARNING, __SHORT_FILE__, __LINE__, ##__VA_ARGS__ ); } while(0)

#define Error(...) \
    do { if ( sLogVerbosity >= eLogLevel::ERROR ) \
        logTrace( eLogLevel::ERROR, __SHORT_FILE__, __LINE__, ##__VA_ARGS__ ); } while(0)


    void initLog();
    void uninitLog();

}
}


#define Unreachable( msg ) \
    Throw( DRM_Assert, "Reached an unexpected part of code in ", __SHORT_FILE__, ":", \
            __LINE__, " (", __func__, ") because ", msg, ": Please contact support." )


#endif // _H_ACCELIZE_DRM_LOG
