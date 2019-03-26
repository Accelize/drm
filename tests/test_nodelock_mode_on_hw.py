from os.path import isdir

import pytest


def test_nodelock_license_is_not_given_to_inactive_user(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test an user who has not bought a valid nodelocked license cannot get a license"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.addNodelock()
    cred_json.set_user('accelize_accelerator_test_01')

    try:
        # Start application
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert not drm_manager.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'No valid entitlement found for your account' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(drm_manager, driver, conf_json, cred_json, ws_admin)


@pytest.mark.minimum
@pytest.mark.no_parallel
def test_nodelock_normal_case(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()

    try:
        # Load a user who has a valid nodelock license
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert not drm_manager.get('license_status')
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        assert drm_manager.get('license_duration') == 0
        expCoins = 10
        activators[0].generate_coin(expCoins)
        assert drm_manager.get('metered_data') == expCoins
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)


@pytest.mark.on_2_fpga
@pytest.mark.minimum
def test_nodelock_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """
    Test behavior when limits are reached. 2 FPGA are required.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()

    try:

        # Create the first instance
        async_cb0.reset()
        accelize_drm.clean_nodelock_env(None, driver0, conf_json, cred_json, ws_admin)
        drm_manager0 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver0.read_register_callback,
            driver0.write_register_callback,
            async_cb0.callback
        )
        assert drm_manager0.get('license_type') == 'Node-Locked'

        # Consume the single token available
        assert drm_manager0.get('drm_license_type') == 'Floating/Metering'
        drm_manager0.activate()
        assert drm_manager0.get('drm_license_type') == 'Node-Locked'
        drm_manager0.deactivate()
        assert drm_manager0.get('drm_license_type') == 'Node-Locked'
        async_cb0.assert_NoError()

        # Create the second instance on the other FPGA
        drm_manager1 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver1.read_register_callback,
            driver1.write_register_callback,
            async_cb1.callback
        )
        assert drm_manager1.get('license_type') == 'Node-Locked'
        assert drm_manager1.get('drm_license_type') == 'Floating/Metering'
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager1.activate()
        assert 'You reach the Nodelocked entitlement limit: 1' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb1.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_env(None, driver0, conf_json, cred_json, ws_admin)


@pytest.mark.no_parallel
def test_metering_mode_is_blocked_after_nodelock_mode(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license

    try:
        # Set nodelock configuration
        conf_json.addNodelock()
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        drm_manager.deactivate()

        # Set metering configuration
        conf_json.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Floating/Metering'
        # Start application
        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager.activate()
        assert "DRM Controller is locked in Node-Locked licensing mode: To use other modes you must reprogram the FPGA device" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadUsage.error_code
        async_cb.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)

