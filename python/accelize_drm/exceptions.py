# coding=utf-8
"""DRM exceptions"""
import sys as _sys


class DRMException(Exception):
    """
    Accelize DRM Base exception
    """
    error_code = None


class DRMBadArg(DRMException):
    """
    Bad argument provided
    """
    #: Error code
    error_code = 1


class DRMBadFormat(DRMException):
    """
    Bad format of provided input or config file
    """
    #: Error code
    error_code = 2


class DRMExternFail(DRMException):
    """
    Fail happened in an external library
    """
    #: Error code
    error_code = 3


class DRMBadUsage(DRMException):
    """
    Wrong usage of the DRMLib
    """
    #: Error code
    error_code = 4


class DRMBadFrequency(DRMException):
    """
    DRM frequency defined in configuration script differs from real one
    detected automatically by the DRMLib
    """
    #: Error code
    error_code = 5


class DRMWSRespError(DRMException):
    """
    A malformed response has been received from Accelize WebService
    """
    #: Error code
    error_code = 10001


class DRMWSReqError(DRMException):
    """
    Failed during HTTP request to Accelize WebService
    """
    #: Error code
    error_code = 10002


class DRMWSError(DRMException):
    """
    Error returned from Accelize WebService
    """
    #: Error code
    error_code = 10003


class DRMWSMayRetry(DRMException):
    """
    Error with request to Accelize Webservice, retry advised
    """
    #: Error code
    error_code = 10004


class DRMCtlrError(DRMException):
    """
    An error happened on a command on the DRM controller
    """
    #: Error code
    error_code = 20001


class DRMFatal(DRMException):
    """
    Fatal error, unknown error (Please contact Accelize)
    """
    #: Error code
    error_code = 30001


class DRMAssert(DRMException):
    """
    Assertion failed internally (Please contact Accelize)
    """
    #: Error code
    error_code = 30002


class DRMDebug(DRMException):
    """Generated for debug and testing only"""
    #: Error code
    error_code = 40001


_ERROR_CODES = {}
__all__ = ['DRMException']

for _name in dir(_sys.modules[__name__]):
    _member = getattr(_sys.modules[__name__], _name)
    if hasattr(_member, 'error_code') and _name != 'DRMException':
        __all__.append(_name)
        _ERROR_CODES[_member.error_code] = _member
del _name, _member, _sys


def _async_error_callback(error_message):
    """
    Default Asynchronous errors callback that raise DRM exceptions.

    Args:
        error_message (bytes): Error message.

    Raises:
        DRMException subclass: Exception
    """
    _raise_from_error(error_message)


def _raise_from_error(error_message, error_code=None):
    """
    Get Exception from error code.

    Args:
        error_message (bytes or str): Error message.
        error_code (int): Error code.

    Raises:
        DRMException subclass: Exception
    """
    if isinstance(error_message, bytes):
        error_message = error_message.decode()

    if error_code is None:
        import re
        match = re.search(r'\[errCode=(\d+)\]', error_message)
        if match is not None:
            error_code = int(match.group(1))
    raise _ERROR_CODES.get(error_code, DRMException)(error_message.strip())
