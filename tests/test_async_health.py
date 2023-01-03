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
    context = {'nb_health':nb_health}
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
        assert drm_manager.get('health_period') == get_context()['health_period'] > 0
        wait_deadline(start, lic_duration)
        wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_duration)
        assert drm_manager.get('health_period') == get_context()['health_period'] == 0
        assert drm_manager.get('health_counter') == nb_health
        wait_deadline(start, 2*lic_duration)
        wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_duration)
        assert drm_manager.get('health_period') == get_context()['health_period'] > 0
        wait_deadline(start, 3*lic_duration + 2)
        drm_manager.deactivate()
        health_cnt = drm_manager.get('health_counter')
        assert health_cnt == 2*nb_health == get_context()['cnt_health']
    log_content = logfile.read()
    assert len(findall(r'Starting background thread which checks health', log_content, MULTILINE)) == 2
    assert len(findall(r'Health thread is disabled', log_content, MULTILINE)) == 1
    assert len(findall(r'Exiting background thread which checks health', log_content, MULTILINE)) == 2
    assert health_cnt == 2*nb_health == len(findall(r'Build health request', log_content))
    assert get_proxy_error() is None
    async_cb.assert_NoError()
    logfile.remove()


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
    nb_health = 2
    context = {'nb_health':nb_health}
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
        sleep(lic_duration+1)
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] == nb_health
        drm_manager.deactivate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] == nb_health
        drm_manager.activate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] == 0
        sleep(lic_duration+1)
        drm_manager.deactivate()
        assert drm_manager.get('health_counter') == get_context()['cnt_health'] == nb_health

    log_content = logfile.read()
    assert search(r'Starting background thread which checks health', log_content, MULTILINE)
    assert search(r'Exiting background thread which checks health', log_content, MULTILINE)
    assert len(findall(r'Build health request', log_content)) == 2*nb_health
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
    health_period = 2
    health_retry = 0  # no retry
    health_retry_sleep = 1
    context = {'data': list(),
               'health_period':health_period,
               'exit': False
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
    data_list = get_context()['data']
    assert len(data_list) >= nb_health + 2
    delta_cnt = [0,0]
    for i, (start, end) in enumerate(data_list):
        delta = parser.parse(end) - parser.parse(start)
        if delta.total_seconds() >= 2*health_period:
            delta_cnt[1] += 1
            assert 2*health_period <= int(delta.total_seconds()) <= 2*health_period + 1
        else:
            delta_cnt[0] += 1
            assert health_period <= int(delta.total_seconds()) <= health_period + 1
    assert delta_cnt[0] >= nb_health
    assert delta_cnt[1] == 2
    assert get_proxy_error() is None
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
        health_period = 3
        health_retry_sleep = 1
        context = {'data': list(),
                   'health_period':health_period,
                   'health_retry_sleep':health_retry_sleep
        }
        set_context(context)
        assert get_context() == context

        drm_manager.activate()
        wait_func_true(lambda: len(get_context()['data']) >= nb_health,
                timeout=(health_period+3) * (nb_health+2))
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
        assert int(delta.total_seconds()) == health_period
        wait_start = end
    assert get_proxy_error() is None
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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    health_period = 3
    health_retry = 10
    health_retry_sleep = 1
    nb_run = 3

    for i in range(nb_run):

        retry_timeout = health_retry+5*i

        # Set initial context on the live server
        context = {'data': list(),
               'health_period':health_period,
               'health_retry':retry_timeout,
               'health_retry_sleep':health_retry_sleep,
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
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    health_period = 3
    health_retry = 10
    health_retry_sleep = 1
    nb_run = 3

    for i in range(nb_run):

        health_retry_sleep = health_retry_sleep + i

        # Set initial context on the live server
        context = {'data': list(),
               'health_period':health_period,
               'health_retry':health_retry,
               'health_retry_sleep':health_retry_sleep,
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
                timeout=(health_retry + health_retry)*2)
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
        health_period = 3
        health_retry = 0  # No retry
        context = {'health_id':0,
                   'health_period':health_period,
                   'health_retry':health_retry
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
        health_period = 300
        health_retry = 0  # no retry
        context = {'nb_genlic':0,
                   'health_period':health_period,
                   'health_retry':health_retry
        }
        set_context(context)
        assert get_context() == context

        # First, get license duration to align health period on it
        drm_manager.activate()
        lic_dur = drm_manager.get('license_duration')
        drm_manager.deactivate()

        # Adjust health period to license duration
        health_period = lic_dur
        context = {'nb_genlic':0,
                   'health_period':health_period,
                   'health_retry':health_retry
        }
        set_context(context)
        assert get_context() == context

        drm_manager.activate()
        assert drm_manager.get('health_period') == health_period
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
