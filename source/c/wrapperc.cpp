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

#include "accelize/drmc.h"
#include "accelize/drm.h"
#include "log.h"

namespace cpp = Accelize::DRM;

struct DrmManager_s {
    cpp::DrmManager *obj;
};

const char * DrmManager_getApiVersion() {
    return cpp::getApiVersion();
}

void checkPointer(void *p) {
    if(p==NULL)
        cpp::Throw(DRM_BadArg, "Provided pointer is NULL");
}

/* Help macros TRY/CATCH to return code error */
#define TRY \
    DRM_ErrorCode __try_ret = DRM_OK; \
    try{

#define _CATCH \
    } catch(const cpp::Exception& e) { \
        if (cpp::getLogLevel() >= cpp::eLogLevel::ERROR) \
            cpp::logTrace("ERROR", __SHORT_FILE__, __LINE__, e.what()); \
        __try_ret = e.getErrCode(); \
    } catch(const std::exception& e) { \
        if (cpp::getLogLevel() >= cpp::eLogLevel::ERROR) \
            cpp::logTrace("ERROR", __SHORT_FILE__, __LINE__, e.what()); \
        __try_ret = DRM_Fatal; \
    }

#define CATCH_RETURN \
    _CATCH; \
    return __try_ret;

// Memory management
DRM_ErrorCode DrmManager_alloc(DrmManager **p_m,
        const char* conf_file_path,
        const char* cred_file_path,
        ReadRegisterCallback f_drm_read32,
        WriteRegisterCallback f_drm_write32,
        AsynchErrorCallback f_error_cb,
        void* user_p) {
    TRY
        DrmManager *m;
        m = (decltype(m))malloc(sizeof(*m));
        m->obj = new cpp::DrmManager(conf_file_path, cred_file_path,
                                [user_p, f_drm_read32](uint32_t offset, uint32_t* value) {return f_drm_read32(offset, value, user_p);},
                                [user_p, f_drm_write32](uint32_t offset, uint32_t value) {return f_drm_write32(offset, value, user_p);},
                                [user_p, f_error_cb](const std::string& msg) {f_error_cb(msg.c_str(), user_p);}
                            );
        *p_m = m;
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_free(DrmManager **p_m) {
    TRY
        checkPointer(p_m);
        DrmManager *m = *p_m;
        checkPointer(m);
        delete m->obj;
        free(m);
        *p_m = NULL;
    CATCH_RETURN
}

// Methods
DRM_ErrorCode DrmManager_activate( DrmManager *m, bool resume_session_request ) {
    TRY
        checkPointer(m);
        m->obj->activate( resume_session_request );
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_deactivate( DrmManager *m, bool pause_session_request ) {
    TRY
        checkPointer(m);
        m->obj->deactivate( pause_session_request );
    CATCH_RETURN
}


DRM_ErrorCode DrmManager_get_int( DrmManager *m, const DrmParameterKey key, int* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->obj->get<int32_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_uint( DrmManager *m, const DrmParameterKey key, unsigned int* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->obj->get<uint32_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_int64( DrmManager *m, const DrmParameterKey key, long long* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->obj->get<int64_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_uint64( DrmManager *m, const DrmParameterKey key, unsigned long long* p_value ) {
    TRY
    checkPointer(m);
    *p_value = m->obj->get<uint64_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_float( DrmManager *m, const DrmParameterKey key, float* p_value ) {
    TRY
    checkPointer(m);
    *p_value = m->obj->get<float>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_double( DrmManager *m, const DrmParameterKey key, double* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->obj->get<double>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_string( DrmManager *m, const DrmParameterKey key, char** p_value ) {
    TRY
        checkPointer(m);
        std::string json = m->obj->get<std::string>((cpp::ParameterKey)key);
        char *json_out = strdup(json.c_str());
        *p_value = json_out;
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_json_string( DrmManager *m, const char* json_in, char** p_json_out ) {
    TRY
        checkPointer(m);
        std::string json_string(json_in);
        m->obj->get(json_string);
        char *json_out = strdup(json_string.c_str());
        *p_json_out = json_out;
    CATCH_RETURN
}


DRM_ErrorCode DrmManager_set_bool( DrmManager *m, const DrmParameterKey key, const bool value ) {
    TRY
        checkPointer(m);
        m->obj->set<bool>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_int( DrmManager *m, const DrmParameterKey key, const int value ) {
    TRY
        checkPointer(m);
        m->obj->set<int32_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_uint( DrmManager *m, const DrmParameterKey key, const unsigned int value ) {
    TRY
        checkPointer(m);
        m->obj->set<uint32_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_int64( DrmManager *m, const DrmParameterKey key, const long long value ) {
    TRY
        checkPointer(m);
        m->obj->set<int64_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_uint64( DrmManager *m, const DrmParameterKey key, const unsigned long long value ) {
    TRY
        checkPointer(m);
        m->obj->set<uint64_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_float( DrmManager *m, const DrmParameterKey key, const float value ) {
    TRY
        checkPointer(m);
        m->obj->set<float>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_string( DrmManager *m, const DrmParameterKey key, const char* value ) {
    TRY
        checkPointer(m);
        m->obj->set<std::string>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_json_string( DrmManager *m, const char* json_string ) {
TRY
    checkPointer(m);
    std::string json(json_string);
    m->obj->set(json);
CATCH_RETURN
}
