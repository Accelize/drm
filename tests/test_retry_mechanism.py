# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import match, search, finditer, findall, MULTILINE, IGNORECASE
from time import sleep, time
from json import loads
from datetime import datetime, timedelta
from flask import request
from dateutil import parser

from tests.conftest import wait_func_true, whoami
from tests.proxy import get_context, set_context


def test_api_retry_disabled(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test retry mechanism is disabled on API function (not including the retry in background thread)
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_api_retry'
    conf_json['settings']['ws_api_retry_period'] = 0  # Disable retry on function call
    logpath = accelize_drm.create_log_path(whoami())
    conf_json['settings']['log_file_verbosity'] = accelize_drm.create_log_level(1)
    conf_json['settings']['log_file_path'] = logpath
    conf_json['settings']['log_file_type'] = 1
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager.get('license_status')
    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    assert (end - start).total_seconds() < 1
    with open(logpath, 'rt') as f:
        log_content = f.read()
    assert 'Metering Web Service error 408' in log_content
    assert 'DRM WS request failed' in log_content
    async_cb.assert_NoError()


def test_api_retry_enabled(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test retry mechanism is working on API function (not including the retry in background thread)
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_api_retry'
    conf_json['settings']['ws_api_retry_period'] = 0  # Disable retry on function call
    logpath = accelize_drm.create_log_path(whoami())
    conf_json['settings']['log_file_verbosity'] = accelize_drm.create_log_level(1)
    conf_json['settings']['log_file_path'] = logpath
    conf_json['settings']['log_file_type'] = 1
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager.get('license_status')
    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    total_seconds = int((end - start).total_seconds())
    assert total_seconds >= timeout
    assert total_seconds <= timeout + 1
    assert (end - start).total_seconds() < 1
    with open(logpath, 'rt') as f:
        log_content = f.read()
    assert 'Metering Web Service error 408' in log_content
    assert 'DRM WS request failed' inlog_content
    m = search(r'Timeout on License request after (\d+) attempts', log_content)
    assert m is not None
    assert int(m.group(1)) > 1
    async_cb.assert_NoError()


def test_long_to_short_retry_switch(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the number of expected retris and the gap between 2 retries are correct when a retryable error is returned
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

    context = {'data':list(),
               'cnt':0,
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
    data = data_list.pop(0)
    assert data[0] == 'running'
    prev_lic = parser.parse(data[2])
    for i, (type, start, end) in enumerate(data_list):
        lic_delta = int((parser.parse(start) - prev_lic).total_seconds())
        prev_lic = parser.parse(end)
        if i < nb_long_retry-1:
            assert (retryLongPeriod-1) <= lic_delta <= (retryLongPeriod+1)
        else:
            assert (retryShortPeriod-1) <= lic_delta <= (retryShortPeriod+1)


def test_retry_on_no_connection(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the number of expected retris and the gap between 2 retries are correct when the requests are lost
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 5
    retryLongPeriod = 20
    licDuration = 60
    requestTimeout = 5
    nb_long_retry = int(licDuration / (retryLongPeriod + requestTimeout + 1))
    nb_short_retry = int(retryLongPeriod / (retryShortPeriod + requestTimeout + 1)) + 1
    nb_retry = nb_long_retry + nb_short_retry

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_retry_on_no_connection'
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json['settings']['ws_request_timeout'] = requestTimeout
    logpath = accelize_drm.create_log_path(whoami())
    conf_json['settings']['log_file_verbosity'] = accelize_drm.create_log_level(1)
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    context = {'cnt':0,
               'timeoutSecond':licDuration
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    try:
        wait_func_true(lambda: async_cb.was_called,
                timeout=licDuration*2)
    finally:
        drm_manager.deactivate()
    del drm_manager
    assert async_cb.was_called
    assert search(r'Timeout on License request after %d attempts' % nb_retry,
            async_cb.message)
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSError.error_code
    wait_func_true(lambda: isfile(logpath), 10)
    with open(logpath, 'rt') as f:
        log_content = f.read()
    attempts_list = [int(e) for e in findall(r'Attempt #(\d+) to obtain a new License failed with message', log_content)]
    assert len(attempts_list) == nb_retry
    assert sorted(list(attempts_list)) == list(range(1,nb_retry+1))
