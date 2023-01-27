# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search, IGNORECASE
from datetime import datetime
from tests.conftest import wait_deadline, wait_until_true


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user_noentitlement(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the test-noentitlement user's entitlement
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('test-noentitlement')
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert search(r'Accelize Web Service error 403', str(excinfo.value))
        assert search(r"No valid entitlement available for this product", str(excinfo.value))
        assert search(r"You can use this product by subscribing new entitlements", str(excinfo.value))
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, "You can use this product by subscribing new entitlements")
        async_cb.reset()
    log_content = logfile.read()
    assert search(r'No valid entitlement available for this product', log_content, IGNORECASE)
    assert search(r'You can use this product by subscribing new entitlements', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user_metering(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the test-metering user's entitlement
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('test-metering')
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
        drm_manager.activate()
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
    async_cb.assert_NoError()
    log_content = logfile.read()
    assert search(r'DRM Controller is in Metering license mode', log_content, IGNORECASE)
    assert search(r'DRM session .{16} started', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user_nodelock(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the test-nodelock user's entitlement
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('test-nodelock')
    conf_json.reset()
#    conf_json.addNodelock()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.clean_user_db(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
            drm_manager.activate()
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
        async_cb.assert_NoError()
        log_content = logfile.read()
        assert search(r'DRM Controller is in Node-Locked license mode', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()
        cred_json.set_user()
        conf_json.reset()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('drm_license_type') == 'Idle'


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user_nodelock_when_forced(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the test-nodelock user's entitlement when forced to use nodelock
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('test-nodelock')
    conf_json.reset()
    conf_json.addNodelock()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.clean_user_db(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
            drm_manager.activate()
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
        async_cb.assert_NoError()
        log_content = logfile.read()
        assert search(r'DRM Controller is in Node-Locked license mode', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()
        cred_json.set_user()
        conf_json.reset()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('drm_license_type') == 'Idle'


@pytest.mark.minimum
def test_force_nodelock_to_inactive_user(accelize_drm, conf_json, cred_json,
                                                        async_handler, ws_admin):
    """Test a user who has not bought a valid nodelocked license cannot get a license"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-noentitlement')
    conf_json.addNodelock()
    ws_admin.clean_user_db(conf_json, cred_json)

    # Start application
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert not drm_manager.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Accelize Web Service error 403' in str(excinfo.value)
        assert search(r'No valid entitlement available for this product', str(excinfo.value))
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'Accelize Web Service error 403')
    async_cb.reset()


@pytest.mark.minimum
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_entitlement_user_limited_on_activate(accelize_drm, conf_json, cred_json, async_handler,
                        log_file_factory, ws_admin):
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
    cred_json.set_user('test-limited')
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.clean_user_db(conf_json, cred_json)
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_type') == 'Floating/Metering'
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
        assert search(r'Accelize Web Service error 403', str(excinfo.value), IGNORECASE)
        assert search(r'No valid entitlement available for this product', str(excinfo.value), IGNORECASE)
        assert search(r'your existing entitlements have reached their quota or expiration date', str(excinfo.value), IGNORECASE)
        assert async_cb.was_called
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'your existing entitlements have reached their quota or expiration date')
        async_cb.reset()
    log_content = logfile.read()
    assert search(r'Accelize Web Service error 403', log_content, IGNORECASE)
    assert search(r'No valid entitlement available for this product', log_content, IGNORECASE)
    assert search(r'your existing entitlements have reached their quota or expiration date', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_entitlement_user_limited_in_thread(accelize_drm, conf_json, cred_json, async_handler,
                        log_file_factory, ws_admin):
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
    cred_json.set_user('test-limited')
    ws_admin.clean_user_db(conf_json, cred_json)
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        start = datetime.now()
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_status')
        assert sum(drm_manager.get('metered_data')) == 0
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        activators[0].generate_coin(999)
        activators.check_coin(drm_manager.get('metered_data'))
        # Wait right after 1st license timer
        wait_deadline(start, license_duration+1)
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        activators[0].generate_coin(1)
        activators.check_coin(drm_manager.get('metered_data'))
        # Wait right after 2nd license timer
        wait_deadline(start, 2*license_duration+1)
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Wait right after 3rd license timer
        wait_deadline(start, 3*license_duration+1)
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
    # Verify asynchronous callback has been called
    assert async_cb.was_called
    assert search(r'Accelize Web Service error 403', async_cb.message, IGNORECASE)
    assert search(r'No valid entitlement available for this product', async_cb.message, IGNORECASE)
    assert search(r'your existing entitlements have reached their quota or expiration date', async_cb.message, IGNORECASE)
    async_cb.reset()
    log_content = logfile.read()
    assert search(r'Accelize Web Service error 403', log_content, IGNORECASE)
    assert search(r'No valid entitlement available for this product', log_content, IGNORECASE)
    assert search(r'your existing entitlements have reached their quota or expiration date', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.on_2_fpga
@pytest.mark.minimum
@pytest.mark.hwtst
def test_entitlement_user_floating(accelize_drm, conf_json, conf_json_second, cred_json,
                        async_handler, log_file_factory):
    """
    Test an error is returned when the floating limit is reached
    """
    if len(accelize_drm.pytest_fpga_driver) < 2:
        pytest.skip('Skip test because 2 FPGA are needed but only 1 is found')

    cred_json.set_user('test-floating')

    # Settings for app running on board #0
    driver0 = accelize_drm.pytest_fpga_driver[0]
    async_cb0 = async_handler.create()
    async_cb0.reset()
    conf_json.reset()
    logfile0 = log_file_factory.create(1)
    conf_json['settings'].update(logfile0.json)
    conf_json['settings']['ws_api_retry_duration'] = 4
    conf_json.save()

    # Settings for app running on board #1
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb1 = async_handler.create()
    async_cb1.reset()
    logfile1 = log_file_factory.create(1)
    conf_json_second['settings'].update(logfile1.json)
    conf_json_second['settings']['ws_api_retry_duration'] = 4
    conf_json_second.save()

    # Get floating license for drm_manager0 and check drm_manager1 fails
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver0.read_register_callback,
                driver0.write_register_callback,
                async_cb0.callback
            ) as drm_manager0:
        assert not drm_manager0.get('license_status')
        assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
        with accelize_drm.DrmManager(
                    conf_json_second.path,
                    cred_json.path,
                    driver1.read_register_callback,
                    driver1.write_register_callback,
                    async_cb1.callback
                ) as drm_manager1:
            assert not drm_manager1.get('license_status')
            assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
            assert not drm_manager0.get('license_status')
            assert drm_manager.get('drm_license_type') in ['Idle', 'Floating/Metering']
            drm_manager0.activate()
            assert drm_manager0.get('license_status')
            assert drm_manager0.get('license_type') == 'Floating/Metering'
            assert drm_manager0.get('drm_license_type') == 'Floating/Metering'
            with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
                drm_manager1.activate()
            assert search(r'Timeout on License request after .+ attempts', str(excinfo.value))
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSTimedOut.error_code
            async_cb1.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, 'Timeout on License request after')
            async_cb1.reset()
    async_cb0.assert_NoError()
    log_content = logfile0.read()
    assert search(r'DRM Controller is in Metering license mode', log_content, IGNORECASE)
    assert search(r'Entitlement Limit Reached', log_content, IGNORECASE)
    assert search(r'You have reached the maximum quantity of 1000', log_content, IGNORECASE)
    log_content = logfile1.read()
    assert search(r'Entitlement Limit Reached', log_content, IGNORECASE)
    assert search(r'You have reached the maximum quantity of 1000', log_content, IGNORECASE)

    # Get floating license for drm_manager1 and check drm_manager0 fails
    with accelize_drm.DrmManager(
                conf_json_second.path,
                cred_json.path,
                driver1.read_register_callback,
                driver1.write_register_callback,
                async_cb1.callback
            ) as drm_manager1:
        assert not drm_manager1.get('license_status')
        assert drm_manager1.get('drm_license_type') == 'Floating/Metering'
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver0.read_register_callback,
                    driver0.write_register_callback,
                    async_cb0.callback
                ) as drm_manager0:
            assert not drm_manager0.get('license_status')
            assert drm_manager0.get('drm_license_type') == 'Floating/Metering'
            assert not drm_manager1.get('license_status')
            assert drm_manager1.get('drm_license_type') == 'Floating/Metering'
            drm_manager1.activate()
            assert drm_manager1.get('license_status')
            assert drm_manager1.get('license_type') == 'Floating/Metering'
            assert drm_manager1.get('drm_license_type') == 'Floating/Metering'
            with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
                drm_manager0.activate()
            assert search(r'Timeout on License request after .+ attempts', str(excinfo.value))
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSTimedOut.error_code
            async_cb0.assert_Error(accelize_drm.exceptions.DRMWSTimedOut.error_code, 'Timeout on License request after')
            async_cb0.reset()
    async_cb1.assert_NoError()
    log_content = logfile0.read()
    assert search(r'Entitlement Limit Reached', log_content, IGNORECASE)
    assert search(r'You have reached the maximum quantity of 1000', log_content, IGNORECASE)
    log_content = logfile1.read()
    assert search(r'DRM Controller is in Metering license mode', log_content, IGNORECASE)
    assert search(r'Entitlement Limit Reached', log_content, IGNORECASE)
    assert search(r'You have reached the maximum quantity of 1000', log_content, IGNORECASE)
    logfile0.remove()
    logfile1.remove()
