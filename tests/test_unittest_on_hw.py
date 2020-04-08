# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import match, search, finditer, MULTILINE
from time import sleep, time
from json import loads
from datetime import datetime, timedelta


LOG_FORMAT_SHORT = "[%^%=8l%$] %-6t, %v"
LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

_PARAM_LIST = ['license_type',
               'license_duration',
               'num_activators',
               'session_id',
               'session_status',
               'license_status',
               'metered_data',
               'nodelocked_request_file',
               'drm_frequency',
               'drm_license_type',
               'product_info',
               'mailbox_size',
               'token_string',
               'token_validity',
               'token_time_left',
               'log_verbosity',
               'log_format',
               'log_file_verbosity',
               'log_file_format',
               'log_file_path',
               'log_file_type',
               'log_file_rotating_size',
               'log_file_rotating_num',
               'bypass_frequency_detection',
               'frequency_detection_method',
               'frequency_detection_threshold',
               'frequency_detection_period',
               'custom_field',
               'mailbox_data',
               'ws_retry_period_long',
               'ws_retry_period_short',
               'ws_request_timeout',
               'log_message_level',
               'list_all',
               'dump_all',
               'page_ctrlreg',
               'page_vlnvfile',
               'page_licfile',
               'page_tracefile',
               'page_meteringfile',
               'page_mailbox',
               'hw_report',
               'trigger_async_callback',
               'bad_product_id',
               'bad_oauth2_token',
               'log_message']


@pytest.mark.minimum
def test_backward_compatibility(accelize_drm, conf_json, cred_json, async_handler):
    from itertools import groupby
    """Test API is not compatible with DRM HDK < 3.0"""
    refdesign = accelize_drm.pytest_ref_designs
    hdk_version = accelize_drm.pytest_hdk_version
    if hdk_version is None:
        pytest.skip("FPGA image is not corresponding to a known HDK version")

    current_major = int(match(r'^(\d+)\.', hdk_version).group(1))
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    drm_manager = None

    try:
        refdesignByMajor = ((int(match(r'^(\d+)\.', x).group(1)), x) for x in refdesign.hdk_versions)

        for major, versions in groupby(refdesignByMajor, lambda x: x[0]):
            if major >= current_major:
                continue

            hdk = sorted((e[1] for e in versions))[0]
            # Program FPGA with older HDK
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)
            # Test compatibility issue
            with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
                async_cb.reset()
                drm_manager = accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
            hit = False
            if 'Unable to find DRM Controller registers' in str(excinfo.value):
                hit =True
            if search(r'This DRM Library version \S+ is not compatible with the DRM HDK version', str(excinfo.value)):
                hit =True
            assert hit
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
            async_cb.assert_NoError()
            print('Test compatibility with %s: PASS' % hdk)

    finally:
        if drm_manager:
            del drm_manager
        gc.collect()
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.minimum
def test_get_version(accelize_drm):
    """Test the versions of the DRM Lib and its dependencies are well displayed"""
    versions = accelize_drm.get_api_version()
    assert search(r'\d+\.\d+\.\d+', versions.version) is not None


def test_authentication_token(accelize_drm, conf_json, cred_json, async_handler):
    """Test authentication token behavior"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = None
    print()
    try:
        # Test when token is wrong
        async_cb.reset()
        conf_json.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.set(bad_oauth2_token=1)
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "invalid_client" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
        print('Test when token is wrong: PASS')

        # Test token validity after deactivate
        async_cb.reset()
        conf_json.reset()
        cred_json.set_user('accelize_accelerator_test_02')
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.activate()
        token_time_left = drm_manager.get('token_time_left')
        if token_time_left < 15:
            drm_manager.deactivate()
            # Wait expiration of current oauth2 token before starting test
            sleep(16)
            drm_manager.activate()
        token_validity = drm_manager.get('token_validity')
        assert token_validity > 15
        exp_token_string = drm_manager.get('token_string')
        drm_manager.deactivate()
        token_string = drm_manager.get('token_string')
        assert token_string == exp_token_string
        drm_manager.activate()
        token_string = drm_manager.get('token_string')
        assert token_string == exp_token_string
        drm_manager.deactivate()
        token_string = drm_manager.get('token_string')
        assert token_string == exp_token_string
        async_cb.assert_NoError()
        print('Test token validity after deactivate: PASS')

#        # Test when token has expired
#        async_cb.reset()
#        conf_json.reset()
#        drm_manager = accelize_drm.DrmManager(
#            conf_json.path,
#            cred_json.path,
#            driver.read_register_callback,
#            driver.write_register_callback,
#            async_cb.callback
#        )
#        drm_manager.activate()
#        start = datetime.now()
#        drm_manager.deactivate()
#        exp_token_string = drm_manager.get('token_string')
#        token_validity = drm_manager.get('token_validity')
#        token_expired_in = drm_manager.get('token_expired_in')
#        exp_token_validity = 10
#        drm_manager.set(token_validity=exp_token_validity)
#        token_validity = drm_manager.get('token_validity')
#        assert token_validity == exp_token_validity
#        token_expired_in = drm_manager.get('token_expired_in')
#        ts = drm_manager.get('token_string')
#        assert token_expired_in > token_validity/3
#        assert token_expired_in > 3
#        # Wait right before the token expires and verifiy it is the same
#        wait_period = start + timedelta(seconds=token_expired_in-3) - datetime.now()
#        sleep(wait_period.total_seconds())
#        drm_manager.activate()
#        drm_manager.deactivate()
#        token_string = drm_manager.get('token_string')
#        assert token_string == exp_token_string
#        sleep(4)
#        drm_manager.activate()
#        drm_manager.deactivate()
#        token_string = drm_manager.get('token_string')
#        assert token_string != exp_token_string
#        async_cb.assert_NoError()
#        print('Test when token has expired: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


@pytest.mark.long_run
@pytest.mark.hwtst
def test_activation_and_license_status(accelize_drm, conf_json, cred_json, async_handler):
    """Test status of IP activators"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
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

        # Test license status on start/pause

        # Check all activators are locked
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        # Activate all activators
        drm_manager.activate()
        start = datetime.now()
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Pause all activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        async_cb.assert_NoError()
        print('Test license status on start/pause: PASS')

        # Test license status on resume from valid license/pause

        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Resume all activators
        drm_manager.activate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Pause all activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        activators.autotest(is_activated=True)
        # Wait until license expires
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=2 * lic_duration + 1) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check all activators are now locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        print('Test license status on resume from valid license/pause: PASS')

        # Test license status on resume from expired license/pause

        # Check all activators are locked
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        # Resume all activators
        drm_manager.activate(True)
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Pause all activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        async_cb.assert_NoError()
        print('Test license status on resume from expired license/pause: PASS')

        # Test license status on resume/stop

        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        async_cb.assert_NoError()
        # Resume all activators
        drm_manager.activate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Deactivate all activators
        drm_manager.deactivate()
        # Check all activators are locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        print('Test license status on resume/stop: PASS')

        # Test license status on restart from paused session/stop

        # Check all activators are locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        # Activate all activators
        drm_manager.activate()
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Pause activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        # Restart all activators
        drm_manager.activate()
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        activators.autotest(is_activated=True)
        async_cb.assert_NoError()
        print('Test license status on restart: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


@pytest.mark.long_run
@pytest.mark.hwtst
def test_session_status(accelize_drm, conf_json, cred_json, async_handler):
    """Test status of session"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        print()

        # Test session status on start/stop

        # Check no session is running and no ID is available
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(session_id) == 0, 'A session ID exists'
        # Activate new session
        drm_manager.activate()
        # Check a session is running with a valid ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        # Deactivate current session
        drm_manager.deactivate()
        # Check session is closed
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(session_id) == 0, 'A session ID exists'
        print('Test session status on start/stop: PASS')

        # Test session status on start/pause

        # Check no session is running and no ID is available
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(session_id) == 0, 'A session ID exists'
        # Activate new session
        drm_manager.activate()
        start = datetime.now()
        # Check a session is running with a valid ID
        status = drm_manager.get('session_status')
        id_ref = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id_ref) == 16, 'No session ID is returned'
        # Pause current session
        drm_manager.deactivate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        async_cb.assert_NoError()
        print('Test session status on start/pause: PASS')

        # Test session status on resume from valid license/pause

        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Resume current session
        drm_manager.activate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Pause current session
        drm_manager.deactivate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Wait until license expires
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=2 * lic_duration + 1) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        async_cb.assert_NoError()
        print('Test session status on resume from valid license/pause: PASS')

        # Test session status on resume from expired license/pause

        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Resume current session
        drm_manager.activate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Pause current session
        drm_manager.deactivate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        async_cb.assert_NoError()
        print('Test session status on resume from expired license/pause: PASS')

        # Test session status on resume/stop

        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Resume current session
        drm_manager.activate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Close session
        drm_manager.deactivate()
        # Check session is closed
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(session_id) == 0, 'A session ID exists'
        async_cb.assert_NoError()
        print('Test session status on resume/stop: PASS')

        # Test session status on start from paused session/stop

        # Check no session is running
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(session_id) == 0, 'A session ID exists'
        # Start a new session
        drm_manager.activate()
        # Check a session is alive with a new ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id != id_ref, 'Return different session ID'
        id_ref = session_id
        # Pause session
        drm_manager.deactivate(True)
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id == id_ref, 'Return different session ID'
        # Start a new session
        drm_manager.activate()
        # Check a new session has been created with a new ID
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(session_id) == 16, 'No session ID is returned'
        assert session_id != id_ref, 'Return different session ID'
        # Close session
        drm_manager.deactivate()
        # Check session is closed
        status = drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(session_id) == 0, 'A session ID exists'
        async_cb.assert_NoError()
        print('Test session status on restart: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


@pytest.mark.long_run
@pytest.mark.hwtst
def test_license_expiration(accelize_drm, conf_json, cred_json, async_handler):
    """Test license expiration"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    try:
        print()

        # Test license expires after 2 duration periods when start/pause

        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        # Start
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        # Pause
        sleep(lic_duration/2)
        drm_manager.deactivate(True)
        # Check license is still running and activator are all unlocked
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Wait right before expiration
        wait_period = start + timedelta(seconds=2*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running and activators are all unlocked
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Wait a bit more time the expiration
        sleep(3)
        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.deactivate()
        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        print('Test license expires after 2 duration periods when start/pause/stop: PASS')

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

        # Test license does not expire after 3 duration periods when start/pause

        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        # Start
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        # Check license is running
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Wait 1 full duration period
        wait_period = start + timedelta(seconds=lic_duration+lic_duration/2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Pause
        drm_manager.deactivate(True)
        # Wait right before the next 2 duration periods expire
        wait_period = start + timedelta(seconds=3*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        # Wait a bit more time the expiration
        sleep(3)
        # Check license has expired
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        drm_manager.deactivate()
        # Check no license is running
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        print('Test license does not expire after 3 duration periods when start/pause: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


@pytest.mark.hwtst
def test_multiple_call(accelize_drm, conf_json, cred_json, async_handler):
    """Test multiple calls to activate and deactivate"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    try:
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
        # Resume
        drm_manager.activate(True)
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
        # Start again
        drm_manager.activate()
        # Check license is active
        assert drm_manager.get('license_status')
        # Check a session is valid
        session_id2 = drm_manager.get('session_id')
        assert len(session_id2) == 16
        assert session_id2 != session_id
        async_cb.assert_NoError()

        # Test multiple deactivate

        # Check license is active
        assert drm_manager.get('license_status')
        # Pause
        drm_manager.deactivate(True)
        # Check license is active
        assert drm_manager.get('license_status')
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        assert session_id == session_id2
        # Resume
        drm_manager.deactivate(True)
        # Check license is active
        assert drm_manager.get('license_status')
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        assert session_id == session_id2
        # Stop
        drm_manager.deactivate()
        # Check license is in active
        assert not drm_manager.get('license_status')
        # Check session ID is invalid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 0
        # Stop
        drm_manager.deactivate()
        # Check license is in active
        assert not drm_manager.get('license_status')
        # Check session ID is invalid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 0
        async_cb.assert_NoError()

    finally:
        if drm_manager:
            drm_manager.deactivate()


@pytest.mark.on_2_fpga
def test_retry_function(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test retry mechanism on API function (not including the retry in background thread)
    The retry is tested with one FPGA actiavted with a floating license and a 2nd FGPA
    that's requesting the same floating license but with a limit to 1 node.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]

    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_04')

    # Test no retry
    conf_json.reset()
    retry_period = 0
    conf_json['settings']['ws_retry_period_short'] = retry_period
    conf_json.save()
    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager1.activate()
        end = datetime.now()
        assert (end - start).total_seconds() < 1
        assert 'License Web Service error 470' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_04@accelize.com', str(excinfo.value)) is not None
        assert 'You have reached the maximum quantity of 1 seat(s) for floating entitlement' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        async_cb0.assert_NoError()
        async_cb1.assert_NoError()
    print('Test no retry: PASS')

    # Test 10s retry
    conf_json.reset()
    timeout = 10
    retry = 1
    conf_json['settings']['ws_request_timeout'] = timeout
    conf_json['settings']['ws_retry_period_short'] = retry
    conf_json.save()
    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager1.activate()
        end = datetime.now()
        m = search(r'Timeout on License request after (\d+) attempts', str(excinfo.value))
        assert m is not None
        assert int(m.group(1)) > 1
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        assert (end - start).total_seconds() >= timeout
        assert (end - start).total_seconds() <= timeout + 1
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        async_cb0.assert_NoError()
        async_cb1.assert_NoError()
    print('Test 10s retry: PASS')


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
    session_id = drm_manager1.get('session_id')
    assert len(session_id) == 0
    async_cb.assert_NoError()


@pytest.mark.endurance
def test_authentication_expiration(accelize_drm, conf_json, cred_json, async_handler):
    from random import sample
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    # Get test duration
    try:
        test_duration = accelize_drm.pytest_params['duration']
    except:
        test_duration = 14000
        print('Warning: Missing argument "duration". Using default value %d' % test_duration)

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    activators[0].generate_coin(1000)
    assert not drm_manager.get('license_status')
    activators[0].autotest(is_activated=False)
    drm_manager.activate()
    try:
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators[0].autotest(is_activated=True)
        activators[0].check_coin(drm_manager.get('metered_ta'))
        start = datetime.now()
        while True:
            assert drm_manager.get('license_status')
            activators[0].generate_coin(1)
            activators[0].check_coin(drm_manager.get('metered_data'))
            seconds_left = test_duration - (datetime.now() - start).total_seconds()
            print('Remaining time: %0.1fs  /  current coins=%d' % (seconds_left, activators[0].metering_data))
            if seconds_left < 0:
                break
            sleep(60)
    finally:
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators[0].autotest(is_activated=False)
        print('Endurance test has completed')


#@pytest.mark.skip(reason='TODO: fix a Python Segmentation Fault generated by latest versions of OS')
@pytest.mark.minimum
def test_curl_host_resolve(accelize_drm, conf_json, cred_json, async_handler):
    """Test host resolve information is taken into account by DRM Library"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.reset()
    url = conf_json['licensing']['url']
    conf_json['licensing']['host_resolves'] = {'%s:443' % url.replace('https://',''): '78.153.251.226'}
    conf_json.save()
    async_cb.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMExternFail) as excinfo:
        drm_manager.activate()
    assert 'SSL peer certificate or SSH remote key was not OK' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMExternFail.error_code
    async_cb.assert_NoError()
    del drm_manager
