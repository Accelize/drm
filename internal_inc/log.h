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
#include <cstdlib>

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
    DEBUG = 4,
    DEBUG2 = 5,
    __SIZE
};

static eLogLevel getLogLevel() {
    static eLogLevel logLevel = [](){
        eLogLevel ret = eLogLevel::INFO;

        const char* verbose_env = std::getenv("ACCELIZE_DRMLIB_VERBOSE");
        if(verbose_env) {
            try{
                unsigned long uint_verbose_env = std::stoul(verbose_env);
                if(uint_verbose_env < static_cast<unsigned long>(eLogLevel::__SIZE))
                    ret = static_cast<eLogLevel>(uint_verbose_env);
            }catch(...){
                //fall back to default
            }
        }

        return ret;
    }();
    return logLevel;
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

static bool isDebug() {
    if(getLogLevel() < eLogLevel::DEBUG) return false;
    return true;
}

template <typename... Args>
void Debug( Args&&... args ) {
    if(!isDebug()) return;
    ssAddToStream(gLog(), "[DEBUG] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Debug2( Args&&... args ) {
    if(getLogLevel() < eLogLevel::DEBUG2) return;
    ssAddToStream(gLog(), "[DEBUG2] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Info( Args&&... args ) {
    if(getLogLevel() < eLogLevel::INFO) return;
    ssAddToStream(gLog(), "[INFO] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Warning( Args&&... args ) {
    if(getLogLevel() < eLogLevel::ERROR) return;
    ssAddToStream(gLog(), "[WARNING] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
void Error( Args&&... args ) {
    if(getLogLevel() < eLogLevel::ERROR) return;
    ssAddToStream(gLog(), "[ERROR] ", std::forward<Args>( args )...);
    gLog() << std::endl;
}

template <typename... Args>
[[noreturn]] void Throw(DRMLibErrorCode errcode, Args&&... args ) {
    throw Exception(errcode, stringConcat(std::forward<Args>( args )...));
}

}
}

#define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)

#define _WarningAssert(expr, ...) \
    if(expr) {} else {Warning("An assertion in DRMLib was not verified in ", __FILENAME__, ":", __LINE__, " (", __func__, ") : ", __VA_ARGS__, ". Please contact support, the library may not work as expected.");}
#define _Assert(expr, ...) \
    if(expr) {} else {Throw(DRMLibAssert, "An assertion failed in DRMLib in ", __FILENAME__, ":", __LINE__, " (", __func__, ") : ", __VA_ARGS__, ". Please contact support.");}

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

#define Unreachable() \
    Throw(DRMLibAssert, "An unreachable part of code has be reached in DRMLib in ", __FILENAME__, ":", __LINE__, " (", __func__, ") : Please contact support.")


#endif // _H_ACCELIZE_METERING_LOG
