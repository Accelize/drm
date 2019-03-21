"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_with_hardware.py> --cred <path/to/cred.json> --library_verbosity=3 --server='dev' --backend='c++' -s
"""

from time import sleep
from random import randint
import pytest


def test_metered_start_stop_short_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    # Test when authentication url in configuration file is wrong
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
        assert not activators.is_activated()
        drm_manager.activate()
        sleep(1)
        coins = drm_manager.get('metered_data')
        assert coins == 0
        assert drm_manager.get('session_status')
        assert activators.is_activated()
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators[0].generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not activators.is_activated()
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.on_dev_only
def test_metered_start_stop_long_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    # Test when authentication url in configuration file is wrong
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
        assert not activators.is_activated()
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        assert drm_manager.get('session_status')
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
            new_coins = randint(1,10)
            activators[0].generate_coin(new_coins)
            coins = drm_manager.get('metered_data')
            assert coins == coins_ref + new_coins
            coins_ref = coins
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not activators.is_activated()
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.skip
def test_metered_pause_resume_short_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    # Test when authentication url in configuration file is wrong
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
        assert not activators.is_activated()
        drm_manager.activate()
        sleep(1)
        assert drm_manager.get('session_status')
        coins = drm_manager.get('metered_data')
        assert coins == 0
        assert activators.is_activated()
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators[0].generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate(True)    # Pause
        assert not drm_manager.get('session_status')
        assert not activators.is_activated()
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.on_dev_only
@pytest.mark.skip
def test_metered_pause_resume_long_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    # Test when authentication url in configuration file is wrong
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
        assert not activators.is_activated()
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        assert drm_manager.get('session_status')
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
            new_coins = randint(1,10)
            activators[0].generate_coin(new_coins)
            coins = drm_manager.get('metered_data')
            assert coins == coins_ref + new_coins
            coins_ref = coins
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        assert not activators.is_activated()
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


def test_metering_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_03')

    # Test when authentication url in configuration file is wrong
    async_cb.reset()
    conf_json.reset()
    accelize_drm.clean_nodelock(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('session_status')
        drm_manager.activate()
        sleep(1)
        assert drm_manager.get('session_status')
        assert drm_manager.get('metered_data') == 0
        new_coins = 999
        activators[0].generate_coin(new_coins)
        assert drm_manager.get('metered_data') == new_coins
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        drm_manager.activate()
        assert drm_manager.get('session_status')
        sleep(1)
        assert drm_manager.get('metered_data') == 0
        new_coins = 1
        activators[0].generate_coin(new_coins)
        assert drm_manager.get('metered_data') == new_coins
        drm_manager.deactivate()
        assert not drm_manager.get('session_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'You reach the licensed , usage_unit  limit: 1000' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


def test_floating_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    pass