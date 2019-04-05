# coding=utf-8
# distutils: language=c++
# distutils: libraries=accelize_drm
# cython: language_level=3
"""Accelize DRM Python binding"""
from libcpp cimport bool
from libcpp.string cimport string

from os import fsencode as _fsencode
from ctypes import (
    CFUNCTYPE as _CFUNCTYPE, POINTER as _POINTER, c_uint32 as _c_uint32,
    c_int as _c_int, c_void_p as _c_void_p, addressof as _addressof)
from json import dumps as _dumps, loads as _loads

from accelize_drm.libaccelize_drm cimport (
    DrmManager as C_DrmManager, getApiVersion,
    ReadRegisterCallback, WriteRegisterCallback, AsynchErrorCallback)

from accelize_drm.exceptions import (
    _async_error_callback,_raise_from_error, DRMBadArg as _DRMBadArg)

_ASYNC_ERROR_CFUNCTYPE = _CFUNCTYPE(_c_void_p, _c_void_p)
_READ_REGISTER_CFUNCTYPE = _CFUNCTYPE(_c_int, _c_uint32, _POINTER(_c_uint32))
_WRITE_REGISTER_CFUNCTYPE = _CFUNCTYPE(_c_int, _c_uint32, _c_uint32)

def _handle_exceptions(exception):
    """
    Raise exception based on Cython converted C++ exception.

    Args:
        exception (RuntimeException): Runtime Exception.

    Raises:
        accelize_drm.exceptions.DrmException subclass: DRM Exception.
    """
    _raise_from_error(str(exception))


def _get_api_version():
    """
    Return "libaccelize_drm" API version.

    Returns:
        bytes: C++ library version
    """
    try:
        return getApiVersion()
    except RuntimeError as exception:
        _handle_exceptions(exception)


cdef class DrmManager:
    """
    Manage Accelize DRM by handling communication with DRM controller IP and
    Accelize Web service.

    The DrmManager require callback functions to access FPGA register.

    Args:
        conf_file_path (path-like object):
            Path to the DRM configuration JSON file.
        cred_file_path (path-like object):
            Path to the user Accelize credential JSON file.
        read_register (function): FPGA read register callback function.
            The function needs to return an int and accept following arguments:
            register_offset (int), returned_data (int).
            The function can't be a non static method.
            register_offset is relative to first register of DRM controller
            This function must be thread-safe in case of concurrency on the
            register bus.
        write_register (function): FPGA write register callback function.
            The function needs to return an int and accept following arguments:
            register_offset (int), data_to_write (int).
            The function can't be a non static method.
            register_offset is relative to first register of DRM controller
            This function must be thread-safe in case of concurrency on the
            register bus.
        async_error (function): Asynchronous error handling callback function.
            This function is called in case of asynchronous error during
            operation.
            The function needs to return None and accept following argument:
            error_message (bytes). The function can't be a non static method.
            If not specified, use default callback that raises
            "accelize_drm.exceptions.DRMException" or its subclasses.
    """

    cdef C_DrmManager* _drm_manager
    cdef object _read_register
    cdef object _read_register_c
    cdef ReadRegisterCallback _read_register_p
    cdef object _write_register
    cdef object _write_register_c
    cdef WriteRegisterCallback _write_register_p
    cdef object _async_error
    cdef object _async_error_c
    cdef AsynchErrorCallback _async_error_p
    cdef string _conf_file_path
    cdef string _cred_file_path


    def __cinit__(self, conf_file_path, cred_file_path,
                  read_register, write_register, async_error=None):

        # Handle python paths
        self._conf_file_path = _fsencode(conf_file_path)
        self._cred_file_path = _fsencode(cred_file_path)

        # Handle callbacks
        if not hasattr(read_register, "__call__"):
            _raise_from_error(
                'Read register callback function must not be None',
                error_code=_DRMBadArg.error_code)
        if not hasattr(write_register, "__call__"):
            _raise_from_error(
                'Write register callback function must not be None',
                error_code=_DRMBadArg.error_code)

        self._read_register = read_register
        self._read_register_c = _READ_REGISTER_CFUNCTYPE(read_register)
        self._read_register_p = (<ReadRegisterCallback*><size_t>_addressof(
            self._read_register_c))[0]

        self._write_register = write_register
        self._write_register_c  = _WRITE_REGISTER_CFUNCTYPE(write_register)
        self._write_register_p = (<WriteRegisterCallback*><size_t>_addressof(
            self._write_register_c))[0]

        if async_error is None:
            # Use default error callback
            async_error = _async_error_callback

        if not hasattr(async_error, "__call__"):
            _raise_from_error(
                'Asynchronous error callback function must be a callable',
                error_code=_DRMBadArg.error_code)

        def async_error_char(error_message):
            """Convert string to char* before passing if to Python callback."""
            cdef string *error_message_str = <string*><size_t>error_message
            error_message_char = error_message_str.c_str()
            async_error(error_message_char)

        self._async_error = async_error_char
        self._async_error_c  = _ASYNC_ERROR_CFUNCTYPE(async_error_char)
        self._async_error_p = (<AsynchErrorCallback*><size_t>_addressof(
            self._async_error_c))[0]

        # Instantiate object
        try:
            with nogil:
                self._drm_manager = new C_DrmManager(
                    self._conf_file_path, self._cred_file_path,
                    self._read_register_p, self._write_register_p,
                    self._async_error_p)
        except RuntimeError as exception:
            _handle_exceptions(exception)

    def __dealloc__(self):
        with nogil:
            del self._drm_manager

    def activate(self, const bool resume_session_request=False):
        """
        Activate DRM session.

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
                reused. If no pending session is found, create a new one. If
                False and a pending session is found, close it and create a new
                one. Default to False.
        """
        try:
            with nogil:
                self._drm_manager.activate(resume_session_request)
        except RuntimeError as exception:
            _handle_exceptions(exception)

    def deactivate(self, const bool pause_session_request=False):
        """
        Deactivate DRM session.

        This function deactivates/locks the hardware back and close the session
        unless the "pause_session_request" argument is True. In this case,
        the session is kept opened for later use.

        This function will join the thread keeping the hardware unlocked.

        When the function returns, the hardware are guaranteed to be locked.

        Args:
            pause_session_request (bool): If True, the current session is kept
                open for later usage. Otherwise, the current session is closed.
                Default to False.
        """
        try:
            with nogil:
                self._drm_manager.deactivate(pause_session_request)
        except RuntimeError as exception:
            _handle_exceptions(exception)

    def set(self, **values):
        """
        Set information of the DRM system.

        This function overwrites an internal parameter of the DRM system.

        Args:
            values: Parameters to set as keyword arguments
        """
        values_json = _dumps(values).encode()
        cdef string json_string = values_json
        try:
            with nogil:
                self._drm_manager.set(json_string)
        except RuntimeError as exception:
            _handle_exceptions(exception)

    def get(self, *keys):
        """
        Get information from the DRM system.

        This function gives access to the internal parameter of the DRM system.

        Args:
            keys (str): Parameters names requested.

        Returns:
            dict or object: If multiple keys are specified, return a dict with
                parameters names as keys else return a single value.
        """
        keys_dict = {key: None for key in filter(lambda x: x, keys)}
        if len(keys_dict) == 0:
            _raise_from_error('keys argument is empty', error_code=_DRMBadArg.error_code)
        keys_json = _dumps(keys_dict).encode()
        cdef string json_string = keys_json
        try:
            with nogil:
                self._drm_manager.get(json_string)
        except RuntimeError as exception:
            print('Caught exception')
            _handle_exceptions(exception)

        items = _loads(bytes(json_string).decode())
        if len(keys) > 1:
            return items
        return items[keys[0]]
