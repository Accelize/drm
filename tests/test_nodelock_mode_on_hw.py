from os.path import isdir
from copy import deepcopy
import pytest


def test_nodelock_license_is_not_given_to_inactive_user(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test an user who has not bought a valid nodelocked license cannot get a license"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.addNodelock()
    cred_json.set_user('accelize_accelerator_test_01')

    try:
        # Start application
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'No valid entitlement found for your account' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_function(drm_manager, driver, conf_json, cred_json, ws_admin)


def test_nodelock_normal_case(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()

    try:
        # Load a user who has a valid nodelock license
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        assert drm_manager.get('license_duration') == 0
        expCoins = 10
        activators.generate_coin(expCoins)
        assert drm_manager.get('metered_data') == expCoins
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_function(drm_manager, driver, conf_json, cred_json, ws_admin)


@pytest.mark.skip
@pytest.mark.on_2_fpga
def test_nodelock_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """
    Test behavior when limits are reached. 2 FPGA are required.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    # Set nodelock configuration
    conf_json.addNodelock()
    assert isdir(conf_json['licensing']['license_dir'])

    # Set nodelock user
    cred_json.set_user('jbleclere@plda.com')
    assert cred_json.user == 'jbleclere@plda.com'

    try:

        # Create the first instance
        async_cb0.reset()
        drm_manager0 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver0.read_register_callback,
            driver0.write_register_callback,
            async_cb0.callback
        )
        assert drm_manager0.get('license_type') == 'Node-Locked'

        # Clear the DB Web Service for this product+user to reset the token usage
        product_info = drm_manager0.get('product_info')
        ws_admin.remove_product_information(product_info, cred_json['email'])

        # Consume the single token available
        drm_manager0.activate()
        drm_manager0.deactivate()
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
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager1.activate()
        assert '???' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
        async_cb1.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_function(drm_manager, driver, conf_json, cred_json, ws_admin)


def test_metering_mode_is_blocked_after_nodelock_mode(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    try:
        # Set nodelock configuration
        conf_json.addNodelock()
        conf_json_copy = deepcopy(conf_json)
        cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        drm_manager.activate()
        drm_manager.deactivate()

        # Set metering configuration
        conf_json.reset()
        cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
        assert drm_manager.get('license_type') == 'Floating/Metering'
        # Start application
        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager.activate()
        assert "DRM Controller is locked in Node-Locked licensing mode: To use other modes you must reprogram the FPGA device." in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadUsage.error_code
        async_cb.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_function(drm_manager, driver, conf_json_copy, cred_json, ws_admin)

