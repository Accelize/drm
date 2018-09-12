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

#ifndef _H_ACCELIZE_METERING_LOG
#define _H_ACCELIZE_METERING_LOG

#include <iostream>
#include <sstream>

#include "accelize/drm/error.h"

namespace Accelize {
namespace DRMLib {

static std::ostream& gLog() {
    return std::cout;
}

enum class eLogLevel {
    NONE = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4
};
static eLogLevel gLogLevel = eLogLevel::INFO;


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

template <typename... Args>
void Debug( Args&&... args ) {
    if(gLogLevel < eLogLevel::DEBUG) return;
    ssAddToStream(gLog(), "[DEBUG] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Info( Args&&... args ) {
    if(gLogLevel < eLogLevel::INFO) return;
    ssAddToStream(gLog(), "[INFO] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Warning( Args&&... args ) {
    if(gLogLevel < eLogLevel::ERROR) return;
    ssAddToStream(gLog(), "[WARNING] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Error( Args&&... args ) {
    if(gLogLevel < eLogLevel::ERROR) return;
    ssAddToStream(gLog(), "[ERROR] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
[[noreturn]] void Throw(DRMLibErrorCode errcode, Args&&... args ) {
    throw Exception(errcode, stringConcat(std::forward<Args>( args )...));
}

}
}

#define Assert(expr) \
    if(expr) {} else {Throw(DRMLibAssert, "An assertion failed in DRMLib in ", __FILE__, ":", __LINE__, " (", __func__, ") : ", #expr, ". Please contact support.");}

#define Unreachable() \
    Throw(DRMLibAssert, "An unreachable part of code has be reached in DRMLib in ", __FILE__, ":", __LINE__, " (", __func__, ") : Please contact support.")


#endif // _H_ACCELIZE_METERING_LOG
