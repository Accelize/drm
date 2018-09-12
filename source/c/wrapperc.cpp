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

namespace cpp = Accelize::DRMLib;

struct MeteringSessionManager_s {
    cpp::MeteringSessionManager *obj;
};

void checkPointer(void *p) {
    if(p==NULL)
        cpp::Throw(DRMBadArg, "Provided pointer is NULL");
}

/* Help macros TRY/CATCH to return code error */
#define TRY \
DRMLibErrorCode __try_ret = DRMLibOK; \
try{

#define _CATCH \
} catch(const cpp::Exception& e) { \
    cpp::Error(e.what()); \
    __try_ret = e.getErrCode(); \
} catch(const std::exception& e) { \
    cpp::Error(e.what()); \
    __try_ret = DRMLibFatal; \
} \

#define CATCH_RETURN \
_CATCH; \
return __try_ret;

// Memory management
DRMLibErrorCode MeteringSessionManager_alloc(MeteringSessionManager **p_m, const char* conf_file_path, const char* cred_file_path, ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb, void * user_p){
TRY
    MeteringSessionManager *m;
    m = (decltype(m))malloc(sizeof(*m));
    m->obj = new cpp::MeteringSessionManager(conf_file_path, cred_file_path,
                            [user_p, f_drm_read32](uint32_t offset, uint32_t* value) {return f_drm_read32(offset, value, user_p);},
                            [user_p, f_drm_write32](uint32_t offset, uint32_t value) {return f_drm_write32(offset, value, user_p);},
                            [user_p, f_error_cb](const std::string& msg) {f_error_cb(msg.c_str(), user_p);}
                        );
    *p_m = m;
CATCH_RETURN
}

DRMLibErrorCode MeteringSessionManager_free(MeteringSessionManager **p_m) {
TRY
    checkPointer(p_m);
    MeteringSessionManager *m = *p_m;
    checkPointer(m);
    delete m->obj;
    free(m);
    *p_m = NULL;
CATCH_RETURN
}

// Methods
DRMLibErrorCode MeteringSessionManager_start_session(MeteringSessionManager *m){
TRY
    checkPointer(m);
    m->obj->start_session();
CATCH_RETURN
}

DRMLibErrorCode MeteringSessionManager_stop_session(MeteringSessionManager *m){
TRY
    checkPointer(m);
    m->obj->stop_session();
CATCH_RETURN
}

DRMLibErrorCode MeteringSessionManager_pause_session(MeteringSessionManager *m){
TRY
    checkPointer(m);
    m->obj->pause_session();
CATCH_RETURN
}

DRMLibErrorCode MeteringSessionManager_resume_session(MeteringSessionManager *m){
TRY
    checkPointer(m);
    m->obj->resume_session();
CATCH_RETURN
}

DRMLibErrorCode MeteringSessionManager_auto_start_session(MeteringSessionManager *m){
TRY
    checkPointer(m);
    m->obj->auto_start_session();
CATCH_RETURN
}

DRMLibErrorCode MeteringSessionManager_dump_drm_hw_report(MeteringSessionManager *m){
TRY
    checkPointer(m);
    m->obj->dump_drm_hw_report(std::cout);
CATCH_RETURN
}

const char * DRMLib_get_version() {
    return cpp::getVersion();
}
