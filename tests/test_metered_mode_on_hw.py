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
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_metered_from_new_objects(accelize_drm, conf_json, conf_json_second,
                    cred_json, async_handler, log_file_factory):
    """
    Test no error occurs in normal metering mode when 2 objects are created
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
        license_duration = drm_manager1.get('license_duration')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        assert sum(drm_manager1.get('metered_data')) == 0
        activators.generate_coin()
        activators.check_coin(drm_manager1.get('metered_data'))
        assert sum(drm_manager1.get('metered_data')) != 0
        wait_func_true(lambda: drm_manager1.get('num_license_loaded') == 2, license_duration)
        drm_manager1.deactivate()
        assert not drm_manager1.get('session_status')
        assert not drm_manager1.get('license_status')
        assert drm_manager1.get('session_id') != session_id
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        sleep(1)

        # Create new object
        activators.reset_coin()
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
        assert not drm_manager2.get('session_status')
        assert not drm_manager2.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager2.get('session_id') == ''
        # Start session
        drm_manager2.activate()
        assert drm_manager2.get('session_status')
        assert drm_manager2.get('license_status')
        activators.autotest(is_activated=True)
        activators.check_coin(drm_manager2.get('metered_data'))
        # Wait for license renewal
        lic_duration = drm_manager2.get('license_duration')
        sleep(lic_duration+2)
        assert drm_manager2.get('session_id') != session_id
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
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_metering_limits_on_activate(accelize_drm, conf_json, cred_json, async_handler, log_file_factory,
                                    ws_admin):
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
    ws_admin.load(conf_json, cred_json)
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
        assert search(r'"Entitlement Limit Reached.* with PT DRM Ref Design .+ for \S+_test_03@accelize.com', str(excinfo.value))
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
    ws_admin.load(conf_json, cred_json)
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
        assert search(r'"Entitlement Limit Reached.* with PT DRM Ref Design .+ for \S+_test_03@accelize.com', async_cb.message)
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


@pytest.mark.skip
def test_heart_beat(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test activator locks if heart beat stops
    """
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    act = activators[0]

    if act.read_register(0x38) & 0xFFFFFF00 == 0:
        pytest.skip('Activator is not supporting the AXI4-stream communication cut')

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    def print_status(expect):
        return act.get_status() == expect

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
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        license_duration = drm_manager.get('license_duration')
        wait_func_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        # Temporarily cut communication between Controller and activator 1
        drm_manager.set(log_message_level=2)
        drm_manager.set(log_message='Disconnecting DRM chain')
        assert print_status(True)
        act.write_register(0x38, 1)
        wait_func_true(lambda: print_status(False), license_duration)
        drm_manager.set(log_message='Lock detected on activator status')
        activators.autotest(is_activated=False)
        assert not drm_manager.get('license_status')
        drm_manager.set(log_message='Reconnecting DRM chain')
        act.write_register(0x38, 0)
        wait_func_true(lambda: print_status(True), license_duration)
        drm_manager.set(log_message='Unlock detected on activator status')
        activators.autotest(is_activated=True)
        assert drm_manager.get('license_status')
        async_cb.assert_Error(accelize_drm.exceptions.DRMCtlrError.error_code, 'Status Bit: 0b0')
        async_cb.reset()
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_session_autoclose(accelize_drm, conf_json, conf_json_second,
                    cred_json, async_handler, log_file_factory):
    """
    Test any pending session is close before starting a new one
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
        license_duration = drm_manager1.get('license_duration')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        assert sum(drm_manager1.get('metered_data')) == 0
        activators.generate_coin()
        activators.check_coin(drm_manager1.get('metered_data'))
        assert sum(drm_manager1.get('metered_data')) != 0
        wait_func_true(lambda: drm_manager1.get('num_license_loaded') == 2, license_duration)
        async_cb.assert_NoError()
        sleep(1)

        # Create new object
        logfile2 = log_file_factory.create(1)
        conf_json_second['settings'].update(logfile2.json)
        conf_json_second.save()
        with accelize_drm.DrmManager(
                    conf_json_second.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager2:
            assert drm_manager2.get('session_status')
            assert drm_manager2.get('license_status')
            activators.autotest(is_activated=True)
            drm_manager2.activate()
            assert drm_manager2.get('session_id') != session_id
            activators.autotest(is_activated=True)
            drm_manager2.deactivate()
        activators.autotest(is_activated=False)
        log_content2 = logfile2.read()
        assert 'The floating/metering session is still pending: trying to close it gracefully' in log_content2
        assert 'Build ending license request #0 to stop current session' in log_content2
        logfile2.remove()
    logfile1.remove()
    async_cb.assert_NoError()
