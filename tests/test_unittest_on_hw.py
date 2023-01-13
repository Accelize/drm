# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search, IGNORECASE
from time import sleep
from datetime import datetime, timedelta
from flask import request as _request


@pytest.mark.minimum
def test_get_version(accelize_drm):
    """Test the versions of the DRM Lib and its dependencies are well displayed"""
    versions = accelize_drm.get_api_version()
    assert search(r'\d+\.\d+\.\d+', versions.version) is not None


@pytest.mark.long_run
@pytest.mark.hwtst
def test_activation_and_license_status(accelize_drm, conf_json, cred_json, async_handler):
    """Test status of IP activators"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        print()

        # Test license status on start/stop

        # Check all activators are locked
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        # Activate all activators
        drm_manager.activate()
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Deactivate all activators
        drm_manager.deactivate()
        # Check all activators are locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        print('Test license status on start/stop: PASS')


@pytest.mark.long_run
@pytest.mark.hwtst
def test_session_status(accelize_drm, conf_json, cred_json, async_handler):
    """Test status of session"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        print()

        # Test session status on start/stop

        # Check no session is running and no ID is available
        assert not drm_manager.get('session_status')
        assert len(drm_manager.get('session_id')) == 0
        # Activate new session
        drm_manager.activate()
        # Check a session is running with a valid ID
        assert drm_manager.get('session_status')
        assert len(drm_manager.get('session_id')) == 16
        # Deactivate current session
        drm_manager.deactivate()
        # Check session is closed
        assert not drm_manager.get('session_status')
        assert len(drm_manager.get('session_id')) == 0
        print('Test session status on start/stop: PASS')


@pytest.mark.long_run
@pytest.mark.hwtst
def test_license_expiration(accelize_drm, conf_json, cred_json, async_handler):
    """Test license expiration"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        print()

        # Test license does not expire after 3 duration periods when start

        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        # Start
        drm_manager.activate()
        start = datetime.now()
        # Check license is running
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Wait 3 duration periods
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=3*lic_duration+2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Stop
        drm_manager.deactivate()
        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        print('Test license does not expire after 3 duration periods when start: PASS')


@pytest.mark.hwtst
def test_multiple_call(accelize_drm, conf_json, cred_json, async_handler):
    """Test multiple calls to activate and deactivate"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        print()

        # Test multiple activate

        # Check license is inactive
        assert not drm_manager.get('license_status')
        # Start
        drm_manager.activate()
        # Check license is active
        assert drm_manager.get('license_status')
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        # Check license is active
        assert drm_manager.get('license_status')
        # Check a session is valid
        session_id2 = drm_manager.get('session_id')
        assert len(session_id2) == 16
        assert session_id2 == session_id
        # Start again
        drm_manager.activate()
        # Check license is active
        assert drm_manager.get('license_status')
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        assert session_id != session_id2
        async_cb.assert_NoError()

        # Test multiple deactivate

        # Check license is active
        assert drm_manager.get('license_status')
        # Stop
        drm_manager.deactivate()
        # Check license is in active
        assert not drm_manager.get('license_status')
        # Check session ID is invalid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 0
        # Stop again
        drm_manager.deactivate()
        # Check license is in active
        assert not drm_manager.get('license_status')
        # Check session ID is invalid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 0
        async_cb.assert_NoError()


def test_security_stop(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test the session is stopped in case of abnormal termination
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager0.activate()
    assert drm_manager0.get('session_status')
    session_id = drm_manager0.get('session_id')
    assert len(session_id) > 0
    del drm_manager0

    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager1.get('session_status')
    assert len(drm_manager1.get('session_id')) == 0
    async_cb.assert_NoError()


@pytest.mark.minimum
@pytest.mark.skip(reason='78.153.251.226 server is not repsonding anymore')
def test_curl_host_resolve(accelize_drm, conf_json, cred_json, async_handler):
    """Test host resolve information is taken into account by DRM Library"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.reset()
    url = conf_json['licensing']['url']
    conf_json['licensing']['host_resolves'] = {'%s:443' % url.replace('https://',''): '92.222.139.190'}
    conf_json.save()
    async_cb.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMExternFail) as excinfo:
            drm_manager.activate()
        assert 'Failed to perform HTTP request ' in str(excinfo.value)
        assert search(r'peer certificate', str(excinfo.value), IGNORECASE)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMExternFail.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMExternFail.error_code, 'peer certificate')
    async_cb.reset()
