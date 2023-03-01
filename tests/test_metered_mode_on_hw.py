# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint, choice
from datetime import datetime, timedelta
from re import search, findall, IGNORECASE
from os.path import realpath, isfile

from tests.conftest import wait_deadline, wait_until_true


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
    cred_json.set_user('test-metering')
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
    cred_json.set_user('test-metering')

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
def test_only_1_instance_is_allowed(accelize_drm, conf_json, conf_json_second,
                    cred_json, async_handler, log_file_factory):
    """
    Test a 2nd DRM_Manager object cannot be created if 1 already exists
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile1 = log_file_factory.create(1)
    conf_json['settings'].update(logfile1.json)
    conf_json.save()

    # Create 1st object
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        async_cb.assert_NoError()
        # Create a 2nd object
        logfile2 = log_file_factory.create(1)
        conf_json_second['settings'].update(logfile2.json)
        conf_json_second.save()
        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager2 = accelize_drm.DrmManager(
                    conf_json_second.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
        assert search(r'Another instance is currently owning the DRM Controller', str(excinfo.value), IGNORECASE)
        assert search(r'You might have another process running the DRM Controller.', str(excinfo.value), IGNORECASE)
        assert search(r'If not, a reset of the DRM Controller is required to recover.', str(excinfo.value), IGNORECASE)
        assert accelize_drm.exceptions.DRMBadUsage.error_code in async_handler.get_error_code(str(excinfo.value))
        async_cb.reset()
        drm_manager.get('device_id')
    log_content1 = logfile1.read()
    m = search(r'DRM Controller is locked by this instance with ID (\S+)', log_content1, IGNORECASE)
    assert int(m.group(1)) != 0
    assert search(r'DRM Controller is unlocked by this instance with ID %s' % m.group(1), log_content1, IGNORECASE)
    logfile1.remove()
    log_content2 = logfile2.read()
    assert search(r'Another instance is currently owning the DRM Controller', log_content2, IGNORECASE)
    assert search(r'You might have another process running the DRM Controller.', log_content2, IGNORECASE)
    assert search(r'If not, a reset of the DRM Controller is required to recover.', log_content2, IGNORECASE)
    logfile2.remove()


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
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        # Temporarily cut communication between Controller and activator 1
        drm_manager.set(log_message_level=2)
        drm_manager.set(log_message='Disconnecting DRM chain')
        assert print_status(True)
        act.write_register(0x38, 1)
        wait_until_true(lambda: print_status(False), license_duration)
        drm_manager.set(log_message='Lock detected on activator status')
        activators.autotest(is_activated=False)
        assert not drm_manager.get('license_status')
        drm_manager.set(log_message='Reconnecting DRM chain')
        act.write_register(0x38, 0)
        wait_until_true(lambda: print_status(True), license_duration)
        drm_manager.set(log_message='Unlock detected on activator status')
        activators.autotest(is_activated=True)
        assert drm_manager.get('license_status')
        async_cb.assert_Error(accelize_drm.exceptions.DRMCtlrError.error_code, 'Status Bit: 0b0')
        async_cb.reset()
    logfile.remove()


@pytest.mark.hwtst
@pytest.mark.minimum
def test_session_autoclose(accelize_drm, conf_json, cred_json, async_handler,
                            ws_admin, log_file_factory):
    """Test a session not propoerly closed is close gracefully"""
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-metering')        # User with a single nodelock license
    logfile = log_file_factory.create(1, append=True)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        assert not drm_manager.get('session_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        # Reopen
        drm_manager.activate()
        session_id2 = drm_manager.get('session_id')
        assert len(session_id2) > 0
        assert session_id2 != session_id
    async_cb.assert_NoError()
    log_content = logfile.read()
    assert search(r'The floating/metering session is still pending: trying to close it gracefully.', log_content, IGNORECASE)
    logfile.remove()
