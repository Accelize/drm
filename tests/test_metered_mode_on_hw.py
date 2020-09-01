# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint, randrange
from datetime import datetime, timedelta
from re import search, findall
from os.path import realpath, isfile
from os import remove

from tests.conftest import wait_deadline, wait_func_true, whoami


def test_metered_start_stop_in_raw(accelize_drm, conf_json, cred_json, async_handler):
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators[0].generate_coin(1000)
        drm_manager.activate()
        assert drm_manager.get('metered_data') == 0
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


def test_metered_start_stop_1_coin_in_raw(accelize_drm, conf_json, cred_json, async_handler):
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators[0].generate_coin(1000)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.activate()
        activators[0].generate_coin(1)
        assert drm_manager.get('metered_data') == 1
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('metered_data') == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_metered_start_stop_short_time(accelize_drm, conf_json, cred_json, async_handler):
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators[0].generate_coin(1000)
        drm_manager.activate()
        activators[0].check_coin(drm_manager.get('metered_data'))
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('metered_data') == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


def test_metered_start_stop_short_time_in_debug(accelize_drm, conf_json, cred_json, async_handler):
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
    conf_json['settings']['log_verbosity'] = 1
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate()
        assert drm_manager.get('metered_data') == 0
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('metered_data') == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.long_run
@pytest.mark.hwtst
def test_metered_start_stop_long_time(accelize_drm, conf_json, cred_json, async_handler):
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate()
        start = datetime.now()
        license_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        assert drm_manager.get('metered_data') == 0
        activators.autotest(is_activated=True)
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        for i in range(3):
            wait_period = randint(license_duration-2, license_duration+2)
            sleep(wait_period)
            start += timedelta(seconds=license_duration)
            new_coins = randint(1,10)
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            activators[0].generate_coin(new_coins)
            activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_metered_pause_resume_short_time(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal pause/resume metering mode during a short period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('metered_data') == 0
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        activators.autotest(is_activated=True)
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('metered_data') == 0
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Wait enough time to be sure the 2nd license has been provisioned
        wait_deadline(start, lic_duration/2)
        drm_manager.deactivate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        # Wait right before license expiration
        wait_deadline(start, 2*lic_duration-2)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        activators.autotest(is_activated=True)
        # Wait expiration
        sleep(4)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.activate(True)
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
    finally:
        drm_manager.deactivate()


@pytest.mark.hwtst
def test_metered_pause_resume_long_time(accelize_drm, conf_json, cred_json, async_handler):
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    nb_pause_resume = 5
    try:
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('metered_data') == 0
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        lic_duration = drm_manager.get('license_duration')
        activators.autotest(is_activated=True)
        for i in range(nb_pause_resume):
            new_coins = randint(1, 100)
            activators[0].generate_coin(new_coins)
            activators[0].check_coin(drm_manager.get('metered_data'))
            drm_manager.deactivate(True)
            async_cb.assert_NoError()
            assert drm_manager.get('session_status')
            assert drm_manager.get('license_status')
            assert drm_manager.get('session_id') == session_id
            # Wait randomly at the limit of the expiration
            random_wait = randint(lic_duration*2-1, lic_duration*2+1)
            wait_deadline(start, random_wait)
            drm_manager.activate(True)
            start = datetime.now()
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
    finally:
        drm_manager.deactivate()


@pytest.mark.hwtst
def test_metered_pause_resume_from_new_object(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal pause/resume metering mode when the resume is executed from a new object and before the license expires
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager1.get('session_status')
    assert not drm_manager1.get('license_status')
    activators.autotest(is_activated=False)
    drm_manager1.activate()
    start = datetime.now()
    assert drm_manager1.get('metered_data') == 0
    assert drm_manager1.get('session_status')
    assert drm_manager1.get('license_status')
    session_id = drm_manager1.get('session_id')
    assert len(session_id) > 0
    activators.autotest(is_activated=True)
    lic_duration = drm_manager1.get('license_duration')
    assert drm_manager1.get('metered_data') == 0
    activators[0].generate_coin(10)
    activators[0].check_coin(drm_manager1.get('metered_data'))
    assert drm_manager1.get('metered_data') == 10
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
    drm_manager2 = accelize_drm.DrmManager(
        conf_json.path,
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
    assert drm_manager2.get('metered_data') == 10
    # Wait for license renewal
    sleep(lic_duration+2)
    assert drm_manager2.get('session_id') == session_id
    assert drm_manager2.get('license_duration') == lic_duration
    activators[0].generate_coin(10)
    activators[0].check_coin(drm_manager2.get('metered_data'))
    assert drm_manager2.get('metered_data') == 20
    drm_manager2.deactivate()
    assert not drm_manager2.get('session_status')
    assert not drm_manager2.get('license_status')
    assert drm_manager2.get('session_id') == ''
    activators.autotest(is_activated=False)
    async_cb.assert_NoError()


@pytest.mark.minimum
@pytest.mark.hwtst
def test_async_on_pause(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test an async health commande is executed on pause.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    logpath = accelize_drm.create_log_path(whoami())
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
    try:
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
    finally:
        drm_manager.deactivate()
    del drm_manager
    wait_func_true(lambda: isfile(logpath), 10)
    with open(logpath, 'rt') as f:
        log_content = f.read()
    assert len(list(findall(r'"request"\s*:\s*"health"', log_content))) == 1
    async_cb.assert_NoError()
    remove(logpath)


@pytest.mark.minimum
@pytest.mark.hwtst
def test_stop_after_pause(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test an async health commande is executed on pause.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager.get('session_status')
    assert not drm_manager.get('license_status')
    activators.autotest(is_activated=False)
    drm_manager.activate()
    try:
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
    finally:
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        assert drm_manager.get('session_id') != session_id
        assert drm_manager.get('session_id') == ''
        activators.autotest(is_activated=False)


@pytest.mark.minimum
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_metering_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """
    Test an error is returned and the design is locked when the limit is reached.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_03')

    # Test activate function call fails when limit is reached
    async_cb.reset()
    conf_json.reset()
    accelize_drm.clean_metering_env(cred_json, ws_admin)
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_status')
        assert drm_manager.get('metered_data') == 0
        activators[0].generate_coin(999)
        activators[0].check_coin(drm_manager.get('metered_data'))
        sleep(1)
        drm_manager.deactivate()
        activators[0].reset_coin()
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        activators[0].check_coin(drm_manager.get('metered_data'))
        activators[0].generate_coin(1)
        activators[0].check_coin(drm_manager.get('metered_data'))
        sleep(1)
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Metering Web Service error 400' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_03@accelize.com', str(excinfo.value))
        assert 'You have reached the maximum quantity of 1000. usage_unit for metered entitlement (licensed)' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()
    print('Test activate function fails when limit is reached: PASS')

    # Test background thread stops when limit is reached
    async_cb.reset()
    conf_json.reset()
    accelize_drm.clean_metering_env(cred_json, ws_admin)
    activators.reset_coin()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_status')
        assert drm_manager.get('metered_data') == 0
        lic_duration = drm_manager.get('license_duration')
        sleep(2)
        activators[0].generate_coin(1000)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Wait right before expiration
        wait_deadline(start, 3*lic_duration-3)
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        sleep(5)
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
    finally:
        drm_manager.deactivate()
    print('Test background thread stops when limit is reached: PASS')


@pytest.mark.on_2_fpga
@pytest.mark.minimum
@pytest.mark.hwtst
def test_floating_limits(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test an error is returned when the floating limit is reached
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_04')
    conf_json.reset()

    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager1.activate()
        assert search(r'Timeout on License request after .+ attempts', str(excinfo.value)) is not None
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        async_cb1.assert_NoError()
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        async_cb0.assert_NoError()
    try:
        drm_manager1.activate()
        assert drm_manager1.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager0.activate()
        assert search(r'Timeout on License request after .+ attempts', str(excinfo.value)) is not None
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        async_cb0.assert_NoError()
    finally:
        drm_manager1.deactivate()
        assert not drm_manager1.get('license_status')
        async_cb1.assert_NoError()


def test_async_call_during_pause(accelize_drm, conf_json, cred_json, async_handler):
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    logpath = accelize_drm.create_log_path(whoami())
    conf_json['settings']['log_file_verbosity'] = accelize_drm.create_log_level(2)
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()
    async_cb.reset()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    activators[0].reset_coin()
    assert not drm_manager.get('session_status')
    assert not drm_manager.get('license_status')
    activators.autotest(is_activated=False)
    activators[0].generate_coin(randint(1,100))
    assert drm_manager.get('metered_data') == 0
    drm_manager.activate()
    try:
        assert drm_manager.get('metered_data') == 0
        coins = randint(1,100)
        activators[0].generate_coin(coins)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate(True)
        sleep(1)
        #assert drm_manager.get('metered_data') == coins
        activators[0].check_coin(drm_manager.get('metered_data'))
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
        #assert drm_manager.get('metered_data') == coins
        activators[0].check_coin(drm_manager.get('metered_data'))
        activators[0].generate_coin(5)
        activators[0].check_coin(drm_manager.get('metered_data'))
        assert drm_manager.get('metered_data') == coins
    finally:
        drm_manager.deactivate()
    assert drm_manager.get('metered_data') == 0
    del drm_manager
    wait_func_true(lambda: isfile(logpath), 10)
    with open(logpath, 'rt') as f:
        log_content = f.read()
    assert len(list(findall(r'warning\b.*\bCannot access metering data when no session is running', log_content))) == 2
    async_cb.assert_NoError()
    remove(logpath)
