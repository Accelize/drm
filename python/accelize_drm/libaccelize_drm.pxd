# cython: language_level=3
# distutils: language = c++
"""libaccelize_drm Cython header"""

from libc.stdint cimport uint32_t
from libcpp cimport bool
from libcpp.string cimport string

ctypedef int (*ReadRegisterCallback)(uint32_t, uint32_t*)
ctypedef int (*WriteRegisterCallback)(uint32_t, uint32_t)
ctypedef void (*AsynchErrorCallback)(const string &)


cdef extern from "accelize/drm.h" namespace "Accelize::DRM" nogil:

    enum ParameterKey:
        pass

    const char* getApiVersion() except +

    cdef cppclass DrmManager:

        DrmManager(string& conf_file_path, string& cred_file_path,
                   ReadRegisterCallback f_read_register,
                   WriteRegisterCallback f_write_register,
                   AsynchErrorCallback f_asynch_error) except +

        void activate(bool resume_session_request) except +

        void deactivate(bool pause_session_request) except +

        void get(string& json_string) except +
        T get[T](const ParameterKey key_id) except +

        void set(string& json_string) except +
        void set[T](const ParameterKey key_id, const T& value) except +
