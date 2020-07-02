# -*- coding: utf-8 -*-
"""
Test asynchronous metering behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randrange
from re import search, MULTILINE
from dateutil import parser
from itertools import groupby
from flask import request, url_for
from requests import get, post

from tests.conftest import wait_func_true
from tests.proxy import get_context, set_context


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_period_disabled(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health feature can be disabled.
    """
    from os import remove
    from os.path import realpath, isfile

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_period_disabled'
    logpath = realpath("./test_health_disabled.%d.log" % randrange(0xFFFFFFFF))
    conf_json['settings']['log_file_verbosity'] = 1
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

    # Set initial context on the live server
    nb_health = 2
    healthPeriod = 2
    context = {'cnt':0, 'healthPeriod':healthPeriod, 'nb_health':nb_health, 'exit':False}
    set_context(context)
    assert get_context() == context

    try:
        drm_manager.activate()
        try:
            wait_func_true(lambda: get_context()['cnt'] > 0,
                    timeout=healthPeriod * nb_health)
            assert drm_manager.get('health_period') == healthPeriod
            wait_func_true(lambda: get_context()['exit'],
                    timeout=healthPeriod * nb_health)
            sleep(healthPeriod * 2)
            assert drm_manager.get('health_period') == 0
            assert get_context()['cnt'] == nb_health + 1
        finally:
            drm_manager.deactivate()
            del drm_manager
        async_cb.assert_NoError()
        assert wait_func_true(lambda: isfile(logpath), 10)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        assert search(r'Health thread is disabled', log_content, MULTILINE)
    finally:
        if isfile(logpath):
            remove(logpath)


@pytest.mark.skip
@pytest.mark.no_parallel
def test_health_period(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health period is correct.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_period'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Set initial context on the live server
    nb_health = 2
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

    drm_manager.activate()
    try:
        wait_func_true(lambda: len(get_context()['data']) > nb_health + 1,
                timeout=(healthPeriod+3) * (nb_health+2))
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    data_list = get_context()['data']
    assert len(data_list) >= nb_health+1
    wait_start = data_list.pop(0)[1]
    for start, end in data_list:
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod
        wait_start = end


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_period_modification(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health feature can be modified dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_period_modification'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

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

    drm_manager.activate()
    try:
        wait_func_true(lambda: len(get_context()['data']) >= (nb_health*2 + 1),
                timeout=(healthPeriod+3) * (nb_health+2))
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    data_list = get_context()['data']
    assert len(data_list) >= nb_health+1
    wait_start = data_list.pop(0)[1]
    for i, (start, end) in enumerate(data_list):
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod + i
        wait_start = end


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_retry_disabled(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health retry feature can be disabled.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_retry_disabled'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

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
    try:
        wait_func_true(lambda: len(get_context()['data']) >= (nb_health + 1),
                timeout=(healthPeriod+3) * (nb_health+2))
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    data_list = get_context()['data']
    assert len(data_list) >= nb_health+1
    # Check there is no duplicated health_id
    id_list = tuple(map(lambda x: x[0], data_list))
    assert id_list == tuple(set(id_list))
    # Check the time between 2 period
    wait_start = data_list.pop(0)[2]
    for hid, start, end in data_list:
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod
        wait_start = end


@pytest.mark.skip
@pytest.mark.no_parallel
def test_health_retry(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health retry.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_retry'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Set initial context on the live server
    healthPeriod = 2
    healthRetry = 15
    healthRetrySleep = 1
    context = {'data': list(),
               'healthPeriod':healthPeriod,
               'healthRetry':healthRetry,
               'healthRetrySleep':healthRetrySleep,
               'exit':False
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    try:
        wait_func_true(lambda: get_context()['exit'],
                timeout=healthRetry*2)
    finally:
        drm_manager.deactivate()
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
    error_gap = drm_manager.get('health_retry_sleep') + 1
    assert healthRetry - error_gap <= int(delta.total_seconds()) <= healthRetry + error_gap


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_retry_modification(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health retry can be modified dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_retry_modification'
    conf_json.save()

    timeoutSecond = 300
    healthPeriod = 1
    healthRetry = 0
    healthRetrySleep = 1
    healthRetryStep = 5
    nb_run = 3

    # Set initial context on the live server
    context = {'data': dict(),
       'response': dict(),
       'nb_run': nb_run,
       'timeoutSecond':timeoutSecond,
       'healthPeriod': healthPeriod,
       'healthRetry': 0,
       'healthRetrySleep': healthRetrySleep,
       'healthRetryStep': healthRetryStep,
       'exit': False
    }
    set_context(context)
    assert get_context() == context

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager.activate()
    assert drm_manager.get('health_period') == healthPeriod
    assert drm_manager.get('health_retry') == healthRetry
    assert drm_manager.get('health_retry_sleep') == healthRetrySleep
    try:
        wait_func_true(lambda: get_context()['exit'],
            timeout=2* nb_run * healthRetryStep, sleep_time=2)
    finally:
        drm_manager.deactivate()

    async_cb.assert_NoError()
    data = get_context()['data']
    assert len(data) == nb_run
    # Check retry timeout
    assert data.keys() == range(0, nb_run*healthRetryStep, healthRetryStep)
    for i, (retry_to, l) in enumerate(sorted(data.items(), key=lambda x: x[0])):
        assert retry_to == i*healthRetryStep
        if i == 0:
            assert len(l) == 1
        else:
            # Check the retry sleep is correct
            hid0, start0, prev_end = l.pop()
            assert hid0 == i
            for hid, start, end in l:
                assert hid == i
                delta = parser.parse(prev_end) - parser.parse(start)
                error_gap = healthRetrySleep
                assert healthRetrySleep - 1 <= int(delta.total_seconds()) <= healthRetrySleep + 1
                prev_end = end


@pytest.mark.skip
@pytest.mark.no_parallel
def test_health_retry_sleep(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health retry sleep value
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_retry_sleep'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Set initial context on the live server
    nb_health = 2
    healthPeriod = 3
    healthRetry = 4
    healthRetrySleep = 2
    context = {'data': list(),
               'nb_health':nb_health,
               'healthPeriod':healthPeriod,
               'healthRetry':healthRetry,
               'healthRetrySleep':healthRetrySleep,
               'exit':False
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    try:
        wait_func_true(lambda: get_context()['exit'],
                timeout=(healthPeriod+3) * (nb_health+2))
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    data_list = get_context()['data']
    data0 = data_list.pop(0)
    # Check the retry sleep period is correct
    check_cnt = 0
    for health_id, group in groupby(data_list, lambda x: x[0]):
        group = list(group)
        if len(group) < 2:
            continue
        assert len(group) >= 2
        start = group[0][2]
        end = group[1][1]
        delta = parser.parse(end) - parser.parse(start)
        assert int(delta.total_seconds()) == healthRetrySleep
        check_cnt += 1
    assert check_cnt > 0


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_retry_sleep_modification(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health retry sleep value when changed dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_health_retry_sleep_modification'
    conf_json.save()

    healthPeriod = 3
    healthRetry = 10
    healthRetrySleep = 1
    nb_run = 2

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

        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.activate()
        try:
            wait_func_true(lambda: get_context()['exit'],
                timeout=healthRetry*2)
        finally:
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


@pytest.mark.no_parallel
@pytest.mark.minimum
def test_health_metering_data(accelize_drm, conf_json, cred_json, async_handler, live_server, ws_admin):
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
    conf_json['licensing']['url'] = request.url + 'test_health_metering_data'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Set initial context on the live server
    healthPeriod = 1
    healthRetry = 0  # No retry
    context = {'data': list(),
               'health_id':0,
               'healthPeriod':healthPeriod,
               'healthRetry':healthRetry
    }
    set_context(context)
    assert get_context() == context

    def wait_and_check_on_next_health(drm):
        next_health_id = get_context()['health_id'] + 1
        wait_func_true(lambda: get_context()['health_id'] >= next_health_id,
                timeout=healthPeriod*3)
        session_id = drm.get('session_id')
        saas_data = ws_admin.get_last_metering_information(session_id)
        assert saas_data['session'] == drm.get('session_id')
        assert saas_data['metering'] == drm.get('metered_data')

    drm_manager.activate()
    try:
        # First round without no unit
        assert drm_manager.get('metered_data') == 0
        activators[0].check_coin(drm_manager.get('metered_data'))
        wait_and_check_on_next_health(drm_manager)
        # Second round with 10 units
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        wait_and_check_on_next_health(drm_manager)
        # Second round with 10 more units for a total of 20 units
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        wait_and_check_on_next_health(drm_manager)
        # Third round with 80 more units for a total of 100 units
        activators[0].generate_coin(30)
        activators[0].check_coin(drm_manager.get('metered_data'))
        wait_and_check_on_next_health(drm_manager)
        assert drm_manager.get('metered_data') == 50
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()


@pytest.mark.skip(reason='HDK is not yet ready to support synch and async metering request')
@pytest.mark.no_parallel
def test_segment_index(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the DRM Controller capacity to handle stressfully health and license requests
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = request.url + 'test_saturate_health_and_genlicense'
    logpath = realpath("./test_segment_index.%d.log" % randrange(0xFFFFFFFF))
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()

    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )

        # Set initial context on the live server
        nb_genlic = 4
        healthPeriod = 1
        healthRetry = 0  # no retry
        timeoutSecond = 6
        context = {'health_cnt':0,
                   'genlic_cnt':0,
                   'healthPeriod':healthPeriod,
                   'healthRetry':healthRetry,
                   'timeoutSecond':timeoutSecond
        }
        set_context(context)
        assert get_context() == context

        drm_manager.activate()
        try:
            wait_func_true(lambda: get_context()['nb_genlic'] >= nb_genlic,
                    timeout=(nb_genlic+1)*timeoutSecond)
        finally:
            drm_manager.deactivate()
            del drm_manager
        async_cb.assert_NoError()
        data_list = get_context()['data']
        assert len(data_list) > nb_genlic
        assert wait_func_true(lambda: isfile(logpath), 10)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        segment_idx_expected = 0
        for m in findall(r'"meteringFile"\s*:\s*"([^"]*)"', log_content):
            assert len(m) > 0
            session_id = m[0:16]
            close_flag = m[20]
            segment_idx = int(m[24:32],16)
            if session_id == "0000000000000000":
                assert close_flag == '0'
                assert segment_idx == 0
            assert segment_idx == segment_idx_expected
            segment_idx_expected += 1
            if close_flag == '1':
                segment_idx_expected = 0
    finally:
        if isfile(logpath):
            remove(logpath)



