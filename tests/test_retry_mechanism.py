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
from math import ceil

from tests.conftest import wait_func_true
from tests.proxy import get_context, set_context


@pytest.mark.no_parallel
def test_api_retry_disabled(accelize_drm, conf_json, cred_json, async_handler, live_server, basic_log_file):
    """
    Test retry mechanism is disabled on API function (not including the retry in background thread)
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_api_retry'
    conf_json['settings']['ws_api_retry_duration'] = 0  # Disable retry on function call
    conf_json['settings'].update(basic_log_file.create(2))
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
    del drm_manager
    assert (end - start).total_seconds() < 1
    log_content = basic_log_file.read()
    assert search(r'\[\s*critical\s*\]\s*\d+\s*,\s*\[errCode=\d+\]\s*Metering Web Service error 408',
            log_content, IGNORECASE)
    assert not search(r'attempt', log_content, IGNORECASE)
    async_cb.assert_NoError()
    basic_log_file.remove()


@pytest.mark.no_parallel
def test_api_retry_enabled(accelize_drm, conf_json, cred_json, async_handler, live_server, basic_log_file):
    """
    Test retry mechanism is working on API function (not including the retry in background thread)
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    retry_duration = 10
    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_api_retry'
    conf_json['settings']['ws_api_retry_duration'] = retry_duration  # Set retry duration to 10s
    conf_json['settings'].update(basic_log_file.create(2))
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager.get('license_status')
    retry_sleep = drm_manager.get('ws_retry_period_short')
    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    del drm_manager
    total_seconds = int((end - start).total_seconds())
    assert retry_duration <= total_seconds <= retry_duration + 1
    log_content = basic_log_file.read()
    m = search(r'\[\s*critical\s*\]\s*\d+\s*,\s*\[errCode=\d+\]\s*Timeout on License request after (\d+) attempts',
            log_content, IGNORECASE)
    assert m is not None
    nb_attempts = int(m.group(1))
    nb_attempts_expected = retry_duration / retry_sleep
    assert nb_attempts_expected - 1 <= nb_attempts <= nb_attempts_expected + 1
    async_cb.assert_NoError()
    basic_log_file.remove()


@pytest.mark.no_parallel
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


@pytest.mark.no_parallel
def test_retry_on_no_connection(accelize_drm, conf_json, cred_json, async_handler, live_server, basic_log_file):
    """
    Test the number of expected retries and the gap between 2 retries are correct when the requests are lost
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 2
    retryLongPeriod = 20
    licDuration = 60
    requestTimeout = 5
    nb_long_retry = ceil((licDuration - retryLongPeriod)/(retryLongPeriod + requestTimeout))
    nb_short_retry = ceil((licDuration - nb_long_retry*(retryLongPeriod + requestTimeout)) / (retryShortPeriod + requestTimeout))
    nb_retry = nb_long_retry + nb_short_retry

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_retry_on_no_connection'
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json['settings']['ws_request_timeout'] = requestTimeout
    conf_json['settings'].update(basic_log_file.create(1))
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
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSError.error_code
    m = search(r'Timeout on License request after (\d+) attempts', async_cb.message)
    assert m is not None
    nb_attempts = int(m.group(1))
    assert nb_retry == nb_attempts
    log_content = basic_log_file.read()
    attempts_list = [int(e) for e in findall(r'Attempt #(\d+) to obtain a new License failed with message', log_content)]
    assert len(attempts_list) == nb_retry
    assert sorted(list(attempts_list)) == list(range(1,nb_retry+1))
    basic_log_file.remove()
