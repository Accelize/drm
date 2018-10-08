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

    \brief Header defining all error codes of DRMLib
*/

#ifndef _H_ACCELIZE_METERING_ERRORCODE
#define _H_ACCELIZE_METERING_ERRORCODE

#ifdef  __cplusplus
    extern "C" {
#endif

/** \brief Error code enum */
typedef enum {
    DRMLibOK               = 0, /**< Function returned successfully */

    DRMBadArg              = 00001, /**< Bad argument provided */
    DRMBadFormat           = 00002, /**< Bad format of provided input or config file */
    DRMExternFail          = 00003, /**< Fail happened in an external library */
    DRMBadUsage            = 00004, /**< Wrong usage of the DRMLib */

    DRMWSRespError         = 10001, /**< A malformed response has been received from Accelize WebService */
    DRMWSReqError          = 10002, /**< Failed during HTTP request to Accelize WebService */
    DRMWSError             = 10003, /**< Error returned from Accelize WebService */
    DRMWSMayRetry          = 10004, /**< Error with request to Accelize Webservice, retry advised */

    DRMCtlrError           = 20001, /**< An error happened on a command on the DRM controller */

    DRMLibFatal            = 90001,  /**< Fatal error, unknown error (Please contact Accelize) */
    DRMLibAssert           = 90002   /**< Assertion failed internally (Please contact Accelize) */
} DRMLibErrorCode;

#ifdef  __cplusplus
}
#endif

#endif /* _H_ACCELIZE_METERING_ERRORCODE */
