# coding=utf-8
# distutils: libraries=accelize_drmc accelize_drm
# cython: language_level=3
"""Accelize DRM Python binding (C binding variant)"""

from os import fsencode as _fsencode
from ctypes import (
    CFUNCTYPE as _CFUNCTYPE, POINTER as _POINTER, c_uint32 as _c_uint32,
    c_int as _c_int, c_void_p as _c_void_p, c_char_p as _c_char_p,
    addressof as _addressof)
from json import dumps as _dumps, loads as _loads

from accelize_drm.libaccelize_drmc cimport (
DrmManager as C_DrmManager,
ReadRegisterCallback, WriteRegisterCallback, AsynchErrorCallback,
DrmManager_alloc, DrmManager_free, DrmManager_activate,
DrmManager_deactivate, DrmManager_get_json_string, DrmManager_set_json_string,
DrmManager_getApiVersion)

from accelize_drm.exceptions import _raise_from_error, _async_error_callback

_ASYNC_ERROR_CFUNCTYPE = _CFUNCTYPE(_c_void_p, _c_char_p, _c_void_p)
_READ_REGISTER_CFUNCTYPE = _CFUNCTYPE(
    _c_int, _c_uint32, _POINTER(_c_uint32), _c_void_p)
_WRITE_REGISTER_CFUNCTYPE = _CFUNCTYPE(
    _c_int, _c_uint32, _c_uint32, _c_void_p)

def _get_api_version():
    """
    Return "libaccelize_drmc" API version.

    Returns:
        bytes: C library version
    """
    return DrmManager_getApiVersion()


cdef class DrmManager:
    """
    Manage Accelize DRM by handling communication with DRM controller IP and
    Accelize Web service.

    The DrmManager require callback functions to access FPGA register.

    Args:
        conf_file_path (path-like object): Path to configuration JSON file.
        cred_file_path (path-like object): Path to credential JSON file.
        read_register (function): FPGA read register function. The function
            needs to return an int and accept following arguments:
            register_offset (int), returned_data (int).
            The function can't be a non static method.
            register_offset is relative to first register of DRM controller
            This function must be thread-safe in case of concurrency on the
            register bus
        write_register (function): FPGA write register function. The function
            needs to return an int and accept following arguments:
            register_offset (int), data_to_write (int).
            The function can't be a non static method.
            register_offset is relative to first register of DRM controller
            This function must be thread-safe in case of concurrency on the
            register bus
        async_error (function): Asynchronous error handling function.
            This function is called in case of asynchronous error during
            operation.
            The function needs to return None and accept following argument:
            error_message (str). The function can't be a non static method.
            If not specified, use default callback that raises
            "accelize_drm.exceptions.DRMException" or its subclasses.
    """

    cdef C_DrmManager*c_drm_manager
    cdef object read_register_py_func
    cdef object read_register_c_func
    cdef object write_register_py_func
    cdef object write_register_c_func
    cdef object async_error_py_func
    cdef object async_error_c_func
    cdef object conf_file_path
    cdef object cred_file_path

    def __cinit__(self, conf_file_path, cred_file_path, read_register,
                  write_register, async_error=None):

        # Handle python paths
        self.conf_file_path = _fsencode(conf_file_path)
        cdef char* conf_file_path_c = self.conf_file_path
        self.cred_file_path = _fsencode(cred_file_path)
        cdef char* cred_file_path_c = self.cred_file_path

        # Handle callbacks

        def read_register_c(register_offset, returned_data, user_p):
            """read_register with "user_p" support"""
            return read_register(register_offset, returned_data)

        self.read_register_py_func = (read_register_c, read_register)
        self.read_register_c_func = _READ_REGISTER_CFUNCTYPE(read_register_c)
        cdef ReadRegisterCallback read_register_ptr = (
            <ReadRegisterCallback*><size_t>_addressof(
                self.read_register_c_func))[0]

        def write_register_c(register_offset, returned_data, user_p):
            """write_register with "user_p" support"""
            return write_register(register_offset, returned_data)

        self.write_register_py_func = (write_register_c, write_register)
        self.write_register_c_func  = _WRITE_REGISTER_CFUNCTYPE(
            write_register_c)
        cdef WriteRegisterCallback write_register_ptr = (
            <WriteRegisterCallback*><size_t>_addressof(
                self.write_register_c_func))[0]

        if async_error is None:
            # Use default error callback
            async_error = _async_error_callback

        def async_error_c(error_message, user_p):
            """error_message with "user_p" support"""
            return async_error(error_message)

        self.async_error_py_func = (async_error_c, async_error)
        self.async_error_c_func  = _ASYNC_ERROR_CFUNCTYPE(async_error_c)
        cdef AsynchErrorCallback async_error_ptr = (
            <AsynchErrorCallback*><size_t>_addressof(
                self.async_error_c_func))[0]

        # Instantiate object
        with nogil:
            self._check_return_code(DrmManager_alloc(
                &self.c_drm_manager, conf_file_path_c, cred_file_path_c,
                read_register_ptr, write_register_ptr, async_error_ptr,
                <void*>self))

    def __dealloc__(self):
        with nogil:
            self._check_return_code(DrmManager_free(&self.c_drm_manager))

    def activate(self, const bint resume_session_request=False):
        """
        Activate DRM session

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

        Args:
            resume_session_request (bool): If True, the pending session is
                resused. If no pending session is found, create a new one. If
                False and a pending session is found, close it and create a new
                one. Default to False.
        """
        with nogil:
            self._check_return_code(DrmManager_activate(
                self.c_drm_manager, resume_session_request))

    def deactivate(self, const bint pause_session_request=False):
        """
        Deactivate DRM session
        This function deactivates/locks the hardware back and close the session
        unless the "pause_session_request" argument is True. In this case,
        the session is kept opened for later use.

        This function will join the thread keeping the hardware unlocked.

        When the function returns, the hardware are guaranteed to be locked.

        Args:
            pause_session_request (bool): If true, the current session is kept
                open for later usage. Otherwise, the current session is closed.
                Default to False.
        """
        with nogil:
            self._check_return_code(DrmManager_deactivate(
                self.c_drm_manager, pause_session_request))

    def set(self, **values):
        """
        Set information of the DRM system.

        Args:
            values: Parameters to set as keyword arguments
        """
        values_json = _dumps(values).encode()
        cdef char* json_in = values_json
        with nogil:
            self._check_return_code(DrmManager_set_json_string(
                self.c_drm_manager, json_in))

    def get(self, *keys):
        """
        Get information from the DRM system.

        Args:
            keys (str): Parameters names requested.

        Returns:
            dict or object: If multiple keys are specified, return a dict with
                parameters names as keys else return a single value.
        """
        keys_json = _dumps({key: None for key in keys}).encode()
        cdef char* json_in = keys_json
        cdef char* json_out
        with nogil:
             self._check_return_code(DrmManager_get_json_string(
                 self.c_drm_manager, json_in, &json_out))
        items = _loads(json_out)
        if len(keys) > 1:
            return items
        return items[keys[0]]

    cdef inline void _check_return_code(self, const int return_code) nogil:
        """
        Raise exception based on return code (if not equal 0)

        Args:
            return_code (int): Return code.

        Raises:
            accelize_drm.exceptions.DrmException subclass: DRM Exception.
        """
        if return_code:
            with gil:
                try:
                    # Get error message
                    error_message = self.get('STRERROR')
                except RuntimeError:
                    error_message = ''
                _raise_from_error(error_message, error_code=return_code)
