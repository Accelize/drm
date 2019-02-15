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

/** \file accelize/drmc/errorcode.h

    \brief Header defining all error codes of DRM Library
*/

#ifndef _H_ACCELIZE_DRM_ERRORCODE
#define _H_ACCELIZE_DRM_ERRORCODE

#ifdef  __cplusplus
    extern "C" {
#endif

/** \brief Error code enum */
typedef enum {
    DRM_OK                  = 0,     /**< Function returned successfully */

    DRM_BadArg              = 00001, /**< Bad argument provided */
    DRM_BadFormat           = 00002, /**< Bad format of provided input or config file */
    DRM_ExternFail          = 00003, /**< Fail happened in an external library */
    DRM_BadUsage            = 00004, /**< Wrong usage of the DRM Library */
    DRM_BadFrequency        = 00005, /**< Wrong value of the DRM frequency provided in the configuration file */

    DRM_WSRespError         = 10001, /**< A malformed response has been received from Accelize WebService */
    DRM_WSReqError          = 10002, /**< Failed during HTTP request to Accelize WebService */
    DRM_WSError             = 10003, /**< Error returned from Accelize WebService */
    DRM_WSMayRetry          = 10004, /**< Error with request to Accelize Webservice, retry advised */

    DRM_CtlrError           = 20001, /**< An error happened on a command on the DRM controller */

    DRM_Fatal               = 90001, /**< Fatal error, unknown error (Please contact Accelize) */
    DRM_Assert              = 90002, /**< Assertion failed internally (Please contact Accelize) */

    DRM_Debug               = 90003  /**< Generated for debug and testing only */
} DRM_ErrorCode;

#ifdef  __cplusplus
}
#endif

#endif /* _H_ACCELIZE_DRM_ERRORCODE */
