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
from tests.proxy import get_context, set_context


def test_health_period_disabled(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Test the asynchronous health feature can be disabled.
    """
    from os import remove
    from os.path import realpath, isfile
    from tests.conftest import wait_func_true

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
    nb_health = 3
    healthPeriod = 2
    context = {'cnt':0, 'healthPeriod':healthPeriod, 'nb_health':nb_health}
    set_context(context)
    assert get_context() == context

    try:
        drm_manager.activate()
        sleep(3)
        healthPeriod = drm_manager.get('health_period')
        sleep(healthPeriod*(nb_health+2))
        drm_manager.deactivate()
        del drm_manager
        async_cb.assert_NoError()
        context = get_context()
        assert context['cnt'] == nb_health
        assert wait_func_true(lambda: isfile(logpath), 10)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        assert search(r'Health thread has been disabled', log_content, MULTILINE)
    finally:
        if isfile(logpath):
            remove(logpath)


@pytest.mark.skip(reason='LGDN bug in async feature')
@pytest.mark.minimum
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
        nb_health = 3
        while True:
            sleep(healthPeriod)
            context = get_context()
            if len(context['data']) >= (nb_health + 1):
                break
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    context = get_context()
    data_list = context['data']
    assert len(data_list) >= nb_health+1
    wait_start = data_list.pop(0)[1]
    for start, end in data_list:
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod
        wait_start = end


@pytest.mark.skip(reason='LGDN bug in async feature')
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
        nb_health = 5
        while True:
            sleep(healthPeriod)
            context = get_context()
            if len(context['data']) >= (nb_health*2 + 1):
                break
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    context = get_context()
    data_list = context['data']
    assert len(data_list) >= nb_health+1
    wait_start = data_list.pop(0)[1]
    for i, (start, end) in enumerate(data_list):
        delta = parser.parse(start) - parser.parse(wait_start)
        assert int(delta.total_seconds()) == healthPeriod + i
        wait_start = end


@pytest.mark.skip(reason='LGDN bug in async feature')
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
        nb_health = 3
        while True:
            sleep(healthPeriod)
            context = get_context()
            if len(context['data']) >= nb_health + 1:
                break
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    context = get_context()
    data_list = context['data']
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


@pytest.mark.skip(reason='LGDN bug in async feature')
@pytest.mark.minimum
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
    healthPeriod = 3
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
        sleep(healthRetry)
        while not context['exit']:
            context = get_context()
            sleep(1)
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    context = get_context()
    data_list = context['data']
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


@pytest.mark.skip(reason='LGDN bug in async feature')
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

        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.activate()
        try:
            sleep(retry_timeout)
            while not context['exit']:
                context = get_context()
                sleep(1)
        finally:
            drm_manager.deactivate()

        async_cb.assert_NoError()
        context = get_context()
        data_list = context['data']
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
        assert retry_timeout - error_gap <= int(delta.total_seconds()) <= retry_timeout + error_gap


@pytest.mark.skip(reason='LGDN bug in async feature')
@pytest.mark.minimum
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
    healthPeriod = 3
    healthRetry = 4
    healthRetrySleep = 2
    nb_run = 3
    context = {'data': list(),
               'nb_run':nb_run,
               'healthPeriod':healthPeriod,
               'healthRetry':healthRetry,
               'healthRetrySleep':healthRetrySleep,
               'exit':False
    }
    set_context(context)
    assert get_context() == context

    drm_manager.activate()
    try:
        sleep(healthRetry)
        while not context['exit']:
            context = get_context()
            sleep(1)
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()
    context = get_context()
    data_list = context['data']
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


@pytest.mark.skip(reason='LGDN bug in async feature')
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

        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.activate()
        try:
            sleep(healthRetry)
            while not context['exit']:
                context = get_context()
                sleep(1)
        finally:
            drm_manager.deactivate()

        async_cb.assert_NoError()
        context = get_context()
        data_list = context['data']
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


@pytest.mark.skip(reason='LGDN bug in async feature')
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

    def wait_and_check_on_next_health(drm_manager):
        next_health_id = get_context()['health_id'] + 2
        while True:
            if get_context()['health_id'] >= next_health_id:
                break
            sleep(1)
        session_id = drm_manager.get('session_id')
        saas_data = ws_admin.get_last_metering_information(session_id)
        assert saas_data['session'] == session_id
        assert saas_data['metering'] == drm_manager.get('metered_data')

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
