# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import match, search, finditer, MULTILINE, IGNORECASE
from time import sleep, time
from json import loads
from datetime import datetime, timedelta
from flask import request, url_for
from dateutil import parser

from tests.conftest import wait_func_true
from tests.proxy import get_context, set_context


@pytest.mark.on_2_fpga
def test_retry_disabled(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test retry mechanism on API function (not including the retry in background thread)
    The retry is tested with one FPGA activated with a floating license and a 2nd FGPA
    that's requesting the same floating license but with a limit to 1 node.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]

    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_04')

    conf_json.reset()
    retry = 0
    conf_json['settings']['ws_retry_period_short'] = retry  # Disable retry on function call
    conf_json.save()
    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager1.activate()
        end = datetime.now()
        assert (end - start).total_seconds() < 1
        assert 'Metering Web Service error 470' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_04@accelize.com', str(excinfo.value)) is not None
        assert 'You have reached the maximum quantity of 1 seat(s) for floating entitlement' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        async_cb0.assert_NoError()
        async_cb1.assert_NoError()


@pytest.mark.on_2_fpga
def test_10s_retry(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test retry mechanism on API function (not including the retry in background thread)
    The retry is tested with one FPGA actiavted with a floating license and a 2nd FGPA
    that's requesting the same floating license but with a limit to 1 node.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]

    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_04')

    conf_json.reset()
    timeout = 10
    retry = 1
    conf_json['settings']['ws_request_timeout'] = timeout
    conf_json['settings']['ws_retry_period_short'] = retry
    conf_json.save()
    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager1.activate()
        end = datetime.now()
        m = search(r'Timeout on License request after (\d+) attempts', str(excinfo.value))
        assert m is not None
        assert int(m.group(1)) > 1
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        total_seconds = int((end - start).total_seconds())
        assert total_seconds >= timeout
        assert total_seconds <= timeout + 1
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        async_cb0.assert_NoError()
        async_cb1.assert_NoError()


@pytest.mark.no_parallel
def test_long_to_short_retry_switch(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test an error is returned if a wrong session id is provided
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 3
    retryLongPeriod = 10
    timeoutSecondFirst2 = 3
    timeoutSecond = 20

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_long_to_short_retry_switch'
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    context = {'data': list(),
               'timeoutSecondFirst2':timeoutSecondFirst2,
               'timeoutSecond':timeoutSecond
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    try:
        wait_func_true(lambda: async_cb.was_called,
                timeout=timeoutSecondFirst2 + 2*timeoutSecond + 2)
    finally:
        drm_manager.deactivate()
    assert async_cb.was_called
    assert 'Timeout on License' in async_cb.message
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSError.error_code
    context = get_context()
    data_list = context['data']
    nb_long_retry = int(timeoutSecond / retryLongPeriod)
    nb_short_retry = int(retryLongPeriod / retryShortPeriod)
    assert (nb_long_retry+nb_short_retry) <= len(data_list) <= (1+nb_long_retry+nb_short_retry)
    data = data_list.pop(0)    # Remove 'open' request
    assert data[0] == 'open'
    prev_lic = parser.parse(data[2])
    data = data_list.pop(0)    # Remove 1st 'running' request
    assert data[0] == 'running'
    lic_delta = int((parser.parse(data[1]) - prev_lic).total_seconds())
    assert lic_delta < 1
    prev_lic = parser.parse(data[2])
    for i, (type, start, end) in enumerate(data_list):
        lic_delta = int((parser.parse(start) - prev_lic).total_seconds())
        prev_lic = parser.parse(end)
        if i < nb_long_retry:
            assert (retryLongPeriod-1) <= lic_delta <= (retryLongPeriod+1)
        else:
            assert (retryShortPeriod-1) <= lic_delta <= (retryShortPeriod+1)


@pytest.mark.no_parallel
def test_retry_on_no_connection(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test an error is returned if a wrong session id is provided
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 3
    retryLongPeriod = 10
    timeoutSecond = 20

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_long_to_short_retry_switch'
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    context = {'data': list(),
               'timeoutSecond':timeoutSecond
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    try:
        wait_func_true(lambda: async_cb.was_called,
                timeout=timeoutSecond*2)
    finally:
        drm_manager.deactivate()
    assert async_cb.was_called
    assert 'Timeout on License' in async_cb.message
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSError.error_code
    context = get_context()
    data_list = context['data']
    nb_long_retry = int(timeoutSecond / retryLongPeriod)
    nb_short_retry = int(retryLongPeriod / retryShortPeriod)
    assert (nb_long_retry+nb_short_retry) <= len(data_list) <= (1+nb_long_retry+nb_short_retry)
    data = data_list.pop(0)    # Remove 'open' request
    assert data[0] == 'open'
    prev_lic = parser.parse(data[2])
    data = data_list.pop(0)    # Remove 1st 'running' request
    assert data[0] == 'running'
    lic_delta = int((parser.parse(data[1]) - prev_lic).total_seconds())
    assert lic_delta < 1
    prev_lic = parser.parse(data[2])
    for i, (type, start, end) in enumerate(data_list):
        lic_delta = int((parser.parse(start) - prev_lic).total_seconds())
        prev_lic = parser.parse(end)
        if i < nb_long_retry:
            assert (retryLongPeriod - 1) <= lic_delta <= (retryLongPeriod + 1)
        else:
            assert (retryShortPeriod - 1) <= lic_delta <= (retryShortPeriod + 1)

