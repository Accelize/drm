# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint
from itertools import groupby
from re import match
from flask import request as _request
from datetime import datetime, timedelta

from tests.conftest import wait_deadline, wait_func_true
from tests.proxy import get_context, set_context


@pytest.mark.no_parallel
@pytest.mark.lgdn
def test_topic0_corrupted_segment_index(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, request):
    """
    Test to reproduce the issue that corrupts the segment ID with
    both async and syn requests.
    !!! CAUTION: THIS TEST ENDS CORRECTLY ONLY WHEN AN ERROR IS HIT !!!
    !!!          IT SHOULD NOT BE PART OF A NORMAL REGRESSION !!!
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        healthPeriod = 10
        context = {'error':0,
                   'healthPeriod':healthPeriod
        }
        set_context(context)
        assert get_context() == context

        drm_manager.activate()
        wait_func_true(lambda: get_context()['error'], timeout=60)
        drm_manager.deactivate()
    async_cb.assert_NoError()


@pytest.mark.lgdn
def test_topic1_corrupted_metering(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test to reproduce the metering corruption issue on pause/resume operating mode
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    nb_run = 5
    nb_pause_resume_max = 100
    for r in range(nb_run):
        print('Run #%d' % r)
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert not drm_manager.get('session_status')
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            async_cb.assert_NoError()
            drm_manager.activate()
            start = datetime.now()
            assert sum(drm_manager.get('metered_data')) == 0
            assert drm_manager.get('session_status')
            assert drm_manager.get('license_status')
            assert len(drm_manager.get('session_id')) > 0
            lic_duration = drm_manager.get('license_duration')
            activators.autotest(is_activated=True)
            for i in range(nb_pause_resume_max):
                print('Pause #%d' % i)
                try:
                    activators.reset_coin()
                    activators.generate_coin()
                    data = drm_manager.get('metered_data')
                    try:
                        activators.check_coin(data)
                    except AssertionError:
                        print("ERROR detected!!!!!!!!")
                        print("1st read gives:", data)
                        print("Waiting 5s ...")
                        sleep(5)
                        print("... and double check the metering")
                        data = drm_manager.get('metered_data')
                        print("2nd read gives:", data)
                        activators.check_coin(drm_manager.get('metered_data'))
                    drm_manager.deactivate()
                    activators.autotest(is_activated=False)
                    async_cb.assert_NoError()
                    assert not drm_manager.get('session_status')
                    assert not drm_manager.get('license_status')
                    # Wait for the limit of the expiration
                    random_wait = lic_duration*2
                    wait_deadline(start, random_wait)
                    drm_manager.activate()
                    start = datetime.now()
                except:
                    raise
            drm_manager.deactivate()
            assert not drm_manager.get('session_status')
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert drm_manager.get('session_id') != session_id
            async_cb.assert_NoError()


@pytest.mark.lgdn
def test_topic1_corrupted_metering2(accelize_drm, conf_json, cred_json,
                async_handler, live_server, request):
    """
    Test to reproduce the metering corruption issue on pause/resume operating mode
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    # Set initial context on the live server
    healthPeriod = 0
    context = {'healthPeriod':healthPeriod}
    set_context(context)
    assert get_context() == context

    async_cb.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json.save()
    nb_run = 5
    nb_pause_resume_max = 100
    for r in range(nb_run):
        print('Run #%d' % r)
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            activators.reset_coin()
            assert not drm_manager.get('session_status')
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            async_cb.assert_NoError()
            drm_manager.activate()
            start = datetime.now()
            assert sum(drm_manager.get('metered_data')) == 0
            assert drm_manager.get('session_status')
            assert drm_manager.get('license_status')
            session_id = drm_manager.get('session_id')
            assert len(session_id) > 0
            lic_duration = drm_manager.get('license_duration')
            activators.autotest(is_activated=True)
            for i in range(nb_pause_resume_max):
                print('Pause #%d' % i)
                try:
                    activators.generate_coin()
                    data = drm_manager.get('metered_data')
                    try:
                        activators.check_coin(data)
                    except AssertionError:
                        print("ERROR detected!!!!!!!!")
                        print("1st read gives:", data)
                        print("Waiting 5s ...")
                        sleep(5)
                        print("... and double check the metering")
                        data = drm_manager.get('metered_data')
                        print("2nd read gives:", data)
                        activators.check_coin(drm_manager.get('metered_data'))
                    drm_manager.deactivate()
                    async_cb.assert_NoError()
                    assert not drm_manager.get('session_status')
                    assert not drm_manager.get('license_status')
                    assert drm_manager.get('session_id') == session_id
                    # Wait for the limit of the expiration
                    random_wait = lic_duration
                    wait_deadline(start, random_wait)
                    drm_manager.activate()
                    start = datetime.now()
                except:
                    raise
            drm_manager.deactivate()
            assert not drm_manager.get('session_status')
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert drm_manager.get('session_id') != session_id
            async_cb.assert_NoError()


@pytest.mark.lgdn
def test_endurance(accelize_drm, conf_json, cred_json, async_handler):
    """Test the continuity of service for a long period"""
    from random import randint
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_05')

    # Get test duration
    try:
        test_duration = accelize_drm.pytest_params['duration']
    except:
        test_duration = 24*3600  # 1 day
        print('Warning: Missing argument "duration". Using default value %d' % test_duration)

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        activators.check_coin(drm_manager.get('metered_data'))
        while True:
            assert drm_manager.get('license_status')
            activators.generate_coin()
            activators.check_coin(drm_manager.get('metered_data'))
            trng = drm_manager.get('trng_status')
            seconds_left = test_duration - (datetime.now() - start).total_seconds()
            print('Remaining time: %0.1fs  /  current coins=%d / security_alert_bit=%d adaptive_proportion_test_error=0x%s repetition_count_test_error=0x%s'
                    % (seconds_left, activators[0].metering_data, trng['security_alert_bit'], trng['adaptive_proportion_test_error'], trng['repetition_count_test_error'] ))
            if seconds_left < 0:
                break
            sleep(randint(10, 3*lic_duration))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
    activators.autotest(is_activated=False)
    elapsed = datetime.now() - start
    print('Endurance test has completed:', str(timedelta(seconds=elapsed.total_seconds())))


@pytest.mark.lgdn
def test_drm_controller_activation_timeout(accelize_drm, conf_json, cred_json, async_handler):
    """Reproduce DRM Controller Activation is in timeout issue"""
    driver = accelize_drm.pytest_fpga_driver[0]
    image_id = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    drm_manager = None

    try:
        nb_loop = accelize_drm.pytest_params['loop']
        print('Using parameter "loop"=%d' % nb_loop)
    except:
        nb_loop = 20
        print('Warning: Missing argument "loop". Using default value %d' % nb_loop)

    err = 0
    for i in range(nb_loop):
        print('\n*** Loop #%d ***' % i)
        # Program FPGA with lastest HDK per major number
        driver.program_fpga(image_id)
        try:
            # Test no compatibility issue
            with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
                assert not drm_manager.get('license_status')
                drm_manager.activate()
                assert drm_manager.get('license_status')
                drm_manager.deactivate()
                assert not drm_manager.get('license_status')
            async_cb.assert_NoError()
            if err > 0:
                print('Reattempt after error succeeded!')
                break
        except AssertionError:
            err += 1
        if err > 1:
            print('Reattempt after error failed!')


@pytest.mark.lgdn
def test_run_for_a_period_of_time(accelize_drm, conf_json, cred_json, async_handler):
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    period = 60
    try:
        period = accelize_drm.pytest_params['period']
    except:
        pass
    print('Using parameter "period"=%d' % period)

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        try:
            wait_func_true(lambda: not drm_manager.get('license_status'), timeout=period, sleep_time=1)
        except RuntimeError:
            pass
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
    async_cb.assert_NoError()
