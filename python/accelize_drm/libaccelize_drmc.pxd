# cython: language_level=3
"""libaccelize_drmc Cython header"""

from libc.stdint cimport uint32_t

ctypedef int(*ReadRegisterCallback)(uint32_t, uint32_t*, void*user_p)
ctypedef int(*WriteRegisterCallback)(uint32_t, uint32_t, void*user_p)
ctypedef void(*AsynchErrorCallback)(const char*, void*user_p)


cdef extern from "accelize/drmc.h" nogil:

    ctypedef struct DrmManager:
        char error_message[1024]
        pass

    ctypedef enum DrmParameterKey:
        pass


    const char * DrmManager_getApiVersion()

    int DrmManager_alloc(DrmManager ** p_m,
                         const char*conf_file_path, const char*cred_file_path,
                         ReadRegisterCallback f_read_register,
                         WriteRegisterCallback f_write_register,
                         AsynchErrorCallback f_asynch_error, void*user_p)

    int DrmManager_free(DrmManager ** p_m)

    int DrmManager_activate(DrmManager *m)

    int DrmManager_deactivate(DrmManager *m)

    int DrmManager_get_json_string(DrmManager *m, const char* json_in, char** json_out)
    int DrmManager_get_bool(DrmManager *m, const DrmParameterKey key, bint* p_value)
    int DrmManager_get_int(DrmManager *m, const DrmParameterKey key, int* p_value)
    int DrmManager_get_uint(DrmManager *m, const DrmParameterKey key, unsigned int* p_value)
    int DrmManager_get_int64(DrmManager *m, const DrmParameterKey key, long long* p_value)
    int DrmManager_get_uint64(DrmManager *m, const DrmParameterKey key, unsigned long long* p_value)
    int DrmManager_get_float(DrmManager *m, const DrmParameterKey key, float* p_value)
    int DrmManager_get_double(DrmManager *m, const DrmParameterKey key, double* p_value)
    int DrmManager_get_string(DrmManager *m, const DrmParameterKey key, char** p_value)

    int DrmManager_set_json_string(DrmManager *m, const char* json_in)
    int DrmManager_set_bool(DrmManager *m, const DrmParameterKey key, bint value)
    int DrmManager_set_int(DrmManager *m, const DrmParameterKey key, int value)
    int DrmManager_set_uint(DrmManager *m, const DrmParameterKey key, unsigned int value)
    int DrmManager_set_int64(DrmManager *m, const DrmParameterKey key, long long value)
    int DrmManager_set_uint64(DrmManager *m, const DrmParameterKey key, unsigned long long value)
    int DrmManager_set_float(DrmManager *m, const DrmParameterKey key, float value)
    int DrmManager_set_double(DrmManager *m, const DrmParameterKey key, double value)
    int DrmManager_set_string(DrmManager *m, const DrmParameterKey key, char* value)
