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
from flask import request, redirect, Response, session, url_for, jsonify
import requests


context = 0

@pytest.mark.no_parallel
def test_header_error_on_key(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    srv_url = conf_json['licensing']['url']

	# Set initial context on the live server
    context = {'cnt':0}
    r = requests.post(url_for('set', _external=True), json=context)
    assert r.status_code == 200
    r = requests.get(url_for('get', _external=True))
    assert r.status_code == 200
    assert loads(r.text) == context

    conf_json['licensing']['url'] = request.url + 'test_header_error_on_key'
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
        drm_manager.activate()
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
    assert "License header check error" in str(excinfo.value)
    async_cb.assert_NoError()



@pytest.mark.no_parallel
def test_header_error_on_licenseTimer(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test a MAC error is returned if the licenseTimer value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    srv_url = conf_json['licensing']['url']

	# Set initial context on the live server
    context = {'cnt':0}
    r = requests.post(url_for('set', _external=True), json=context)
    assert r.status_code == 200
    r = requests.get(url_for('get', _external=True))
    assert r.status_code == 200
    assert loads(r.text) == context

    conf_json['licensing']['url'] = request.url + 'test_header_error_on_licenseTimer'
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    drm_manager.activate()
    try:
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        wait_period = start + timedelta(seconds=lic_duration+2) - datetime.now()
        sleep(wait_period.total_seconds())
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
    finally:
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert async_cb.was_called
        assert async_cb.message is not None
        assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
        assert "License header check error" in async_cb.message


@pytest.mark.no_parallel
def test_session_id_error(accelize_drm, conf_json, cred_json, async_handler, live_server):
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
    srv_url = conf_json['licensing']['url']

    # Set initial context on the live server
    context = {'session_id':None, 'session_cnt':0, 'request_cnt':0}
    r = requests.post(url_for('set', _external=True), json=context)
    assert r.status_code == 200
    r = requests.get(url_for('get', _external=True))
    assert r.status_code == 200
    assert loads(r.text) == context

    conf_json['licensing']['url'] = request.url + 'test_session_id_error'
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Start session #1 to record
    drm_manager.activate()
    try:
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        wait_period = start + timedelta(seconds=lic_duration/2) - datetime.now()
        sleep(wait_period.total_seconds())
    finally:
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
    # Start session #2 to replay session #1
    drm_manager.activate()
    try:
        start = datetime.now()
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=lic_duration+2) - datetime.now()
        sleep(wait_period.total_seconds())
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
    finally:
        drm_manager.deactivate()
        assert async_cb.was_called
        assert async_cb.message is not None
        assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
        assert "License header check error" in async_cb.message
