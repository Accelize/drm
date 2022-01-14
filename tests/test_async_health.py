# -*- coding: utf-8 -*-
"""
Test asynchronous metering behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randrange, randint
from re import search, findall, MULTILINE, IGNORECASE
from dateutil import parser
from itertools import groupby
from flask import request as _request
from requests import get, post
from os import remove
from os.path import realpath, isfile

from tests.conftest import wait_func_true
from tests.proxy import get_context, set_context, get_proxy_error


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
    nb_health = 2
    healthPeriod = 2
    context = {'cnt':0, 'healthPeriod':healthPeriod, 'nb_health':nb_health, 'exit':False}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
                conf_json.path, cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        assert drm_manager.get('health_period') == healthPeriod
        wait_func_true(lambda: get_context()['exit'],
                timeout=healthPeriod * (nb_health + 1) * 2)
        assert drm_manager.get('health_period') == 0
        assert get_context()['cnt'] == nb_health
        sleep(healthPeriod + 1)
        assert get_context()['cnt'] == nb_health
        drm_manager.deactivate()
    log_content = logfile.read()
    assert search(r'Exiting background thread which checks health', log_content, MULTILINE)
    assert search(r'Health thread is disabled', log_content, MULTILINE)
    assert search(r'Exiting background thread which checks health', log_content, MULTILINE)
    health_req = findall(r'"request"\s*:\s*"health"', log_content)
    assert len(list(health_req)) == nb_health
    assert get_proxy_error() is None
    async_cb.assert_NoError()
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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    nb_health = 4
    healthPeriod = 2
    healthRetry = 0  # no retry
    healthRetrySleep = 1
    context = {'data': list(),
               'healthPeriod':healthPeriod,
               'healthRetry':healthRetry,
               'healthRetrySleep':healthRetrySleep
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
        wait_func_true(lambda: len(get_context()['data']) >= nb_health,
                timeout=(healthPeriod+3) * (nb_health+2))
        drm_manager.deactivate()
    async_cb.assert_NoError()
    data_list = get_context()['data']
    assert len(data_list) >= nb_health
    wait_start = data_list.pop(0)[1]
    for i, (start, end) in enumerate(data_list):
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod + i
        wait_start = end
    assert get_proxy_error() is None


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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        nb_health = 2
        healthPeriod = 3
        healthRetrySleep = 1
        context = {'data': list(),
                   'healthPeriod':healthPeriod,
                   'healthRetrySleep':healthRetrySleep
        }
        set_context(context)
        assert get_context() == context

        drm_manager.activate()
        wait_func_true(lambda: len(get_context()['data']) >= nb_health,
                timeout=(healthPeriod+3) * (nb_health+2))
        drm_manager.deactivate()
    async_cb.assert_NoError()
    data_list = get_context()['data']
    assert len(data_list) >= nb_health
    # Check there is no duplicated health_id
    id_list = tuple(map(lambda x: x[0], data_list))
    assert id_list == tuple(set(id_list))
    # Check the time between 2 period
    wait_start = data_list.pop(0)[2]
    for hid, start, end in data_list:
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod
        wait_start = end
    assert get_proxy_error() is None


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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    healthPeriod = 3
    healthRetry = 10
    healthRetrySleep = 1
    nb_run = 3

    for i in range(nb_run):

        retry_timeout = healthRetry+5*i

        # Set initial context on the live server
        context = {'data': list(),
               'healthPeriod':healthPeriod,
               'healthRetry':retry_timeout,
               'healthRetrySleep':healthRetrySleep,
               'exit':False
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
            wait_func_true(lambda: get_context()['exit'],
                timeout=(retry_timeout+3) * 2)
            drm_manager.deactivate()
            error_gap = drm_manager.get('health_retry_sleep') + 1

        async_cb.assert_NoError()
        data_list = get_context()['data']
        data0 = data_list.pop(0)
        assert len(data_list) > 1
        assert data0[0] == 0
        # Check health_id is unchanged during the retry period
        assert sum(map(lambda x: x[0], data_list)) == len(data_list)
        # Check the retry period is correct
        start = data_list[0][1]
        end = data_list[-1][2]
        delta = parser.parse(end) - parser.parse(start)
        assert retry_timeout - error_gap <= int(delta.total_seconds()) <= retry_timeout + error_gap
        assert get_proxy_error() is None


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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    healthPeriod = 3
    healthRetry = 10
    healthRetrySleep = 1
    nb_run = 3

    for i in range(nb_run):

        health_retry_sleep = healthRetrySleep + i

        # Set initial context on the live server
        context = {'data': list(),
               'healthPeriod':healthPeriod,
               'healthRetry':healthRetry,
               'healthRetrySleep':health_retry_sleep,
               'exit':False
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
            wait_func_true(lambda: get_context()['exit'],
                timeout=(healthRetry + healthRetry)*2)
            drm_manager.deactivate()

        async_cb.assert_NoError()
        data_list = get_context()['data']
        data0 = data_list.pop(0)
        nb_sleep_prev = 0
        # Check the retry sleep period is correct
        for health_id, group in groupby(data_list, lambda x: x[0]):
            group = list(group)
            assert len(group) > nb_sleep_prev
            nb_sleep_prev = len(group)
            start = group.pop(0)[2]
            for _, lstart, lend in group:
                delta = parser.parse(lstart) - parser.parse(start)
                assert int(delta.total_seconds()) == health_retry_sleep
                start = lend
        assert get_proxy_error() is None


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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        loop = 5
        healthPeriod = 3
        healthRetry = 0  # No retry
        context = {'health_id':0,
                   'healthPeriod':healthPeriod,
                   'healthRetry':healthRetry
        }
        set_context(context)
        assert get_context() == context

        def wait_and_check_on_next_health(drm):
            next_health_id = get_context()['health_id'] + 1
            wait_func_true(lambda: get_context()['health_id'] >= next_health_id)
            session_id = drm.get('session_id')
            saas_data = ws_admin.get_last_metering_information(session_id)
            assert saas_data['session'] == session_id
            assert saas_data['metering'] == sum(drm.get('metered_data'))

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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        nb_genlic = 3
        healthPeriod = 300
        healthRetry = 0  # no retry
        context = {'nb_genlic':0,
                   'healthPeriod':healthPeriod,
                   'healthRetry':healthRetry
        }
        set_context(context)
        assert get_context() == context

        # First, get license duration to align health period on it
        drm_manager.activate()
        lic_dur = drm_manager.get('license_duration')
        drm_manager.deactivate()

        # Adjust health period to license duration
        healthPeriod = lic_dur
        context = {'nb_genlic':0,
                   'healthPeriod':healthPeriod,
                   'healthRetry':healthRetry
        }
        set_context(context)
        assert get_context() == context

        drm_manager.activate()
        assert drm_manager.get('health_period') == healthPeriod
        wait_func_true(lambda: get_context()['nb_genlic'] >= nb_genlic,
                timeout=lic_dur * nb_genlic + 2)
        drm_manager.deactivate()
    async_cb.assert_NoError()
    log_content = logfile.read()
    segment_idx_expected = 0
    for m in findall(r'"meteringFile"\s*:\s*"([^"]*)"', log_content):
        assert len(m) > 0
        session_id = m[0:16]
        close_flag = m[19]
        segment_idx = int(m[24:32],16)
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
    assert get_proxy_error() is None
    logfile.remove()


@pytest.mark.no_parallel
def test_async_call_on_pause_when_health_is_enabled(accelize_drm, conf_json, cred_json,
                            async_handler, live_server, log_file_factory, request):
    """
    Test the DRM pause function does perform a async request before pausing
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + 'test_async_call_on_pause_depending_on_health_status'
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        context = {'healthPeriod':300,
                   'healthRetry':0,
                   'health_cnt':0
        }
        set_context(context)
        assert get_context() == context

        # First, get license duration to align health period on it
        drm_manager.activate()
        lic_dur = drm_manager.get('license_duration')
        drm_manager.deactivate(True) # Pause session
        drm_manager.deactivate()

    async_cb.assert_NoError()
    # Check the proxy received only 1 health request (corresponding to the pause call)
    context = get_context()
    assert context['health_cnt'] == 1
    # Check the health request occurred after the pause call
    pause_line = 0
    health_line = 0
    stop_line = 0
    for i, line in enumerate(logfile.read().split('\n')):
        if search(r"'pause_session_request'\s*=\s*true", line, IGNORECASE):
            pause_line = i
        elif search(r'"request"\s*:\s*"health"', line, IGNORECASE):
            health_line = i
        elif search(r"'pause_session_request'\s*=\s*false", line, IGNORECASE):
            stop_line = i
    assert pause_line > 0 and health_line > 0 and stop_line > 0
    assert pause_line < health_line < stop_line
    assert get_proxy_error() is None
    logfile.remove()


@pytest.mark.no_parallel
def test_no_async_call_on_pause_when_health_is_disabled(accelize_drm, conf_json, cred_json,
                        async_handler, live_server, log_file_factory, request):
    """
    Test the DRM pause function does NOT perform a async request before pausing
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + 'test_async_call_on_pause_depending_on_health_status'
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path, cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        context = {'healthPeriod':0,
                   'healthRetry':0,
                   'health_cnt':0
        }
        set_context(context)
        assert get_context() == context

        # First, get license duration to align health period on it
        drm_manager.activate()
        lic_dur = drm_manager.get('license_duration')
        drm_manager.deactivate(True) # Pause session
        drm_manager.deactivate()
    async_cb.assert_NoError()
    # Check the proxy did not receive any health request
    context = get_context()
    assert context['health_cnt'] == 0
    # Check no health request appeared in the log file
    assert search(r'"request"\s*:\s*"health"', logfile.read(), IGNORECASE) is None
    assert get_proxy_error() is None
    logfile.remove()
