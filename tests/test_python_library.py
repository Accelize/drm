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
        with pytest.raises(exc.DRMFatal):
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
        # Short version
        py_version = '1.2.3'
        version = '4.5.6'
        accelize_drm.__version__ = py_version
        accelize_drm._get_api_version = lambda: version.encode()
        versions = accelize_drm.get_api_version()
        assert versions.py_major == 1
        assert versions.py_minor == 2
        assert versions.py_revision == 3
        assert versions.py_prerelease == ''
        assert versions.py_build == ''
        assert versions.py_version == py_version
        assert versions.major == 4
        assert versions.minor == 5
        assert versions.revision == 6
        assert versions.prerelease == ''
        assert versions.build == ''
        assert versions.version == version

        # Prerelease version with build
        py_version = '1.2.3-alpha.1+commit1'
        version = '4.5.6-beta.2+commit2'
        accelize_drm.__version__ = py_version
        accelize_drm._get_api_version = lambda: version.encode()
        versions = accelize_drm.get_api_version()
        assert versions.py_major == 1
        assert versions.py_minor == 2
        assert versions.py_revision == 3
        assert versions.py_prerelease == 'alpha.1'
        assert versions.py_build == 'commit1'
        assert versions.py_version == py_version
        assert versions.major == 4
        assert versions.minor == 5
        assert versions.revision == 6
        assert versions.prerelease == 'beta.2'
        assert versions.build == 'commit2'
        assert versions.version == version

    # Restore mocked versions
    finally:
        accelize_drm.__version__ = accelize_drm___version__
        accelize_drm._get_api_version = accelize_drm__get_api_version
