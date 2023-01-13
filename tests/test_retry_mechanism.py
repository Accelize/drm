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

from tests.conftest import wait_until_true, HTTP_TIMEOUT_ERR_MSG
from tests.proxy import get_context, set_context, update_context


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
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert not drm_manager.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager.activate()
        end = datetime.now()
        assert (end - start).total_seconds() < 2
    log_content = logfile.read()
    assert search(r'\[\s*critical\s*\]\s*.+\[errCode=\d+\]\s*Accelize Web Service error 408 on HTTP request',
            log_content, IGNORECASE)
    assert len(findall(r'attempt', log_content, IGNORECASE)) == 1
    assert search(HTTP_TIMEOUT_ERR_MSG, log_content, IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSMayRetry.error_code, HTTP_TIMEOUT_ERR_MSG)
    async_cb.reset()
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
    api_retry_duration = 10
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.set(ws_api_retry_duration=api_retry_duration)
        retry_duration = drm_manager.get('ws_api_retry_duration')
        assert retry_duration == api_retry_duration
        assert not drm_manager.get('license_status')
        retry_sleep = drm_manager.get('ws_retry_period_short')
        nb_attempts_expected = retry_duration / retry_sleep
        assert nb_attempts_expected > 1
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
            drm_manager.activate()
        end = datetime.now()
    total_seconds = int((end - start).total_seconds())
    assert retry_duration - 1 <= total_seconds <= retry_duration + 1
    log_content = logfile.read()
    assert search(HTTP_TIMEOUT_ERR_MSG, log_content, IGNORECASE)
    m = search(r'\[\s*critical\s*\]\s*.+\[errCode=\d+\]\s*Timeout on License request after (\d+) attempts',
            log_content, IGNORECASE)
    assert m is not None
    nb_attempts = int(m.group(1))
    assert nb_attempts > 1
    assert nb_attempts_expected - 1 <= nb_attempts <= nb_attempts_expected + 1
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, 'Timeout on License request after')
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, HTTP_TIMEOUT_ERR_MSG)
    async_cb.reset()
    logfile.remove()


@pytest.mark.no_parallel
def test_long_to_short_retry_on_authentication(accelize_drm, conf_json,
            cred_json, async_handler, live_server, request, log_file_factory):
    """
    Test the number of expected retries and the gap between 2 retries
    on authentication requests occurring in the background thread
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    expires_in = 4
    retryShortPeriod = 2
    retryLongPeriod = 4
    retry_timeout = 5
    license_period_second = 25

    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json['settings']['ws_request_timeout'] = retry_timeout
    conf_json.save()

    # Set initial context on the live server
    context = {'expires_in': expires_in,
               'license_period_second': license_period_second,
               'data': list()}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: async_cb.was_called, lic_duration + 2*retry_timeout)
        update_context(allow=True)
        assert get_context('allow')
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSTimedOut.error_code
    assert search(r'Timeout on Authentication request after', async_cb.message, IGNORECASE)
    async_cb.reset()
    log_content = logfile.read()
    assert len(findall(r'Attempt #\d+ on Authentication request failed with message: .+ New attempt planned in \d+ seconds',
                log_content, IGNORECASE)) >= 3
    assert search(r'Timeout on Authentication request after \d+ attempts', log_content, IGNORECASE)
    # Analyze retry periods
    assert get_context('data')
    data_list = get_context('data')
    assert len(data_list) >= 3
    delta_list = list()
    prev_data = data_list.pop(0)
    for data in data_list:
        delta = int((parser.parse(data) - parser.parse(prev_data)).total_seconds())
        delta_list.append(delta)
        prev_data = data
    assert len(set(delta_list)) == 2
    delta_exp = retryLongPeriod
    for delta in delta_list:
        print('delta=', delta, 'exp=', delta_exp)
        try:
            assert delta_exp <= delta <= delta_exp+1
        except AssertionError:
            delta_exp = retryShortPeriod
            assert delta_exp <= delta <= delta_exp+1
    async_cb.assert_NoError()
    logfile.remove()


@pytest.mark.no_parallel
def test_long_to_short_retry_on_license(accelize_drm, conf_json, cred_json,
                       async_handler, live_server, request, log_file_factory):
    """
    Test the number of expected retries and the gap between 2 retries
    on license requests occurring in the background thread
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    retryShortPeriod = 3
    retryLongPeriod = 10
    retry_timeout = 20
    license_period_second = 18

    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json['settings']['ws_request_timeout'] = retry_timeout
    conf_json.save()

    # Set initial context on the live server
    context = {'data':list(),
               'license_period_second':license_period_second
    }
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: async_cb.was_called, lic_duration + 2*retry_timeout)
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSTimedOut.error_code
    assert search(r'Timeout on License request after \d+ attempts', async_cb.message, IGNORECASE)
    async_cb.reset()
    log_content = logfile.read()
    assert len(findall(r'Attempt #\d+ on License request failed with message: .+ New attempt planned in \d+ seconds',
                log_content, IGNORECASE)) >= 3
    assert search(r'Timeout on License request after \d+ attempts', log_content, IGNORECASE)
    # Analyze retry periods
    assert get_context('data')
    data_list = get_context('data')
    assert len(data_list) >= 3
    delta_list = list()
    prev_data = data_list.pop(0)
    for data in data_list:
        delta = int((parser.parse(data) - parser.parse(prev_data)).total_seconds())
        delta_list.append(delta)
        prev_data = data
    assert len(set(delta_list)) == 2
    delta_exp = retryLongPeriod
    for delta in delta_list:
        print('delta=', delta, 'exp=', delta_exp)
        try:
            assert delta_exp == delta
        except AssertionError:
            delta_exp = retryShortPeriod
            assert delta_exp == delta
    async_cb.assert_NoError()
    logfile.remove()


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

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        retry_duration = drm_manager.get('ws_api_retry_duration')
        retry_timeout = drm_manager.get('ws_request_timeout')
        retry_sleep = drm_manager.get('ws_retry_period_short')
        nb_attempts_expected = int(retry_duration / (retry_timeout + retry_sleep)) + 1
        assert nb_attempts_expected >= 1
        context = {'data': list(),
                   'sleep':retry_timeout + 1}
        set_context(context)
        assert get_context() == context
        with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
            drm_manager.activate()
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSTimedOut.error_code
    m = search(r'Timeout on License request after (\d+) attempts', str(excinfo.value))
    assert m is not None
    nb_attempts = int(m.group(1))
    assert nb_attempts_expected == nb_attempts
    log_content = logfile.read()
    attempts_list = [int(e) for e in findall(r'Attempt #(\d+) on License request failed with message', log_content)]
    assert len(attempts_list) == nb_attempts_expected
    assert sorted(list(attempts_list)) == list(range(1,nb_attempts + 1))
    # Check time between each call
    data = get_context('data')
    assert len(data) == nb_attempts_expected
    prev_time = data.pop(0)
    for time in data:
        delta = int((time - prev_time).total_seconds())
        assert retry_timeout + retry_sleep - 1 <= delta <= retry_timeout + retry_sleep
        prev_time = time
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, HTTP_TIMEOUT_ERR_MSG)
    async_cb.reset()
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
    retryLongPeriod = 8
    requestTimeout = 5
    license_period_second = 20

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json['settings']['ws_retry_period_short'] = retryShortPeriod
    conf_json['settings']['ws_retry_period_long'] = retryLongPeriod
    conf_json['settings']['ws_request_timeout'] = requestTimeout
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    context = {'data': list(),
               'license_period_second': license_period_second}
    set_context(context)
    assert get_context() == context

    nb_long_retry = ceil((license_period_second - retryLongPeriod - 2*retryShortPeriod) / (retryLongPeriod + requestTimeout))
    nb_short_retry = ceil((license_period_second - nb_long_retry*(retryLongPeriod + requestTimeout)) / (retryShortPeriod + requestTimeout))
    nb_retry = nb_long_retry + nb_short_retry


    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        wait_until_true(lambda: async_cb.was_called, timeout=2*license_period_second)
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSTimedOut.error_code
    m = search(r'Timeout on License request after (\d+) attempts', async_cb.message)
    assert m is not None
    nb_attempts = int(m.group(1))
    assert nb_retry == nb_attempts
    log_content = logfile.read()
    attempts_list = [int(e) for e in findall(r'Attempt #(\d+) on License request failed with message', log_content)]
    assert len(attempts_list) == nb_retry
    assert sorted(list(attempts_list)) == list(range(1,nb_retry+1))
    logfile.remove()
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, HTTP_TIMEOUT_ERR_MSG)
    async_cb.reset()

