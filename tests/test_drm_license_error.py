# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint
from datetime import datetime, timedelta
from re import search
from json import loads, dumps
from flask import request as _request
from requests import get, post
from tests.proxy import get_context, set_context, update_context
from tests.conftest import wait_deadline, wait_until_true


@pytest.mark.no_parallel
def test_header_error_on_key(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, request, log_file_factory):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]

    # Program FPGA with lastest HDK per major number
    image_id = driver.fpga_image
    driver.program_fpga(image_id)

    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        # Check failure is detected
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager.activate()
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
        assert "DRM Controller Activation is in timeout" in str(excinfo.value)
        assert "License header check error" in str(excinfo.value)


@pytest.mark.no_parallel
def test_header_error_on_key2(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, request, log_file_factory):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]

    # Program FPGA with lastest HDK per major number
    image_id = driver.fpga_image
    driver.program_fpga(image_id)

    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        # Check failure is detected
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager.activate()
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
        assert "DRM Controller Activation is in timeout" in str(excinfo.value)
        assert "License MAC check error" in str(excinfo.value)


@pytest.mark.no_parallel
def test_header_error_on_licenseTimer(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, request, log_file_factory):
    """
    Test a MAC error is returned if the licenseTimer value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]

    # Program FPGA with lastest HDK per major number
    image_id = driver.fpga_image
    driver.program_fpga(image_id)

    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        wait_until_true(lambda: async_cb.was_called, 2*lic_duration)
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert async_cb.was_called
        assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
        assert search(r"License header check error", async_cb.message)
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager.deactivate()
        assert async_cb.was_called
        assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
        assert search(r"Session ID mismatch", async_cb.message)


@pytest.mark.no_parallel
def test_header_error_on_licenseTimer2(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, request, log_file_factory):
    """
    Test a MAC error is returned if the licenseTimer value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]

    # Program FPGA with lastest HDK per major number
    image_id = driver.fpga_image
    driver.program_fpga(image_id)

    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        wait_until_true(lambda: async_cb.was_called, 2*lic_duration)
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
    assert search(r"License (header|MAC) check error", async_cb.message)


@pytest.mark.no_parallel
def test_replay_request(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, request, log_file_factory):
    """
    Test an error is returned if a wrong session id is provided
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    context = {'record': True}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        # Start session #1 to record
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        lic_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_duration)
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        assert get_context('cnt_license') >= 2

        # Start session #2 to replay session #1
        update_context(record=False)
        assert not get_context('record')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        lic_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: async_cb.was_called, 2*lic_duration)
        assert not drm_manager.get('license_status')
        assert async_cb.was_called
        assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
        assert search(r"License (header|MAC) check error", async_cb.message)
        activators.autotest(is_activated=False)
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager.deactivate()
        assert async_cb.was_called
        assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
        assert search(r"Session ID mismatch", async_cb.message)

