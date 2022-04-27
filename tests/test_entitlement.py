# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search, MULTILINE


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
        assert "Metering Web Service error 400" in str(excinfo.value)
        assert "DRM WS request failed" in str(excinfo.value)
        assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
        assert "User account has no entitlement. Purchase additional licenses via your portal" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'User account has no entitlement. Purchase additional licenses via your portal')
        async_cb.reset()
    log_content = logfile.read()
    assert search(r'User account has no entitlement.', log_content, MULTILINE)
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
            assert "Metering Web Service error 400" in str(excinfo.value)
            assert "DRM WS request failed" in str(excinfo.value)
            assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
            assert "User account has no entitlement. Purchase additional licenses via your portal" in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'User account has no entitlement. Purchase additional licenses via your portal')
        async_cb.reset()
        log_content = logfile.read()
        assert search(r'Failed to request license file', log_content, MULTILINE)
        assert search(r'User account has no entitlement.', log_content, MULTILINE)
        logfile.remove()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)


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
    assert search(r'DRM session .{16} started', log_content, MULTILINE)
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
            assert search(r'\\"No Entitlement\\" with .+ for \S+_test_02@accelize.com', str(excinfo.value))
            assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'No valid NodeLocked entitlement found for your account')
        async_cb.reset()
        log_content = logfile.read()
        assert search(r'Failed to request license file', log_content, MULTILINE)
        assert search(r'No valid NodeLocked entitlement found for your account', log_content, MULTILINE)
        logfile.remove()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)


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
    accelize_drm.clean_metering_env(cred_json, ws_admin)
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
    assert search(r'DRM session .{16} started', log_content, MULTILINE)
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
    accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)
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
        assert search(r'Installed node-locked license successfully', log_content, MULTILINE)
        logfile.remove()
    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)
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
    assert search(r'DRM session .{16} started', log_content, MULTILINE)
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
            assert search(r'\\"No Entitlement\\" with .+ for \S+_test_04@accelize.com', str(excinfo.value))
            assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'No valid NodeLocked entitlement found for your account')
        async_cb.reset()
        log_content = logfile.read()
        assert search(r'Failed to request license file', log_content, MULTILINE)
        assert search(r'No valid NodeLocked entitlement found for your account', log_content, MULTILINE)
        logfile.remove()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
