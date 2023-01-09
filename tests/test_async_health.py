# -*- coding: utf-8 -*-
"""
Test asynchronous metering behaviors of DRM Library.
"""
import pytest
from time import sleep
from datetime import datetime
from random import randrange, randint
from re import search, findall, MULTILINE, IGNORECASE
from dateutil import parser
from itertools import groupby
from flask import request as _request
from requests import get, post
from os import remove
from os.path import realpath, isfile

from tests.conftest import wait_deadline, wait_func_true
from tests.proxy import get_context, set_context, get_proxy_error


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_counter_is_reset_on_new_session(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, log_file_factory, request):
    """
    Test the asynchronous health counter in the DRMLib is reset on new sessions.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    health_period = 4
    context = {'health_period': health_period}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('health_counter') == 0
        drm_manager.activate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] == 0
        lic_duration = drm_manager.get('license_duration')
        nb_health = lic_duration // health_period
        sleep(lic_duration+1)
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] >= nb_health
        drm_manager.deactivate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] >= nb_health
        drm_manager.activate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] == 0
        sleep(lic_duration+1)
        drm_manager.deactivate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] >= nb_health
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    log_content = logfile.read()
    assert search(r'Starting background thread which checks health', log_content, MULTILINE)
    assert search(r'Exiting background thread which checks health', log_content, MULTILINE)
    assert len(findall(r'Build health request', log_content)) >= nb_health
    logfile.remove()


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_period_disabled(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, log_file_factory, request):
    """
    Test the asynchronous health feature can be disabled.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    health_period = 4
    context = {'health_period': health_period}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('health_period') == get_context()['health_period'] == health_period
        wait_deadline(start, lic_duration + 1)
        wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_duration)
        cnt_health = get_context()['cnt_health']
        assert cnt_health == drm_manager.get('health_counter')
        assert drm_manager.get('health_period') == 0
        wait_deadline(start, lic_duration + health_period + 2)
        drm_manager.deactivate()
        assert cnt_health == drm_manager.get('health_counter') == get_context()['cnt_health']
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    log_content = logfile.read()
    assert findall(f"Found parameter 'health_period' of type Integer: return its value {health_period}", log_content)
    assert findall(f"Found parameter 'health_period' of type Integer: return its value 0", log_content)
    assert len(findall(r'Starting background thread which checks health', log_content, MULTILINE))
    assert len(findall(r'Health thread is disabled', log_content, MULTILINE))
    assert len(findall(r'Exiting background thread which checks health', log_content, MULTILINE))
    assert cnt_health == len(findall(r'Build health request', log_content))
    logfile.remove()


@pytest.mark.no_parallel
def test_health_period_modification(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, request, log_file_factory):
    """
    Test the asynchronous health feature can be modified dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    health_period = 3
    health_period_step = 3
    context = {'data': list(),
               'health_period': health_period,
               'health_period_step': health_period_step
    }
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        nb_health = lic_duration // health_period
        wait_func_true(lambda: get_context()['exit'], timeout=3*lic_duration)
        drm_manager.deactivate()
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    data_list = get_context()['data']
    assert len(data_list) >= nb_health + 1
    for start, end in data_list[:-2]:
        delta = int((parser.parse(end) - parser.parse(start)).total_seconds())
        assert health_period <= delta <= health_period + 1
    start, end = data_list[-1]
    delta = int((parser.parse(end) - parser.parse(start)).total_seconds())
    assert health_period+3 <= delta <= health_period+4
    log_content = logfile.read()
    assert findall(f"Found parameter 'health_period' of type Integer: return its value {health_period}", log_content)
    assert findall(f"Found parameter 'health_period' of type Integer: return its value {health_period+health_period_step}", log_content)
    logfile.remove()


@pytest.mark.no_parallel
def test_health_retry_disabled(accelize_drm, conf_json, cred_json, async_handler,
                            live_server, request, log_file_factory):
    """
    Test the asynchronous health retry feature can be disabled.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    health_period = 4
    health_retry = 3
    health_retry_sleep = 1
    nb_attempts = health_retry // health_retry_sleep
    context = {'data': list(),
               'health_period': health_period,
               'health_retry': health_retry,
               'health_retry_sleep': health_retry_sleep
    }
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        wait_func_true(lambda: get_context()['exit'], timeout=3*lic_duration)
        drm_manager.deactivate()
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    data_list = get_context()['data']
    assert len(data_list) >= (nb_attempts + 2)
    id_n_1 = ''
    for id, start, end in data_list:
        delta = int((parser.parse(end) - parser.parse(start)).total_seconds())
        if id_n_1 == '' or id_n_1 != id:
            assert health_period <= delta <= health_period + 1
        else:
            assert health_retry_sleep <= delta <= health_retry_sleep + 1
        id_n_1 = id
    assert data_list[-2][0] != data_list[-1][0]
    log_content = logfile.read()
    assert findall(r'Health retry is disabled', log_content)
    assert len(findall(r'warning.*Attempt #\d+ on Health request failed with message.*New attempt planned', log_content)) > nb_attempts
    assert len(findall(r'warning.*Attempt on Health request failed with message', log_content))
    logfile.remove()


@pytest.mark.no_parallel
def test_health_retry_modification(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, request, log_file_factory):
    """
    Test the asynchronous health retry can be modified dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    health_period = 2
    health_retry_step = 3
    health_retry = 3
    health_retry_sleep = 1
    context = {'data': list(),
           'health_period': health_period,
           'health_retry': health_retry,
           'health_retry_step': health_retry_step,
           'health_retry_sleep': health_retry_sleep
    }
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        wait_func_true(lambda: get_context()['cnt_license'] >= 4, timeout=3*lic_duration)
        drm_manager.deactivate()
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    data_list = get_context()['data']
    assert len(data_list) >= 2
    last_id = ''
    ref_start = None
    retry_list = list()
    for id, start, end in data_list:
        delta = int((parser.parse(end) - parser.parse(start)).total_seconds())
        if last_id != id:
            assert health_period <= delta <= health_period + 2
            if ref_start is not None:
                delta = int((parser.parse(ref_end) - parser.parse(ref_start)).total_seconds())
                retry_list.append(delta)
            ref_start = end
        else:
            assert health_retry_sleep <= delta <= health_retry_sleep + 1
        ref_end = end
        last_id = id
    delta = int((parser.parse(ref_end) - parser.parse(ref_start)).total_seconds())
    retry_list.append(delta)
    ref_delta = health_retry-1
    for d in retry_list:
        if ref_delta != d:
           ref_delta += health_retry_step
        assert d == ref_delta
    log_content = logfile.read()
    assert findall(f"Found parameter 'health_retry' of type Integer: return its value {health_retry}", log_content)
    assert findall(f"Found parameter 'health_retry' of type Integer: return its value {health_retry+health_retry_step}", log_content)
    assert findall(r'warning.*Attempt #\d+ on Health request failed with message.*New attempt planned', log_content)
    logfile.remove()


@pytest.mark.no_parallel
def test_health_retry_sleep_modification(accelize_drm, conf_json, cred_json,
                async_handler, live_server, request, log_file_factory):
    """
    Test the asynchronous health retry sleep value when changed dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    health_period = 2
    health_retry = 5
    health_retry_sleep = 1
    health_retry_sleep_step = 3
    context = {'data': list(),
           'health_period': health_period,
           'health_retry': health_retry,
           'health_retry_sleep': health_retry_sleep,
           'health_retry_sleep_step': health_retry_sleep_step
    }
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        wait_func_true(lambda: get_context()['cnt_license'] >= 4, timeout=3*lic_duration)
        drm_manager.deactivate()
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    data_list = get_context()['data']
    assert len(data_list) >= 2
    last_id = ''
    ref_start = None
    retry_list = list()
    for id, start, end in data_list:
        delta = int((parser.parse(end) - parser.parse(start)).total_seconds())
        if last_id != id:
            assert health_period <= delta <= health_period + 2
            if ref_start is not None:
                delta = int((parser.parse(ref_end) - parser.parse(ref_start)).total_seconds())
                retry_list.append(delta)
            ref_start = end
        else:
            if delta > health_retry_sleep + 1:
                health_retry_sleep += health_retry_sleep_step
            assert health_retry_sleep <= delta <= health_retry_sleep + 1
        ref_end = end
        last_id = id
    delta = int((parser.parse(ref_end) - parser.parse(ref_start)).total_seconds())
    retry_list.append(delta)
    ref_delta = health_retry-1
    for d in retry_list:
        if ref_delta != d:
           ref_delta += health_retry_sleep_step
        assert d == ref_delta
    log_content = logfile.read()
    assert findall(f"Found parameter 'health_retry_sleep' of type Integer: return its value {health_retry_sleep}", log_content)
    assert findall(f"Found parameter 'health_retry_sleep' of type Integer: return its value {health_retry_sleep+health_retry_sleep_step}", log_content)
    assert findall(r'warning.*Attempt #\d+ on Health request failed with message.*New attempt planned', log_content)
    logfile.remove()


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_metering_data(accelize_drm, conf_json, cred_json, async_handler,
                    live_server, ws_admin, request, log_file_factory):
    """
    Test the metering data returned to the web service is correct.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    loop = 4
    health_period = 2
    health_retry = 0  # No retry
    context = {'health_period': health_period,
               'health_retry': health_retry    }
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        def wait_and_check_on_next_health(drm):
            cnt_health = drm.get('health_counter')
            session_id = drm.get('session_id')
            metering_data = drm.get('metered_data')
            wait_func_true(lambda: drm.get('health_counter') > cnt_health)
            assert session_id == drm.get('session_id')
            assert sum(metering_data) <= sum(drm.get('metered_data'))

        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert sum(drm_manager.get('metered_data')) == 0
        activators.check_coin(drm_manager.get('metered_data'))
        wait_and_check_on_next_health(drm_manager)
        total_coin = 0
        for i in range(loop):
            total_coin += activators.generate_coin()
            activators.check_coin(drm_manager.get('metered_data'))
            wait_and_check_on_next_health(drm_manager)
        assert sum(drm_manager.get('metered_data')) == total_coin
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
    assert get_proxy_error() is None
    async_cb.assert_NoError()
    log_content = logfile.read()
    assert search(r'Starting background thread which checks health', log_content)
    assert search(r'Sending new health info', log_content)
    logfile.remove()


@pytest.mark.no_parallel
def test_segment_index(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, log_file_factory, request):
    """
    Test the DRM Controller capacity to handle stressfully health and license requests
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Adjust health period to license duration
    nb_samples = 4
    health_period = 2
    context = {'health_period': health_period,
               'health_retry': 0}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('health_period') == health_period
        wait_func_true(lambda: get_context()['cnt_health'] >= nb_samples,
                timeout=nb_samples // (lic_duration // health_period) + 1)
        drm_manager.deactivate()
    async_cb.assert_NoError()
    assert get_proxy_error() is None
    # Analyze results
    data_list = get_context()['data']
    segment_idx_expected = 0
    for metering_file in data_list:
        print(metering_file)
        if session_id == "0000000000000000":
            assert close_flag == '0'
            assert segment_idx == 0
            session_id_exp = "0000000000000000"
        else:
            if session_id_exp == "0000000000000000":
                session_id_exp == session_id
            else:
                assert session_id == session_id_exp
        assert segment_idx_expected - 1 <= segment_idx <= segment_idx_expected + 1
        segment_idx_expected += 1
        if close_flag == '1':
            segment_idx_expected = 0
    log_content = logfile.read()
    assert findall(r'Health retry is disabled', log_content)
    assert search(r'Starting background thread which checks health', log_content)
    assert len(findall(r'Sending new health info', log_content)) ==  len(data_list)
    logfile.remove()
