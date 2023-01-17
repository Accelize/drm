# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search, IGNORECASE


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user1_metering(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the metering entitlement of user1
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_01')
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
        assert drm_manager.get('license_type') == 'Floating/Metering'
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
def test_entitlement_user1_nodelock(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the nodelock entitlement of user1
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_01')
    conf_json.reset()
    conf_json.addNodelock()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    try:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
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
    finally:
        driver.program_fpga()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user2_metering(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the metering entitlement of user2
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_02')
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
        assert drm_manager.get('license_type') == 'Floating/Metering'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
    async_cb.assert_NoError()
    log_content = logfile.read()
    assert search(r'DRM session .{16} started', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user2_nodelock(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the nodelock entitlement of user2
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    conf_json.addNodelock()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.load(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
                drm_manager.activate()
            assert "Metering Web Service error 400" in str(excinfo.value)
            assert "DRM WS request failed" in str(excinfo.value)
            assert search(r'No Entitlement.* with PT DRM Ref Design .+ for \S+_test_02@accelize.com', str(excinfo.value))
            assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'No valid NodeLocked entitlement found for your account')
        async_cb.reset()
        log_content = logfile.read()
        assert search(r'Failed to request license file', log_content, IGNORECASE)
        assert search(r'No valid NodeLocked entitlement found for your account', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user3_metering(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the metering entitlement of user3
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.load(conf_json, cred_json)
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
    async_cb.assert_NoError()
    log_content = logfile.read()
    assert search(r'DRM session .{16} started', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user3_nodelock(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the nodelock entitlement of user3
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.reset()
    conf_json.addNodelock()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.load(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            drm_manager.activate()
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            drm_manager.deactivate()
        async_cb.assert_NoError()
        log_content = logfile.read()
        assert search(r'Installed node-locked license successfully', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user4_metering(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the metering entitlement of user4
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_04')
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.load(conf_json, cred_json)
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
    async_cb.assert_NoError()
    log_content = logfile.read()
    assert search(r'DRM session .{16} started', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.no_parallel
def test_entitlement_user4_nodelock(accelize_drm, conf_json, cred_json, async_handler,
                ws_admin, log_file_factory):
    """
    Test the nodelock entitlement of user4
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_04')
    conf_json.reset()
    conf_json.addNodelock()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.load(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
                drm_manager.activate()
            assert "Metering Web Service error 400" in str(excinfo.value)
            assert "DRM WS request failed" in str(excinfo.value)
            assert search(r'No Entitlement.* with PT DRM Ref Design .+ for \S+_test_04@accelize.com', str(excinfo.value))
            assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'No valid NodeLocked entitlement found for your account')
        async_cb.reset()
        log_content = logfile.read()
        assert search(r'Failed to request license file', log_content, IGNORECASE)
        assert search(r'No valid NodeLocked entitlement found for your account', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()
