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

/** \brief Accelize DRM C Library
*/

#ifndef _H_ACCELIZE_DRM_CWRAPPER
#define _H_ACCELIZE_DRM_CWRAPPER

#include "accelize/drmc/errorcode.h"
#include "accelize/drmc/common.h"

#ifdef  __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>


#define MAX_MSG_SIZE 1024


/** \brief Struct handling DRM manager.

    Manage Accelize DRM by handling communication with DRM controller IP
    and Accelize Web service.
*/
struct DrmManager_s;

/** \brief Typedef struct handling DRM manager.

    Manage error message and Accelize DRM by handling communication with DRM controller IP
    and Accelize Web service.
*/
typedef struct {
    char error_message[MAX_MSG_SIZE];
    struct DrmManager_s *drm;
} DrmManager;


/** \brief Typedef enum listing the parameters accessible from User code.
    Some have ready only access while others have read and write access.
*/
typedef enum {
#   define PARAMETERKEY_ITEM(id) DRM__##id,
#   include "accelize/drm/ParameterKey.def"
#   undef PARAMETERKEY_ITEM
    DRM__ParameterKeyCount
} DrmParameterKey;


/** \brief FPGA read register callback function.
    The register offset is relative to first register of DRM controller

    \param[in] register_offset : Register offset relative to DRM controller IP
    base address.
    \param[in] returned_data : Pointer to an integer that will contain the value
    of the corresponding register.
    \param[in] user_p : User pointer.

    \warning This function must be thread-safe in case of concurrency on the
    register bus.
*/
typedef int/*errcode*/ (*ReadRegisterCallback)(uint32_t /*register offset*/, uint32_t* /*returned data*/, void* user_p);


/** \brief FPGA write register callback function.
    The register offset is relative to first register of DRM controller

    \param[in] register_offset : Register offset relative to DRM controller IP
    base address.
    \param[in] data_to_write : Data to write in register.
    \param[in] user_p : User pointer.

    \warning This function must be thread-safe in case of concurrency on the
    register bus.
*/
typedef int/*errcode*/ (*WriteRegisterCallback)(uint32_t /*register offset*/, uint32_t /*data to write*/, void* user_p);


/** \brief Asynchronous Error handling callback function.
    This function is called in case of asynchronous error during operation.

    \param[in] error_message : Error message.
    \param[in] user_p : User pointer.
*/
typedef void (*AsynchErrorCallback)(const char* /*error message*/, void* user_p);


/** \brief Return API version.
*/
const char * DrmManager_getApiVersion() DRM_EXPORT;


/** \brief Instantiate and initialize a DRM manager.

    \see ReadRegisterCallback WriteRegisterCallback AsynchErrorCallback

    \param[in] p_m : Pointer to a DrmManager pointer that will be set to the new
    constructed object.
    \param[in] conf_file_path : Path to the DRM configuration JSON file.
    \param[in] cred_file_path : Path to the user Accelize credential JSON file.
    \param[in] read_register : FPGA read register callback function.
    \param[in] write_register : FPGA write register callback function.
    \param[in] async_error : Asynchronous Error handling callback function.
    \param[in] user_p : User pointer that will be passed to the callback
    functions.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_alloc(DrmManager **p_m,
        const char* conf_file_path,
        const char* cred_file_path,
        ReadRegisterCallback read_register,
        WriteRegisterCallback write_register,
        AsynchErrorCallback async_error,
        void* user_p
) DRM_EXPORT;


/** \brief Free a DRM manager object.

    \param[in] p_m : Pointer to a DrmManager pointer that will be freed.
    After *p_m == NULL.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_free(DrmManager **p_m) DRM_EXPORT;


/** \brief Activate DRM session.

    This function activate/unlocks the hardware by unlocking the protected
    IPs in the FPGA and opening a DRM session.

    If a session is still pending the behavior depends on
    "resume_session_request" argument. If true the session is reused.
    Otherwise the session is closed and a new session is created.

    This function will start a thread that keeps the hardware unlocked by
    automatically updating the license when necessary.

    Any error during this thread execution will be reported asynchronously
    through the "async_error" function, if provided in constructor.

    When this function returns and the license is valid,
    the protected IPs are guaranteed to be unlocked.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] resume_session_request : If true, the pending session is
    reused. If no pending session is found, create a new one. If
    false and a pending session is found, close it and create a new
    one. Set to false to have default behavior.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_activate(DrmManager *m, bool resume_session_request ) DRM_EXPORT;


/** \brief Deactivate DRM session.

    This function deactivates/locks the hardware back and close the session
    unless the "pause_session_request" argument is True. In this case,
    the session is kept opened for later use.

    This function will join the thread keeping the hardware unlocked.

    When the function returns, the hardware are guaranteed to be locked.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] pause_session_request : If true, the current session is kept
    open for later usage. Otherwise, the current session is closed.
    Set to false to have default behavior.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_deactivate(DrmManager *m, bool pause_session_request ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] json_in : JSON formatted string listing the parameter names
        requested as key associated to any value. For instance,
            std::string json_string = "{\"NUM_ACTIVATORS\": null, \"SESSION_ID\": null}";
    \param[out] json_out : Pointer to a char* that will point to a JSON
    formatted string containing the parameter names requested and their actual
    values in the DRM system.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.

    \note The function will allocated the output string, \p json_out. This is
    the responsibility to the user to free it: free(json_out)
*/
DRM_ErrorCode DrmManager_get_json_string( DrmManager *m, const char* json_in, char** json_out ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to a boolean that will contain the value of
    the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_bool( DrmManager *m, const DrmParameterKey key_id, bool* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to an integer that will contain the value of
    the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_int( DrmManager *m, const DrmParameterKey key_id, int* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to an unsigned integer that will contain the
    value of the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_uint( DrmManager *m, const DrmParameterKey key_id, unsigned int* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to a long integer that will contain the value
    of the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_int64( DrmManager *m, const DrmParameterKey key_id, long long* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to an unsigned long integer that will contain
    the value of the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_uint64( DrmManager *m, const DrmParameterKey key_id, unsigned long long* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to a float that will contain the value of
    the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_float( DrmManager *m, const DrmParameterKey key_id, float* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to a double float that will contain the value
    of the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_double( DrmManager *m, const DrmParameterKey key_id, double* p_value ) DRM_EXPORT;


/** \brief Get information from the DRM system.

    This function gives access to the internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[out] p_value : Pointer to a string that will contain the value of
    the corresponding parameter.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_get_string( DrmManager *m, const DrmParameterKey key_id, char** p_value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] json_in : JSON formatted string listing the parameter names and
    corresponding values. For instance,
        std::string json_string = "{\"CUSTOM_FIELD\": 0x12345678}";

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_json_string( DrmManager *m, const char* json_in ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Boolean value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_bool  ( DrmManager *m, const DrmParameterKey key_id, const bool value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Integer value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_int   ( DrmManager *m, const DrmParameterKey key_id, const int value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Unsigned integer value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_uint  ( DrmManager *m, const DrmParameterKey key_id, const unsigned int value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Long integer value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_int64 ( DrmManager *m, const DrmParameterKey key_id, const long long value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Unsigned long integer value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_uint64( DrmManager *m, const DrmParameterKey key_id, const unsigned long long value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Float value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_float ( DrmManager *m, const DrmParameterKey key_id, const float value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : Double float value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_double( DrmManager *m, const DrmParameterKey key_id, const double value ) DRM_EXPORT;


/** \brief Set information of the DRM system.

    This function overwrites an internal parameter of the DRM system.

    \param[in] m : Pointer to a DrmManager object.
    \param[in] key_id : Unique identifier of the parameter to access;
    available IDs are listed in #DrmParameterKey.
    \param[in] value : String value to overwrite with.

    \return An error code defined by the enumerator #DRM_ErrorCode indicating
    the success or the cause of the error during the function execution.
*/
DRM_ErrorCode DrmManager_set_string( DrmManager *m, const DrmParameterKey key_id, const char* value ) DRM_EXPORT;


#ifdef  __cplusplus
}
#endif

#endif /* _H_ACCELIZE_DRM_CWRAPPER */
