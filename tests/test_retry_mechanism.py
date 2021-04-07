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
from flask import request as _request
from dateutil import parser
from math import ceil

from tests.conftest import wait_func_true
from tests.proxy import get_context, set_context


@pytest.mark.no_parallel
def test_api_retry_disabled(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, log_file_factory, request):
    """
    Test retry mechanism is disabled on API function (not including
    the retry in background thread)
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + 'test_api_retry'
    conf_json['settings']['ws_api_retry_duration'] = 0  # Disable retry on function call
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
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
    log_content = logfile.read()
    assert search(r'\[\s*critical\s*\]\s*\d+\s*,\s*\[errCode=\d+\]\s*Metering Web Service error 408',
            log_content, IGNORECASE)
    assert not search(r'attempt', log_content, IGNORECASE)
    async_cb.assert_NoError()
    logfile.remove()


@pytest.mark.no_parallel
def test_api_retry_enabled(accelize_drm, conf_json, cred_json, async_handler,
                   live_server, log_file_factory, request):
    """
    Test retry mechanism is working on API function (not including the
    retry in background thread)
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = _request.url + 'test_api_retry'
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    retry_duration = drm_manager.get('ws_api_retry_duration')
    assert not drm_manager.get('license_status')
    retry_sleep = drm_manager.get('ws_retry_period_short')
    nb_attempts_expected = retry_duration / retry_sleep
    assert nb_attempts_expected > 1
    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    del drm_manager
    total_seconds = int((end - start).total_seconds())
    assert retry_duration - 1 <= total_seconds <= retry_duration
    log_content = logfile.read()
    m = search(r'\[\s*critical\s*\]\s*\d+\s*,\s*\[errCode=\d+\]\s*Timeout on License request after (\d+) attempts',
            log_content, IGNORECASE)
    assert m is not None
    nb_attempts = int(m.group(1))
    assert nb_attempts > 1
    assert nb_attempts_expected - 1 <= nb_attempts <= nb_attempts_expected
    async_cb.assert_NoError()
    logfile.remove()


@pytest.mark.no_parallel
def test_long_to_short_retry_switch_on_authentication(accelize_drm, conf_json,
                        cred_json, async_handler, live_server, request):
    """
    Test the number of expected retries and the gap between 2 retries
    on authentication requests occurring in the background thread
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    expires_in = 1
    retryShortPeriod = 3
    retryLongPeriod = 10
    timeoutSecond = 25

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
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
               'expires_in':expires_in,
               'timeoutSecond':timeoutSecond,
               'exit': False
    }
    set_context(context)
    assert get_context() == context

    try:
        drm_manager.activate()
        wait_func_true(lambda: async_cb.was_called, timeout=timeoutSecond*2)
    finally:
        context = get_context()
        context['exit'] = True
        set_context(context)
        assert get_context() == context
        drm_manager.deactivate()
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSError.error_code
    assert search(r'Timeout on Authentication request after', async_cb.message, IGNORECASE)
    context = get_context()
    data_list = context['data']
    data = data_list.pop(0)
    data = data_list.pop(-1)
    assert len(data_list) >= 3
    data = data_list.pop(0)
    prev_lic = parser.parse(data[1])
    for start, end in data_list:
        lic_delta = int((parser.parse(start) - prev_lic).total_seconds())
        prev_lic = parser.parse(end)
        if lic_delta > retryShortPeriod:
            assert (retryLongPeriod-1) <= lic_delta <= retryLongPeriod
        else:
            assert (retryShortPeriod-1) <= lic_delta <= retryShortPeriod


@pytest.mark.no_parallel
def test_long_to_short_retry_switch_on_license(accelize_drm, conf_json, cred_json,
                           async_handler, live_server, request):
    """
    Test the number of expected retries and the gap between 2 retries
    on license requests occurring in the background thread
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 3
    retryLongPeriod = 10
    timeoutSecond = 25

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
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
               'timeoutSecond':timeoutSecond
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    lic_duration = drm_manager.get('license_duration')
    try:
        wait_func_true(lambda: async_cb.was_called,
                timeout= lic_duration + 2*timeoutSecond)
    finally:
        drm_manager.deactivate()
    assert async_cb.was_called
    assert 'Timeout on License' in async_cb.message
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSError.error_code
    context = get_context()
    data_list = context['data']
    data = data_list.pop(0)
    data = data_list.pop(-1)
    assert len(data_list) >= 3
    data = data_list.pop(0)
    assert data[0] == 'running'
    prev_lic = parser.parse(data[2])
    for type, start, end in data_list:
        assert type == 'running'
        lic_delta = int((parser.parse(start) - prev_lic).total_seconds())
        prev_lic = parser.parse(end)
        if lic_delta > retryShortPeriod:
            assert (retryLongPeriod-1) <= lic_delta <= retryLongPeriod
        else:
            assert (retryShortPeriod-1) <= lic_delta <= retryShortPeriod


@pytest.mark.no_parallel
def test_api_retry_on_lost_connection(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, log_file_factory, request):
    """
    Test the number of expected retries and the gap between 2 retries
    are correct when the requests are lost on the activate call
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    retry_duration = drm_manager.get('ws_api_retry_duration')
    retry_timeout = drm_manager.get('ws_request_timeout')
    retry_sleep = drm_manager.get('ws_retry_period_short')
    nb_attempts_expected = int(retry_duration / (retry_timeout + retry_sleep)) + 1
    assert nb_attempts_expected > 1

    context = {'data': list(),
               'sleep':retry_timeout + 1}
    set_context(context)
    assert get_context() == context

    try:
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager.activate()
    except:
        drm_manager.deactivate()
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
    m = search(r'Timeout on License request after (\d+) attempts', str(excinfo.value))
    assert m is not None
    nb_attempts = int(m.group(1))
    assert nb_attempts_expected == nb_attempts
    log_content = logfile.read()
    attempts_list = [int(e) for e in findall(r'Attempt #(\d+) to obtain a new License failed with message', log_content)]
    assert len(attempts_list) == nb_attempts_expected
    assert sorted(list(attempts_list)) == list(range(1,nb_attempts + 1))
    # Check time between each call
    data = get_context()['data']
    assert len(data) == nb_attempts_expected
    prev_time = parser.parse(data.pop(0))
    for time in data:
        delta = int((parser.parse(time) - prev_time).total_seconds())
        assert retry_timeout + retry_sleep - 1 <= delta <= retry_timeout + retry_sleep
        prev_time = parser.parse(time)
    async_cb.assert_NoError()
    logfile.remove()


@pytest.mark.no_parallel
def test_thread_retry_on_lost_connection(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, log_file_factory, request):
    """
    Test the number of expected retries and the gap between 2 retries
    are correct when the requests are lost on the background thread
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 2
    retryLongPeriod = 20
    licDuration = 60
    requestTimeout = 5

    nb_long_retry = ceil((licDuration - retryLongPeriod - 2*retryShortPeriod) / (retryLongPeriod + requestTimeout))
    nb_short_retry = ceil((licDuration - nb_long_retry*(retryLongPeriod + requestTimeout)) / (retryShortPeriod + requestTimeout))
    nb_retry = nb_long_retry + nb_short_retry

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json['settings']['ws_request_timeout'] = requestTimeout
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
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

    try:
        drm_manager.activate()
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
    log_content = logfile.read()
    attempts_list = [int(e) for e in findall(r'Attempt #(\d+) to obtain a new License failed with message', log_content)]
    assert len(attempts_list) == nb_retry
    assert sorted(list(attempts_list)) == list(range(1,nb_retry+1))
    logfile.remove()
