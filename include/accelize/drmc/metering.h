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

/** \file accelize/drmc/metering.h

    \brief Header for using DrmManager feature of the DRM
    \note This C API is wrapping C++ API, please refer to C++ API
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

/** \brief Wrapper struct around C++ Accelize::DRM::DrmManager */
struct DrmManager_s;

/** \brief Typedef enum listing the parameters ID accessible from User code */
typedef enum {
#   define PARAMETERKEY_ITEM(id) DRM__##id,
#   include "accelize/drm/ParameterKey.def"
#   undef PARAMETERKEY_ITEM
    DRM__ParameterKeyCount
} DrmParameterKey;

/** \brief Wrapper typedef around structure DrmManager_s */
typedef struct DrmManager_s DrmManager;

/** \brief Wrapper typedef around C++ Accelize::DRM::DrmManager::ReadRegisterByAddrCallback
  \note user_p is the user pointer provided at construction to DrmManager_alloc
*/
typedef int/*errcode*/ (*ReadRegisterCallback)(uint32_t /*register offset*/, uint32_t* /*returned data*/, void* user_p);

/** \brief Wrapper typedef around C++ Accelize::DRM::DrmManager::WriteReg32ByOffsetHandler
  \note user_p is the user pointer provided at construction to DrmManager_alloc
*/
typedef int/*errcode*/ (*WriteRegisterCallback)(uint32_t /*register offset*/, uint32_t /*data to write*/, void* user_p);

/** \brief Wrapper typedef around C++ Accelize::DRM::DrmManager::ErrorCallBackHandler
  \note user_p is the user pointer provided at construction to DrmManager_alloc
*/
typedef void (*AsynchErrorCallback)(const char* /*error message*/, void* user_p);

/** \brief Return version of the library as text */
const char * DrmManager_getApiVersion() DRM_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRM::DrmManager::DrmManager constructor
  \param[in] p_m : pointer to a DrmManager pointer that will be set to the new constructed object
  \param[in] conf_file_path, cred_file_path, f_drm_read32, f_drm_write32, f_error_cb : see C++ API documentation
  \param[in] user_p : user pointer that will be passed to the callback functions
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_alloc(DrmManager **p_m,
        const char* conf_file_path,
        const char* cred_file_path,
        ReadRegisterCallback f_drm_read32,
        WriteRegisterCallback f_drm_write32,
        AsynchErrorCallback f_error_cb,
        void* user_p
) DRM_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRM::DrmManager::~DrmManager destructor
  \param[in] p_m : pointer to a DrmManager pointer that will be freed. After *p_m == NULL.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_free(DrmManager **p_m) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::activate
  This function activate/unlocks the hardware by unlocking the protected IPs in the FPGA and opening a DRM session.
  \param[in] m : pointer to a DrmManager object
  \param[in] resume_session_request : If true, the pending session is reused. If no pending session is found,
     create a new one. If false and a pending session is found, close it and create a new one.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_activate(DrmManager *m, bool resume_session_request ) DRM_EXPORT;

/** \brief Wrapper function around C++ #Accelize::DRM::DrmManager::deactivate
  This function deactivates/locks the hardware back and close the session unless the \p pause_session argument is true.
  In this case, the session is kept opened for later use.
  \param[in] m : pointer to a DrmManager object
  \param[in] pause_session_request : If true, the current session is kept open for later usage. Otherwise, the current session is closed.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_deactivate(DrmManager *m, bool pause_session_request ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for JSON string
  This function accesses the internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] json_in : JSON formatted string listing the parameter names requested as key associated to any value.
        For instance, std::string json_string = "{\"NUM_ACTIVATORS\": null, \"SESSION_ID\": null}";
  \param[out] json_out : Pointer to a char* that will point to a JSON formatted strng containing the parameter names requested
        and their actual values in the DRM system.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
  \note The function will allocated the output string, \p json_out. This is the responsibility to the user to free it: free(json_out)
*/
DRM_ErrorCode DrmManager_get_json_string( DrmManager *m, const char* json_in, char** json_out ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for boolean parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_bool( DrmManager *m, const DrmParameterKey key_id, bool* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for 32-bit integer parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_int( DrmManager *m, const DrmParameterKey key_id, int* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for unsigned 32-bit integer parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_uint( DrmManager *m, const DrmParameterKey key_id, unsigned int* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for 64-bit integer parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_int64( DrmManager *m, const DrmParameterKey key_id, long long* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for unsigned 64-bit integer parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_uint64( DrmManager *m, const DrmParameterKey key_id, unsigned long long* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for float parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_float( DrmManager *m, const DrmParameterKey key_id, float* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for double parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_double( DrmManager *m, const DrmParameterKey key_id, double* p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::get for string parameter
  This function accesses the internal parameter with the enum identifier defined in #DrmParameterKey
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[out] p_value : Pointer to a boolean that will contain the value of the corresponding parameter.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_get_string( DrmManager *m, const DrmParameterKey key_id, char** p_value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for JSON string
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] json_in : JSON formatted string listing the parameter names and corresponding values.
        For instance, std::string json_string = "{\"CUSTOM_FIELD\": 0x12345678}";
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
  \note The function will allocated the output string, \p p_value. This is the responsibility to the user to free it: free(p_value)
*/
DRM_ErrorCode DrmManager_set_json_string( DrmManager *m, const char* json_in ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : boolean value to overwrite with.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_bool  ( DrmManager *m, const DrmParameterKey key_id, const bool value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : 32-bit integer value to overwrite with.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_int   ( DrmManager *m, const DrmParameterKey key_id, const int value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : unsigned 32-bit integer value to overwrite with.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_uint  ( DrmManager *m, const DrmParameterKey key_id, const unsigned int value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : 64-bit integer value to overwrite with.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_int64 ( DrmManager *m, const DrmParameterKey key_id, const long long value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : unsigned 64-bit integer value to overwrite with.
  return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_uint64( DrmManager *m, const DrmParameterKey key_id, const unsigned long long value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : float value to overwrite with.
  \return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_float ( DrmManager *m, const DrmParameterKey key_id, const float value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : double value to overwrite with.
  return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_double( DrmManager *m, const DrmParameterKey key_id, const double value ) DRM_EXPORT;

/** \brief Wrapper function around C++ Accelize::DRM::DrmManager::set for boolean parameter
  This function overwrites an internal parameter of the DRM system.
  \param[in] m : pointer to a DrmManager object
  \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #DrmParameterKey.
  \param[in] value : string value to overwrite with.
  return An error code defined by the enumerator #DRM_ErrorCode indicating the success or the cause of the error
        during the function execution.
*/
DRM_ErrorCode DrmManager_set_string( DrmManager *m, const DrmParameterKey key_id, const char* value ) DRM_EXPORT;

#ifdef  __cplusplus
}
#endif

#endif /* _H_ACCELIZE_DRM_CWRAPPER */
