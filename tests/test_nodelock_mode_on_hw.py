from os.path import isdir
import pytest


def test_nodelock_license_is_not_given_to_inactive_user(accelize_drm, conf_json, cred_json, async_handler, tmpdir):
    """Test an user who has not bought a valid nodelocked license cannot get a license"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Set nodelock configuration
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = str(tmpdir)
    conf_json.save()
    assert isdir(conf_json['licensing']['license_dir'])

    # Load a user who has not a valid nodelock license
    cred_json.set_user('jbleclere@accelize.com')
    assert cred_json.user == 'jbleclere@accelize.com'

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
    assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_NoError()


def test_nodelock_normal_case(accelize_drm, conf_json, cred_json, async_handler, tmpdir, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Set nodelock configuration
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = str(tmpdir)
    conf_json.save()
    assert isdir(conf_json['licensing']['license_dir'])

    # Load a user who has a valid nodelock license
    cred_json.set_user('jbleclere@plda.com')        # User with a single nodelock license
    assert cred_json.user == 'jbleclere@plda.com'
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )
    assert drm_manager.get('license_type') == 'Node-Locked'

    # Clear the DB Web Service for this product+user to reset the token usage
    product_info = drm_manager.get('product_info')
    ws_admin.remove_product_information(product_info, cred_json.user)

    # Start application
    drm_manager.activate()
    lic_duration = drm_manager.get('license_duration')
    print('lic_duration=', lic_duration)
    assert lic_duration == 0
    expCoins = 10
    generate_coin(expCoins)
    assert drm_manager.get('metered_data') == expCoins
    drm_manager.deactivate()
    async_cb.assert_NoError()

@pytest.mark.skip
@pytest.mark.require2fpga
def test_nodelock_limits(accelize_drm, conf_json, cred_json, async_handler, tmpdir, ws_admin):
    """
    Test behavior when limits are reached. 2 FPGA are required.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    # Set nodelock configuration
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = str(tmpdir)
    conf_json.save()
    assert isdir(conf_json['licensing']['license_dir'])

    # Set nodelock user
    cred_json.set_user('jbleclere@plda.com')
    assert cred_json.user == 'jbleclere@plda.com'

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
    ws_admin.remove_product_information(product_info, cred_json.user)

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
    assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    async_cb1.assert_NoError()


