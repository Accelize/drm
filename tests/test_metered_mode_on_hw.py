"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_with_hardware.py> --cred <path/to/cred.json> --library_verbosity=3 --server='dev' --backend='c++' -s
"""

from time import sleep
from random import randint
import pytest


@pytest.mark.quick_on_hw
def test_metered_start_stop_short_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]

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
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        drm_manager.activate()
        sleep(1)
        session_status = drm_manager.get('session_status')
        assert session_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        ip_status = activators.get_status(0)
        assert ip_status
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators.generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate()
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


def test_metered_start_stop_long_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]

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
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        session_status = drm_manager.get('session_status')
        assert session_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        ip_status = activators.get_status(0)
        assert ip_status
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators.generate_coin(10)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        coins_ref = coins
        for i in range(3):
            wait_period = randint(license_duration-2, license_duration+2)
            sleep(wait_period)
            new_coins = randint(1,10)
            activators.generate_coin(new_coins)
            coins = drm_manager.get('metered_data')
            assert coins == coins_ref + new_coins
            coins_ref = coins
        drm_manager.deactivate()
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
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
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        drm_manager.activate()
        sleep(1)
        session_status = drm_manager.get('session_status')
        assert session_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        ip_status = activators.get_status(0)
        assert ip_status
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators.generate_coin(new_coins)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        drm_manager.deactivate(True)    # Pause
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()

@pytest.mark.skip
def test_metered_pause_resume_long_time(accelize_drm, conf_json, cred_json, async_handler):
    """Test no error occurs in normal metering mode."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]

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
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        session_status = drm_manager.get('session_status')
        assert session_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        ip_status = activators.get_status(0)
        assert ip_status
        orig_coins = drm_manager.get('metered_data')
        new_coins = 10
        activators.generate_coin(10)
        coins = drm_manager.get('metered_data')
        assert coins == orig_coins + new_coins
        coins_ref = coins
        for i in range(3):
            wait_period = randint(license_duration-2, license_duration+2)
            sleep(wait_period)
            new_coins = randint(1,10)
            activators.generate_coin(new_coins)
            coins = drm_manager.get('metered_data')
            assert coins == coins_ref + new_coins
            coins_ref = coins
        drm_manager.deactivate()
        session_status = drm_manager.get('session_status')
        assert not session_status
        ip_status = activators.get_status(0)
        assert not ip_status
        coins = drm_manager.get('metered_data')
        assert coins == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


@pytest.mark.skip(reason='todo')
def test_limits():
    pass