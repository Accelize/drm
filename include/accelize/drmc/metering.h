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

    \brief Header for using MeteringSessionManager feature of the DRMLib
    \note This C API is wrapping C++ API, please refer to C++ API
*/

#ifndef _H_ACCELIZE_METERING_CWRAPPER
#define _H_ACCELIZE_METERING_CWRAPPER

#include "accelize/drmc/errorcode.h"
#include "accelize/drmc/common.h"

#ifdef  __cplusplus
    extern "C" {
#endif

#include "stdint.h"

struct MeteringSessionManager_s;

/** \brief Wrapper struct around C++ Accelize::DRMLib::MeteringSessionManager */
typedef struct MeteringSessionManager_s MeteringSessionManager;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::ReadReg32ByOffsetHandler

    \note user_p is the user pointer provided at construction to MeteringSessionManager_alloc
*/
typedef int/*errcode*/ (*ReadReg32ByOffsetHandler )(uint32_t /*register offset*/, uint32_t* /*returned data*/, void* user_p);
/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::WriteReg32ByOffsetHandler

    \note user_p is the user pointer provided at construction to MeteringSessionManager_alloc
*/
typedef int/*errcode*/ (*WriteReg32ByOffsetHandler)(uint32_t /*register offset*/, uint32_t /*data to write*/, void* user_p);
/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::ErrorCallBackHandler

    \note user_p is the user pointer provided at construction to MeteringSessionManager_alloc
*/
typedef void           (*ErrorCallBackHandler     )(const char* /*error message*/, void* user_p);

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::MeteringSessionManager constructor

    \param[in] **p_m : pointer to a MeteringSessionManager pointer that will be set to the new constructed object
    \param[in] conf_file_path, cred_file_path, f_drm_read32, f_drm_write32, f_error_cb : see C++ API documentation
    \param[in] user_p : user pointer that will be passed to the callback functions
*/
DRMLibErrorCode MeteringSessionManager_alloc(MeteringSessionManager **p_m, const char* conf_file_path, const char* cred_file_path, ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb, void* user_p) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::~MeteringSessionManager destructor

    \param[in] **p_m : pointer to a MeteringSessionManager pointer that will be freed. After *p_m == NULL.
*/
DRMLibErrorCode MeteringSessionManager_free(MeteringSessionManager **p_m) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::start_session

    \param[in] *m : pointer to a MeteringSessionManager object
*/
DRMLibErrorCode MeteringSessionManager_start_session(MeteringSessionManager *m) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::stop_session

    \param[in] *m : pointer to a MeteringSessionManager object
*/
DRMLibErrorCode MeteringSessionManager_stop_session(MeteringSessionManager *m) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::pause_session

    \param[in] *m : pointer to a MeteringSessionManager object
*/
DRMLibErrorCode MeteringSessionManager_pause_session(MeteringSessionManager *m) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::resume_session

    \param[in] *m : pointer to a MeteringSessionManager object
*/
DRMLibErrorCode MeteringSessionManager_resume_session(MeteringSessionManager *m) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::auto_start_session

    \param[in] *m : pointer to a MeteringSessionManager object
*/
DRMLibErrorCode MeteringSessionManager_auto_start_session(MeteringSessionManager *m) DRMLIB_EXPORT;

/** \brief Wrapper typedef around C++ Accelize::DRMLib::MeteringSessionManager::dump_drm_hw_report

    \param[in] *m : pointer to a MeteringSessionManager object
*/
DRMLibErrorCode MeteringSessionManager_dump_drm_hw_report(MeteringSessionManager *m) DRMLIB_EXPORT;

#ifdef  __cplusplus
}
#endif

#endif /* _H_ACCELIZE_METERING_CWRAPPER */
