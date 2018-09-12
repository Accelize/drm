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

#ifndef _H_ACCELIZE_METERING_ERROR
#define _H_ACCELIZE_METERING_ERROR

#include <stdexcept>

#include "accelize/drmc/errorcode.h" // enum with error codes

namespace Accelize {
namespace DRMLib {

/** \brief Exception class with error code for the DRMLib

    This class is an exception that may be thrown by the DRMLib in case of synchronous error
*/
class Exception : public std::runtime_error {
protected:
    DRMLibErrorCode errCode; /**< error code from the DRMLibErrorCode enum */
    mutable std::string errWhat; /**< internal error message to be accessed from what() */

public:
    template <class S>
    Exception(DRMLibErrorCode errCode, S&& errMsg) : std::runtime_error(std::forward<S>(errMsg)), errCode(errCode) {}
    DRMLibErrorCode getErrCode() const;
    virtual const char* what() const noexcept;
};

} //namespace DRMLib
} //namespace Accelize

#endif // _H_ACCELIZE_METERING_ERROR
