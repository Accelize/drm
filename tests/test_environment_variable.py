# -*- coding: utf-8 -*-
"""
Test environment variables behavior to set parameters of the DRM Library.
"""
import pytest
from os import environ


def test_env_var_DRMSAAS_URL(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test DRMSAAS_URL environment variable overwrite value in config file
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Check when DRMSAAS_URL is set
    environ['DRMSAAS_URL'] = conf_json['licensing']['url']
    conf_json['licensing']['url'] = 'http://acme.com'
    conf_json.save()
    assert conf_json['licensing']['url'] != environ['DRMSAAS_URL']

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

    # Check when DRMSAAS_URL is unset
    del environ['DRMSAAS_URL']
    assert 'DRMSAAS_URL' not in  environ.keys()
    cred_json.flush_cache()

    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            pass
    assert "Accelize Web Service error 404 on HTTP request" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'Accelize Web Service error 404')
    async_cb.reset()


def test_env_var_DRMSAAS_CLIENT_ID(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test DRMSAAS_CLIENT_ID environment variable overwrite value in cred file
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Check when DRMSAAS_CLIENT_ID is set
    environ['DRMSAAS_CLIENT_ID'] = cred_json['client_id']
    cred_json['client_id'] = 'acme_is_a_great_company' #'A2B3C4D5E6F7G2H3I4J5K6L7M2'
    cred_json.save()
    assert cred_json['client_id'] != environ['DRMSAAS_CLIENT_ID']
    cred_json.flush_cache()

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

    # Check when DRMSAAS_CLIENT_ID is unset
    del environ['DRMSAAS_CLIENT_ID']
    assert 'DRMSAAS_CLIENT_ID' not in  environ.keys()
    cred_json.flush_cache()

    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            pass
    assert "Accelize Web Service error 401" in str(excinfo.value)
    assert "invalid_client" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'Accelize Web Service error 401')
    async_cb.reset()


def test_env_var_DRMSAAS_CLIENT_SECRET(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test DRMSAAS_CLIENT_SECRET environment variable overwrite value in cred file
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Check when DRMSAAS_CLIENT_SECRET is set
    environ['DRMSAAS_CLIENT_SECRET'] = cred_json['client_secret']
    cred_json['client_secret'] = 'acme_is_a_great_company'
    cred_json.save()
    assert cred_json['client_secret'] != environ['DRMSAAS_CLIENT_SECRET']
    cred_json.flush_cache()

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

    # Check when DRMSAAS_CLIENT_SECRET is unset
    del environ['DRMSAAS_CLIENT_SECRET']
    assert 'DRMSAAS_CLIENT_SECRET' not in  environ.keys()
    cred_json.flush_cache()

    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            pass
    assert "Accelize Web Service error 401" in str(excinfo.value)
    assert "invalid_client" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'Accelize Web Service error 401')
    async_cb.reset()
