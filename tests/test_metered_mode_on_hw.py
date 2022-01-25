# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint, choice
from datetime import datetime, timedelta
from re import search, findall
from os.path import realpath, isfile

from tests.conftest import wait_deadline, wait_func_true


def test_fast_start_stop(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test no error occurs witha quick start/stop
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
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
        activators.autotest(is_activated=False)
        drm_manager.activate()
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert sum(drm_manager.get('metered_data')) == 0
        async_cb.assert_NoError()
        drm_manager.deactivate()
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_metered_start_stop_short_time(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test no error occurs in normal start/stop metering mode during a short period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
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
        activators.autotest(is_activated=False)
        activators.generate_coin(1000)
        activators.check_coin()
        drm_manager.activate()
        activators.autotest(is_activated=True)
        activators.generate_coin(1)
        activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators.check_coin()
        async_cb.assert_NoError()
        drm_manager.deactivate()
    logfile.remove()


def test_metered_start_stop_short_time_in_debug(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test no error occurs in normal start/stop metering mode during a short period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
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
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate()
        assert sum(drm_manager.get('metered_data')) == 0
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert sum(drm_manager.get('metered_data')) == 0
        async_cb.assert_NoError()
        drm_manager.deactivate()
    logfile.remove()


@pytest.mark.long_run
@pytest.mark.hwtst
def test_metered_start_stop_long_time(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test no error occurs in normal start/stop metering mode during a long period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
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
        activators.autotest(is_activated=False)
        drm_manager.activate()
        start = datetime.now()
        license_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        assert sum(drm_manager.get('metered_data')) == 0
        activators.autotest(is_activated=True)
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        for i in range(3):
            wait_period = randint(license_duration-2, license_duration+2)
            sleep(wait_period)
            start += timedelta(seconds=license_duration)
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            activators.generate_coin()
            activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        drm_manager.deactivate()
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_metered_pause_resume_long_time(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test no error occurs in normal start/stop metering mode during a long period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')
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
        nb_pause_resume = 2
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
        wait_numbers = [lic_duration*2-2,lic_duration*2+2]
        activators.autotest(is_activated=True)
        for i in range(nb_pause_resume):
            activators.generate_coin()
            activators.check_coin(drm_manager.get('metered_data'))
            wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, 10)
            drm_manager.deactivate(True)
            async_cb.assert_NoError()
            assert drm_manager.get('session_status')
            assert drm_manager.get('license_status')
            assert drm_manager.get('session_id') == session_id
            # Wait randomly at the limit of the expiration
            random_wait = choice(wait_numbers)
            wait_deadline(start, random_wait)
            drm_manager.activate(True)
            if random_wait > lic_duration*2:
                start = datetime.now()
                assert drm_manager.get('session_id') != session_id
                activators.reset_coin()
                session_id = drm_manager.get('session_id')
            else:
                start += timedelta(seconds=lic_duration)
                assert drm_manager.get('session_id') == session_id, 'after loop #%d' % i
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('session_id') != session_id
        async_cb.assert_NoError()
        drm_manager.deactivate()
    logfile.remove()


@pytest.mark.hwtst
def test_metered_pause_resume_from_new_object(accelize_drm, conf_json, conf_json_second,
                    cred_json, async_handler, log_file_factory):
    """
    Test no error occurs in normal pause/resume metering mode when the resume
    is executed from a new object and before the license expires
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')
    logfile1 = log_file_factory.create(2)
    conf_json['settings'].update(logfile1.json)
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager1:
        assert not drm_manager1.get('session_status')
        assert not drm_manager1.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager1.activate()
        start = datetime.now()
        assert sum(drm_manager1.get('metered_data')) == 0
        assert drm_manager1.get('session_status')
        assert drm_manager1.get('license_status')
        session_id = drm_manager1.get('session_id')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        lic_duration = drm_manager1.get('license_duration')
        assert sum(drm_manager1.get('metered_data')) == 0
        activators.generate_coin()
        activators.check_coin(drm_manager1.get('metered_data'))
        assert sum(drm_manager1.get('metered_data')) != 0
        # Wait enough time to be sure the 2nd license has been provisioned
        wait_deadline(start, lic_duration/2)
        drm_manager1.deactivate(True)
        assert drm_manager1.get('session_status')
        assert drm_manager1.get('license_status')
        assert drm_manager1.get('session_id') == session_id
        activators.autotest(is_activated=True)
        async_cb.assert_NoError()
        sleep(1)

        # Create new object
        logfile2 = log_file_factory.create(2)
        conf_json_second['settings'].update(logfile2.json)
        conf_json_second.save()

        drm_manager2 = accelize_drm.DrmManager(
                    conf_json_second.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
        assert drm_manager1 != drm_manager2
        assert drm_manager2.get('session_status')
        assert drm_manager2.get('license_status')
        activators.autotest(is_activated=True)
        assert drm_manager2.get('session_id') == ''
        # Resume session
        drm_manager2.activate(True)
        assert drm_manager2.get('session_status')
        assert drm_manager2.get('license_status')
        activators.autotest(is_activated=True)
        activators.check_coin(drm_manager2.get('metered_data'))
        # Wait for license renewal
        sleep(lic_duration+2)
        assert drm_manager2.get('session_id') == session_id
        assert drm_manager2.get('license_duration') == lic_duration
        activators.generate_coin()
        activators.check_coin(drm_manager2.get('metered_data'))
        drm_manager2.deactivate()
        assert not drm_manager2.get('session_status')
        assert not drm_manager2.get('license_status')
        assert drm_manager2.get('session_id') == ''
        activators.autotest(is_activated=False)
        logfile2.remove()
    logfile1.remove()
    async_cb.assert_NoError()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_async_on_pause(accelize_drm, conf_json, cred_json, async_handler, log_file_factory, request):
    """
    Test an async health commande is executed on pause.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    logfile = log_file_factory.create(0)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

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
        drm_manager.activate()
        start = datetime.now()
        if drm_manager.get('health_period') == 0:
            logfile.remove()
            pytest.skip('Health is not active: skip async test')
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        drm_manager.deactivate(True) # Pause session
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        drm_manager.deactivate()
    log_content = logfile.read()
    assert len(list(findall(r'"request"\s*:\s*"health"', log_content))) == 1
    async_cb.assert_NoError()
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_stop_after_pause(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test an async health commande is executed on pause.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
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
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        drm_manager.deactivate(True) # Pause session
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        assert drm_manager.get('session_id') != session_id
        assert drm_manager.get('session_id') == ''
        activators.autotest(is_activated=False)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_metering_limits_on_activate(accelize_drm, conf_json, cred_json, async_handler, log_file_factory, ws_admin):
    """
    Test an error is returned by the activate function and the design is locked when the limit is reached.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    # Test activate function call fails when limit is reached
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    cred_json.set_user('accelize_accelerator_test_03')
    accelize_drm.clean_metering_env(cred_json, ws_admin)
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_status')
        assert sum(drm_manager.get('metered_data')) == 0
        activators[0].generate_coin(999)
        activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        activators.reset_coin()
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        activators.check_coin(drm_manager.get('metered_data'))
        activators[0].generate_coin(1)
        activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Metering Web Service error 400' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_03@accelize.com', str(excinfo.value))
        assert 'You have reached the maximum quantity of 1000. usage_unit for metered entitlement (licensed)' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'You have reached the maximum quantity of 1000. usage_unit for metered entitlement')
        async_cb.reset()
        drm_manager.deactivate()
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_metering_limits_on_licensing_thread(accelize_drm, conf_json, cred_json, async_handler, log_file_factory, ws_admin):
    """
    Test an error is returned by the async error function and the design is locked when the limit is reached.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    cred_json.set_user('accelize_accelerator_test_03')
    accelize_drm.clean_metering_env(cred_json, ws_admin)
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_status')
        assert sum(drm_manager.get('metered_data')) == 0
        lic_duration = drm_manager.get('license_duration')
        sleep(int(lic_duration/2) + 1)
        activators[0].generate_coin(1000)
        activators.check_coin(drm_manager.get('metered_data'))
        # Wait right before lock
        wait_deadline(start, 3*lic_duration-3)
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        wait_deadline(start, 3*lic_duration+3)
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        # Verify asynchronous callback has been called
        assert async_cb.was_called
        assert 'Metering Web Service error 400' in async_cb.message
        assert 'DRM WS request failed' in async_cb.message
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_03@accelize.com', async_cb.message)
        assert 'You have reached the maximum quantity of 1000. usage_unit for metered entitlement (licensed)' in async_cb.message
        assert async_cb.errcode == accelize_drm.exceptions.DRMWSReqError.error_code
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
    logfile.remove()


@pytest.mark.on_2_fpga
@pytest.mark.minimum
@pytest.mark.hwtst
def test_floating_limits(accelize_drm, conf_json, conf_json_second, cred_json, async_handler, log_file_factory):
    """
    Test an error is returned when the floating limit is reached
    """
    if len(accelize_drm.pytest_fpga_driver) < 2:
        pytest.skip('Skip test because 2 FPGA are needed but only 1 is found')
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb0.reset()
    async_cb1 = async_handler.create()
    async_cb1.reset()
    cred_json.set_user('accelize_accelerator_test_04')

    # Get floating license for drm_manager0 and check drm_manager1 fails
    conf_json.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver0.read_register_callback,
            driver0.write_register_callback,
            async_cb0.callback
        ) as drm_manager0:
        conf_json_second['settings']['ws_api_retry_duration'] = 4
        conf_json_second.save()
        drm_manager1 = accelize_drm.DrmManager(
            conf_json_second.path,
            cred_json.path,
            driver1.read_register_callback,
            driver1.write_register_callback,
            async_cb1.callback
        )
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
            drm_manager1.activate()
        assert search(r'Timeout on License request after .+ attempts', str(excinfo.value))
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSTimedOut.error_code
        async_cb1.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, 'Timeout on License request after')
        async_cb1.reset()
    async_cb0.assert_NoError()

    # Get floating license for drm_manager1 and check drm_manager0 fails
    conf_json_second.reset()
    with accelize_drm.DrmManager(
            conf_json_second.path,
            cred_json.path,
            driver1.read_register_callback,
            driver1.write_register_callback,
            async_cb1.callback
        ) as drm_manager1:
        conf_json['settings']['ws_api_retry_duration'] = 4
        conf_json.save()
        drm_manager0 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver0.read_register_callback,
            driver0.write_register_callback,
            async_cb0.callback
        )
        drm_manager1.activate()
        assert drm_manager1.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
            drm_manager0.activate()
        assert search(r'Timeout on License request after .+ attempts', str(excinfo.value)) is not None
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSTimedOut.error_code
        async_cb0.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, 'Timeout on License request after')
        async_cb0.reset()
    async_cb1.assert_NoError()


def test_async_call_during_pause(accelize_drm, conf_json, cred_json, async_handler, log_file_factory, request):
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
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
        activators.reset_coin()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators.generate_coin()
        assert sum(drm_manager.get('metered_data')) == 0
        drm_manager.activate()
        assert sum(drm_manager.get('metered_data')) == 0
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate(True)
        sleep(1)
        activators.check_coin(drm_manager.get('metered_data'))
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        lic_duration = drm_manager.get('license_duration')
        activators.autotest(is_activated=True)
        sleep(lic_duration * 2 + 1)
        activators.autotest(is_activated=False)
        assert not drm_manager.get('license_status')
        assert drm_manager.get('session_status')
        assert session_id == drm_manager.get('session_id')
        activators.check_coin(drm_manager.get('metered_data'))
        activators.generate_coin()
        activators.check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert sum(drm_manager.get('metered_data')) == 0
    log_content = logfile.read()
    assert len(list(findall(r'warning\b.*\bCannot access metering data when no session is running', log_content))) == 2
    async_cb.assert_NoError()
    logfile.remove()

