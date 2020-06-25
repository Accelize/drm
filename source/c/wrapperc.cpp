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
    if ( p == NULL )
        Throw( DRM_BadArg, spdlog::level::err, "Provided pointer is NULL" );
}

/* Help macros TRY/CATCH to return code error */
#define TRY                                        \
    DRM_ErrorCode __try_ret = DRM_OK;              \
    if ( m != NULL )                               \
        memset(m->error_message, 0, MAX_MSG_SIZE); \
    try {

#define CATCH_RETURN                                          \
    } catch( const cpp::Exception& e ) {                      \
        int cp_size = strlen( e.what() );                     \
        if ( cp_size >= MAX_MSG_SIZE ) {                      \
            cp_size = MAX_MSG_SIZE - 6;                       \
            strcpy( m->error_message + cp_size, "[...]" );    \
        }                                                     \
        strncpy( m->error_message, e.what(), cp_size );       \
        __try_ret = e.getErrCode();                           \
    } catch( const std::exception& e ) {                      \
        int cp_size = strlen( e.what() );                     \
        if ( cp_size >= MAX_MSG_SIZE ) {                      \
            cp_size = MAX_MSG_SIZE - 6;                       \
            strcpy( m->error_message + cp_size, "[...]" );    \
        }                                                     \
        strncpy( m->error_message, e.what(), cp_size );       \
        SPDLOG_ERROR( e.what() );                             \
        __try_ret = DRM_Fatal;                                \
    }                                                         \
    return __try_ret;


// Memory management
DRM_ErrorCode DrmManager_alloc( DrmManager **p_m,
        const char* conf_file_path,
        const char* cred_file_path,
        ReadRegisterCallback read_register,
        WriteRegisterCallback write_register,
        AsynchErrorCallback async_error,
        void* user_p) {
    DrmManager *m;
    m = (decltype(m))malloc(sizeof(*m));
    m->drm = NULL;
    *p_m = m;
    TRY
        if (read_register == NULL)
            Throw( DRM_BadArg, spdlog::level::err, "Read register callback function must not be NULL" );
        if (write_register == NULL)
            Throw( DRM_BadArg, spdlog::level::err, "Write register callback function must not be NULL" );
        if (async_error == NULL)
            Throw( DRM_BadArg, spdlog::level::err, "Asynchronous error callback function must not be NULL" );
        m->drm = (decltype(m->drm))malloc(sizeof(*(m->drm)));
        m->drm->obj = NULL;
        m->drm->obj = new cpp::DrmManager(conf_file_path, cred_file_path,
                    [user_p, read_register](uint32_t offset, uint32_t* value)
                        { return read_register(offset, value, user_p); },
                    [user_p, write_register](uint32_t offset, uint32_t value)
                        { return write_register(offset, value, user_p); },
                    [user_p, async_error](const std::string& msg)
                        { async_error(msg.c_str(), user_p); }
                );
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_free(DrmManager **p_m) {
    DrmManager *m = NULL;
    TRY
        checkPointer(p_m);
        m = *p_m;
        checkPointer(m);
        checkPointer(m->drm);
        if (m->drm->obj != NULL) {
            delete m->drm->obj;
        }
        free(m->drm);
        free(m);
        *p_m = NULL;
    CATCH_RETURN
}

// Methods
DRM_ErrorCode DrmManager_activate( DrmManager *m, bool resume_session_request ) {
    TRY
        checkPointer(m);
        m->drm->obj->activate( resume_session_request );
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_deactivate( DrmManager *m, bool pause_session_request ) {
    TRY
        checkPointer(m);
        m->drm->obj->deactivate( pause_session_request );
    CATCH_RETURN
}


DRM_ErrorCode DrmManager_get_bool( DrmManager *m, const DrmParameterKey key, bool* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->drm->obj->get<bool>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_int( DrmManager *m, const DrmParameterKey key, int* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->drm->obj->get<int32_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_uint( DrmManager *m, const DrmParameterKey key, unsigned int* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->drm->obj->get<uint32_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_int64( DrmManager *m, const DrmParameterKey key, long long* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->drm->obj->get<int64_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_uint64( DrmManager *m, const DrmParameterKey key, unsigned long long* p_value ) {
    TRY
    checkPointer(m);
    *p_value = m->drm->obj->get<uint64_t>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_float( DrmManager *m, const DrmParameterKey key, float* p_value ) {
    TRY
    checkPointer(m);
    *p_value = m->drm->obj->get<float>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_double( DrmManager *m, const DrmParameterKey key, double* p_value ) {
    TRY
        checkPointer(m);
        *p_value = m->drm->obj->get<double>((cpp::ParameterKey) key);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_string( DrmManager *m, const DrmParameterKey key, char** p_value ) {
    TRY
        checkPointer(m);
        std::string json = m->drm->obj->get<std::string>((cpp::ParameterKey)key);
        char *json_out = strdup(json.c_str());
        *p_value = json_out;
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_get_json_string( DrmManager *m, const char* json_in, char** p_json_out ) {
    TRY
        checkPointer(m);
        std::string json_string(json_in);
        m->drm->obj->get(json_string);
        char *json_out = strdup(json_string.c_str());
        *p_json_out = json_out;
    CATCH_RETURN
}


DRM_ErrorCode DrmManager_set_bool( DrmManager *m, const DrmParameterKey key, const bool value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<bool>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_int( DrmManager *m, const DrmParameterKey key, const int value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<int32_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_uint( DrmManager *m, const DrmParameterKey key, const unsigned int value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<uint32_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_int64( DrmManager *m, const DrmParameterKey key, const long long value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<int64_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_uint64( DrmManager *m, const DrmParameterKey key, const unsigned long long value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<uint64_t>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_float( DrmManager *m, const DrmParameterKey key, const float value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<float>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_double( DrmManager *m, const DrmParameterKey key, const double value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<double>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_string( DrmManager *m, const DrmParameterKey key, const char* value ) {
    TRY
        checkPointer(m);
        m->drm->obj->set<std::string>((cpp::ParameterKey)key, value);
    CATCH_RETURN
}

DRM_ErrorCode DrmManager_set_json_string( DrmManager *m, const char* json_string ) {
TRY
    checkPointer(m);
    std::string json(json_string);
    m->drm->obj->set(json);
CATCH_RETURN
}
