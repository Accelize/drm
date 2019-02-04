# -*- coding: utf-8 -*-
"""
Test Python library.
"""
import pytest


def test_raise_exceptions(accelize_drm):
    """
    Test proper Exception raising.
    """
    exc = accelize_drm.exceptions

    # Test: Raise function and default async callback
    for raise_from_error in (exc._raise_from_error, exc._async_error_callback):

        # Raise an exception from message with error code
        with pytest.raises(exc.DRMLibFatal):
            raise_from_error('message [errCode=90001]\n')

        # Raise an exception with no error code
        with pytest.raises(exc.DRMException):
            raise_from_error('message')

    # Tests: Raise an exception from error code
    with pytest.raises(exc.DRMWSRespError):
        exc._raise_from_error('message', error_code=10001)


def test_get_api_version(accelize_drm):
    """
    Test get_api_version function.
    """
    # Mock Versions
    accelize_drm___version__ = accelize_drm.__version__
    accelize_drm__get_api_version = accelize_drm._get_api_version

    # Tests
    try:
        # Versions without commit
        accelize_drm.__version__ = '1.2.3'
        accelize_drm._get_api_version = lambda: b'4.5.6'
        versions = accelize_drm.get_api_version()
        assert versions.py_major == 1
        assert versions.py_minor == 2
        assert versions.py_revision == 3
        assert versions.py_commit == ''
        assert versions.major == 4
        assert versions.minor == 5
        assert versions.revision == 6
        assert versions.commit == ''

        # Versions with commit
        accelize_drm.__version__ = '1.2.3.commit1'
        accelize_drm._get_api_version = lambda: b'4.5.6.commit2'
        versions = accelize_drm.get_api_version()
        assert versions.py_major == 1
        assert versions.py_minor == 2
        assert versions.py_revision == 3
        assert versions.py_commit == 'commit1'
        assert versions.major == 4
        assert versions.minor == 5
        assert versions.revision == 6
        assert versions.commit == 'commit2'

    # Restore mocked versions
    finally:
        accelize_drm.__version__ = accelize_drm___version__
        accelize_drm._get_api_version = accelize_drm__get_api_version
