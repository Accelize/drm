"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_with_hardware.py> --cred <path/to/cred.json> --library_verbosity=3 --server='dev' --backend='c++' -s
"""

from time import sleep
from random import randint
from datetime import datetime, timedelta
from re import search
import pytest


@pytest.mark.minimum
def test_metered_start_stop_short_time(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal start/stop metering mode during a short period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
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
        assert not activators.is_activated()
        drm_manager.activate()
        sleep(1)
        coins = drm_manager.get('metered_data')
        assert coins == 0
        assert drm_manager.get('license_status')
        assert activators.is_activated()
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators[0].generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        coins = drm_manager.get('metered_data')
        assert coins == 0
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
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 4
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
        assert not activators.is_activated()
        drm_manager.activate()
        sleep(1)
        coins = drm_manager.get('metered_data')
        assert coins == 0
        assert drm_manager.get('license_status')
        assert activators.is_activated()
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators[0].generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.long_run
def test_metered_start_stop_long_time(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal start/stop metering mode during a long period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
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
        assert not activators.is_activated()
        drm_manager.activate()
        start = datetime.now()
        license_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        coins = drm_manager.get('metered_data')
        assert coins == 0
        assert activators.is_activated()
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators[0].generate_coin(10)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        coins_ref = coins
        for i in range(3):
            wait_period = randint(license_duration-2, license_duration+2)
            sleep(wait_period)
            start += timedelta(seconds=license_duration)
            new_coins = randint(1,10)
            activators[0].generate_coin(new_coins)
            coins = drm_manager.get('metered_data')
            assert coins == coins_ref + new_coins
            coins_ref = coins
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.minimum
def test_metered_pause_resume_short_time(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal pause/resume metering mode during a short period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
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
        assert not activators.is_activated()
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('metered_data') == 0
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        assert activators.is_activated()
        lic_duration = drm_manager.get('license_duration')
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators[0].generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        assert activators.is_activated()
        # Wait right before license expiration
        nb_lic_expired = int((datetime.now() - start).total_seconds() / lic_duration)
        wait_period = start + timedelta(seconds=(nb_lic_expired+2)*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        assert drm_manager.get('session_id') == session_id
        assert activators.is_activated()
        # Wait expiration
        sleep(4)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        drm_manager.activate(True)
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert drm_manager.get('license_status')
        assert activators.is_activated()
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        assert drm_manager.get('session_id') != session_id
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.long_run
def test_metered_pause_resume_long_time(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test no error occurs in normal start/stop metering mode during a long period of time
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
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
        assert not activators.is_activated()
        async_cb.assert_NoError()
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('metered_data') == 0
        assert drm_manager.get('session_status')
        assert drm_manager.get('license_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        lic_duration = drm_manager.get('license_duration')
        assert activators.is_activated()
        coins_ref = drm_manager.get('metered_data')
        for i in range(3):
            new_coins = randint(1, 100)
            activators[0].generate_coin(new_coins)
            coins = drm_manager.get('metered_data')
            assert coins == coins_ref + new_coins
            coins_ref = coins
            drm_manager.deactivate(True)
            async_cb.assert_NoError()
            assert drm_manager.get('session_status')
            assert drm_manager.get('license_status')
            assert drm_manager.get('session_id') == session_id
            # Wait randomly
            nb_lic_expired = int((datetime.now() - start).total_seconds() / lic_duration)
            random_wait = randint((nb_lic_expired+2)*lic_duration-2, (nb_lic_expired+2)*lic_duration+2)
            wait_period = start + timedelta(seconds=random_wait) - datetime.now()
            sleep(wait_period.total_seconds())
            drm_manager.activate(True)
            start = datetime.now()
        assert drm_manager.get('session_status')
        assert drm_manager.get('session_id') == session_id
        assert drm_manager.get('license_status')
        assert activators.is_activated()
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        assert drm_manager.get('session_id') != session_id
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.minimum
@pytest.mark.no_parallel
def test_metering_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """
    Test an error is returned and the design is locked when the limit is reached.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
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
        new_coins = 999
        activators[0].generate_coin(new_coins)
        assert drm_manager.get('metered_data') == new_coins
        sleep(1)
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('metered_data') == 0
        new_coins = 1
        activators[0].generate_coin(new_coins)
        assert drm_manager.get('metered_data') == new_coins
        sleep(1)
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'You reach the licensed , usage_unit  limit: 1000' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()
    print('Test activate function fails when limit is reached: PASS')

    # Test background thread stops when limit is reached
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
        start = datetime.now()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_status')
        assert drm_manager.get('metered_data') == 0
        lic_duration = drm_manager.get('license_duration')
        sleep(2)
        new_coins = 1000
        activators[0].generate_coin(new_coins)
        assert drm_manager.get('metered_data') == new_coins
        # Wait right before expiration
        nb_lic_expired = int((datetime.now() - start).total_seconds() / lic_duration)
        wait_period = start + timedelta(seconds=(nb_lic_expired+3)*lic_duration-3) - datetime.now()
        sleep(wait_period.total_seconds())
        assert drm_manager.get('license_status')
        assert activators.is_activated()
        sleep(5)
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
        # Verify asynchronous callback has been called
        assert async_cb.was_called
        assert 'You reach the licensed , usage_unit  limit: 1000' in async_cb.message
        assert async_cb.errcode == accelize_drm.exceptions.DRMWSReqError.error_code
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        assert not activators.is_activated()
    finally:
        drm_manager.deactivate()
    print('Test background thread stops when limit is reached: PASS')


@pytest.mark.on_2_fpga
@pytest.mark.minimum
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
