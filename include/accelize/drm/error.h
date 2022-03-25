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

#ifndef _H_ACCELIZE_DRM_ERROR
#define _H_ACCELIZE_DRM_ERROR

#include <stdexcept>

#include "accelize/drmc/errorcode.h" // enum with error codes
#include "accelize/drmc/common.h"

namespace Accelize {
namespace DRM {

/** \brief Exception class with error code for the DRM Library

    This class is an exception that may be thrown by the DRM Library in case of synchronous error
*/
class DRM_EXPORT Exception : public std::runtime_error {
protected:
    DRM_ErrorCode errCode; /**< error code from the DRM_ErrorCode enum */
    mutable std::string errWhat; /**< internal error message to be accessed from what() */

public:
    template <class S>
    Exception(DRM_ErrorCode errCode, S&& errMsg)
        : std::runtime_error(std::forward<S>(errMsg)), errCode(errCode) {}
    virtual ~Exception() {};
    DRM_ErrorCode getErrCode() const;
    virtual const char* what() const noexcept;
};

} //namespace DRM
} //namespace Accelize

#endif // _H_ACCELIZE_DRM_ERROR
