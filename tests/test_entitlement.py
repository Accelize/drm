# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_users_entitlements(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """
    Test the entitlements for all accounts used in regression
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    print()

    # Test user-01 entitlements
    # Request metering license
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_01')
    conf_json.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        drmLicType = drm_manager.get('drm_license_type')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "Metering Web Service error 400" in str(excinfo.value)
        assert "DRM WS request failed" in str(excinfo.value)
        assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
        assert "User account has no entitlement. Purchase additional licenses via your portal" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'User account has no entitlement. Purchase additional licenses via your portal')
        async_cb.reset()
    # Request nodelock license
    try:
        async_cb.reset()
        cred_json.set_user('accelize_accelerator_test_01')
        conf_json.reset()
        conf_json.addNodelock()
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == drmLicType
            with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
                drm_manager.activate()
            assert "Metering Web Service error 400" in str(excinfo.value)
            assert "DRM WS request failed" in str(excinfo.value)
            assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
            assert "User account has no entitlement. Purchase additional licenses via your portal" in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'User account has no entitlement. Purchase additional licenses via your portal')
        async_cb.reset()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-01 entitlements: PASS')

    # Test user-02 entitlements
    # Request metering license
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == drmLicType
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
    async_cb.assert_NoError()
    # Request nodelock license
    try:
        async_cb.reset()
        conf_json.reset()
        conf_json.addNodelock()
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == 'Floating/Metering'
            with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
                drm_manager.activate()
            assert "Metering Web Service error 400" in str(excinfo.value)
            assert "DRM WS request failed" in str(excinfo.value)
            assert search(r'\\"No Entitlement\\" with .+ for \S+_test_02@accelize.com', str(excinfo.value))
            assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'No valid NodeLocked entitlement found for your account')
        async_cb.reset()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-02 entitlements: PASS')

    # Test user-03 entitlements
    # Request metering license
    cred_json.set_user('accelize_accelerator_test_03')
    async_cb.reset()
    conf_json.reset()
    accelize_drm.clean_metering_env(cred_json, ws_admin)
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
    async_cb.assert_NoError()
    # Request nodelock license
    try:
        async_cb.reset()
        conf_json.reset()
        conf_json.addNodelock()
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            # Start application
            assert drm_manager.get('drm_license_type') == 'Idle'
            drm_manager.activate()
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(drm_manager, driver, conf_json, cred_json, ws_admin)
    print('Test user-03 entitlements: PASS')

    # Test user-04 entitlements
    # Request metering license
    cred_json.set_user('accelize_accelerator_test_04')
    async_cb.reset()
    conf_json.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.set(log_verbosity=1)
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Idle'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
    async_cb.assert_NoError()
    # Request nodelock license
    try:
        async_cb.reset()
        conf_json.reset()
        conf_json.addNodelock()
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == 'Floating/Metering'
            with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
                drm_manager.activate()
            assert "Metering Web Service error 400" in str(excinfo.value)
            assert "DRM WS request failed" in str(excinfo.value)
            assert search(r'\\"No Entitlement\\" with .+ for \S+_test_04@accelize.com', str(excinfo.value))
            assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'No valid NodeLocked entitlement found for your account')
        async_cb.reset()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-04 entitlements: PASS')
