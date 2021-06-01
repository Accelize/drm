# -*- coding: utf-8 -*-
"""
Test environment variables behavior to set parameters of the DRM Library.
"""
import pytest
from os import environ


def test_env_var_ONEPORTAL_URL(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test ONEPORTAL_URL environment variable overwrite value in config file
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Check when ONEPORTAL_URL is set
    environ['ONEPORTAL_URL'] = conf_json['licensing']['url']
    conf_json['licensing']['url'] = 'http://acme.com'
    conf_json.save()
    assert conf_json['licensing']['url'] != environ['ONEPORTAL_URL']

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        drm_manager.deactivate()
    async_cb.assert_NoError()

    # Check when ONEPORTAL_URL is unset
    del environ['ONEPORTAL_URL']
    assert 'ONEPORTAL_URL' not in  environ.keys()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "OAuth2 Web Service error 404" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'OAuth2 Web Service error 404')
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'The issue could be caused by a networking problem: please verify your internet access')


def test_env_var_ONEPORTAL_CLIENT_ID(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test ONEPORTAL_CLIENT_ID environment variable overwrite value in cred file
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Check when ONEPORTAL_CLIENT_ID is set
    environ['ONEPORTAL_CLIENT_ID'] = cred_json['client_id']
    cred_json['client_id'] = 'acme_is_a_great_company'
    cred_json.save()
    assert cred_json['client_id'] != environ['ONEPORTAL_CLIENT_ID']

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        drm_manager.deactivate()
    async_cb.assert_NoError()

    # Check when ONEPORTAL_CLIENT_ID is unset
    del environ['ONEPORTAL_CLIENT_ID']
    assert 'ONEPORTAL_CLIENT_ID' not in  environ.keys()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "OAuth2 Web Service error 401" in str(excinfo.value)
        assert "invalid_client" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'OAuth2 Web Service error 401')
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'The issue could be caused by a networking problem: please verify your internet access')

