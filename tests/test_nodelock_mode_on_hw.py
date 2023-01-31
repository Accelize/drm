# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
from re import search
from json import loads
from glob import glob
from os.path import join, isfile
from re import search, findall, IGNORECASE
from shutil import copyfile, move

import pytest


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_request_file(accelize_drm, conf_json, cred_json, async_handler,
            ws_admin, log_file_factory):
    """Test request file behaviors when in nodelock mode"""
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-nodelock')  # User with a single nodelock license
    conf_json.reset()
    conf_json.addNodelock()
    logfile = log_file_factory.create(1, append=True)
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
            request_file = drm_manager.get('nodelocked_request_file')
            assert request_file
            assert isfile(request_file)

            # Test nodelocked request file content
            with open(request_file) as f:
                data = loads(f.read())
            assert data.get('device_id')
            assert data.get('diagnostic')
            assert data.get('drm_config')
            assert data['drm_config'].get('drm_type') == 1
            assert data['drm_config'].get('lgdn_version')
            assert data['drm_config'].get('metering_file')
            assert data['drm_config'].get('saas_challenge')
            assert data['drm_config'].get('vlnv_file')

            # Test nodelocked request file not found
            copy_request_file = request_file + '.copy'
            move(request_file, copy_request_file)
            with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
                drm_manager.activate()
            assert 'Path is not a valid file' in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
            async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'Path is not a valid file')
            async_cb.reset()

            # Test nodelocked request file with bad format
            with open(copy_request_file) as fr:
                req_json = fr.read()
            with open(request_file, 'w') as fw:
                fw.write(req_json.replace(':', '='))
            with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
                drm_manager.activate()
            assert 'Cannot parse JSON string ' in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
            async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, 'Cannot parse JSON string')
            async_cb.reset()
        log_content = logfile.read()
        assert search(r'Path is not a valid file', log_content, IGNORECASE)
        assert search(r'Cannot parse JSON string', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_license_file(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test license file behaviors in nodelock mode"""
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-nodelock')  # User with a single nodelock license
    conf_json.reset()
    conf_json.addNodelock()
    ws_admin.clean_user_db(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('license_status')
            drm_manager.deactivate()
            request_file = drm_manager.get('nodelocked_request_file')
            assert request_file
            assert isfile(request_file)
            license_file = request_file.replace('.req', '.lic')
            assert isfile(license_file)
            # Get DNA
            with open(request_file) as f:
                data = loads(f.read())
            device_id = data['device_id']

            # Test nodelocked license file content
            with open(license_file) as f:
                data = loads(f.read())
            assert data.get('id')
            assert data.get('drm_config')
            assert len(data['drm_config'].get('drm_session_id')) == 16
            assert data['drm_config'].get('health_period') == 0
            assert data['drm_config'].get('license')
            assert data['drm_config']['license'].get(device_id)
            assert search(r'[a-fA-F0-9]{16,}', data['drm_config']['license'][device_id].get('key'))
            assert not data['drm_config']['license'][device_id].get('timer')

            # Test nodelocked license file with bad format
            copy_license_file = license_file + '.copy'
            move(license_file, copy_license_file)
            with open(copy_license_file) as fr:
                req_json = fr.read()
            with open(license_file, 'w') as fw:
                fw.write(req_json.replace(':', '='))
            with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
                drm_manager.activate()
            assert 'Cannot parse JSON string ' in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
            async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, 'Cannot parse JSON string')
            async_cb.reset()
    finally:
        driver.program_fpga()


@pytest.mark.no_parallel
@pytest.mark.minimum
@pytest.mark.hwtst
def test_nodelock_reuse_existing_license(accelize_drm, conf_json, cred_json, async_handler,
                                         ws_admin, log_file_factory):
    """Test normal nodelock license usage and capability to reuse an already existing license file"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()

    cred_json.set_user('test-nodelock')
    conf_json.addNodelock()
    logfile = log_file_factory.create(1, append=True)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.clean_user_db(conf_json, cred_json)
    try:
        # Load a user who has a valid nodelock license
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('license_status')
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            assert drm_manager.get('license_duration') == 0
        async_cb.assert_NoError()
        # Recrete a new object with a bad url to verify it will reuse the existing license file
        conf_json.removeNodelock()
        conf_json['licensing']['url'] = "bad_url"
        conf_json.save()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            drm_manager.activate()
            assert drm_manager.get('license_status')
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            assert drm_manager.get('license_duration') == 0
        async_cb.assert_NoError()
        log_content = logfile.read()
        assert search(r'Looking for local node-locked license file', log_content, IGNORECASE)
        assert search(r'Could not find nodelocked license file', log_content, IGNORECASE)
        assert search(r'Cleared session ID', log_content, IGNORECASE)
        assert search(r'Parsed newly created node-locked License Request file', log_content, IGNORECASE)
        assert search(r'Requested and saved new node-locked license file', log_content, IGNORECASE)
        assert len(findall(r'Installed node-locked license successfully', log_content, IGNORECASE)) == 2
        assert search(r'Parsed existing node-locked License file', log_content, IGNORECASE)
        logfile.remove()
    finally:
        driver.program_fpga()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_with_malformed_license_file(accelize_drm, conf_json, cred_json, async_handler,
                                        ws_admin):
    """Test error is returned when the license file is malformed"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-nodelock')  # User with a single nodelock license

    # Switch to nodelock
    conf_json.reset()
    conf_json.addNodelock()
    license_dir = conf_json['licensing']['license_dir']
    ws_admin.clean_user_db(conf_json, cred_json)

    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            # Run a first time to create license file
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('license_status')
            license_file = glob(join(license_dir, '*.lic'))
            assert len(license_file) == 1
            license_file = license_file[0]
            # Corrupt license file
            with open(license_file, 'rt') as f:
                license_json = f.read()
            with open(license_file, 'wt') as f:
                f.write(license_json[:-1])
            # Run a second time after having corrupted the license file
            with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
                drm_manager.activate()
            assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFormat.error_code
            assert search(r'Invalid local license file', str(excinfo.value))
        async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, 'Invalid local license file')
        async_cb.reset()
    finally:
        driver.program_fpga()


@pytest.mark.on_2_fpga
@pytest.mark.minimum
@pytest.mark.hwtst
def test_nodelock_limits(accelize_drm, conf_json, conf_json_second, cred_json, async_handler, ws_admin):
    """
    Test behavior when limits are reached. 2 FPGA are required.
    """
    if len(accelize_drm.pytest_fpga_driver) < 2:
        pytest.skip('Skip test because 2 FPGA are needed but only 1 is found')

    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('test-nodelock')
    conf_json.addNodelock()
    ws_admin.clean_user_db(conf_json, cred_json)
    try:
        # Create the first instance
        async_cb0.reset()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver0.read_register_callback,
                    driver0.write_register_callback,
                    async_cb0.callback
                ) as drm_manager0:
            assert drm_manager0.get('license_type') == 'Node-Locked'
            # Consume the single token available
            assert not drm_manager.get('license_status')
            drm_manager0.activate()
            assert drm_manager.get('license_status')
            assert drm_manager0.get('drm_license_type') == 'Node-Locked'
            drm_manager0.deactivate()
            assert drm_manager0.get('drm_license_type') == 'Node-Locked'
        async_cb0.assert_NoError()
        # Create the second instance on the other FPGA
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver1.read_register_callback,
                    driver1.write_register_callback,
                    async_cb1.callback
                ) as drm_manager1:
            assert drm_manager1.get('license_type') == 'Node-Locked'
            with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
                drm_manager1.activate()
            assert 'Metering Web Service error 400' in str(excinfo.value)
            assert 'DRM WS request failed' in str(excinfo.value)
            assert search(r'"Entitlement Limit Reached.* with PT DRM Ref Design .+ for \S+_test_03@accelize.com', str(excinfo.value))
            assert 'You have reached the maximum quantity of 1 nodes for Nodelocked entitlement' in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb1.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'You have reached the maximum quantity')
        async_cb1.reset()

    finally:
        driver.program_fpga()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_metering_mode_is_blocked_after_nodelock_mode(accelize_drm, conf_json, cred_json,
                                  async_handler, ws_admin, log_file_factory):
    """
    Test we cannot switch to metering mode when nodelock is already set.
    Board needs to be reprogramed
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip('Nodelock to Metering license switch a actually supported on SoM target')
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-nodelock')        # User with a single nodelock license
    logfile = log_file_factory.create(1, append=True)
    conf_json['settings'].update(logfile.json)
    conf_json.addNodelock()     # Set nodelock configuration
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
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            assert drm_manager.get('license_status')
        # Set metering configuration
        cred_json.set_user('test-metering')
        conf_json.removeNodelock()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert not drm_manager.get('license_status')
            assert drm_manager.get('license_type') == 'Floating/Metering'
            with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
                drm_manager.activate()
            assert "DRM Controller is locked in Node-Locked licensing mode: " \
                   "To use other modes you must reprogram the FPGA device" in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMBadUsage.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMBadUsage.error_code, 'DRM Controller is locked in Node-Locked licensing mode')
        async_cb.reset()
    finally:
        # Reprogram FPGA
        driver.program_fpga()

    # Restart in metering
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
    log_content = logfile.read()
    assert search(r'Looking for local node-locked license file', log_content, IGNORECASE)
    assert search(r'Installed node-locked license successfully', log_content, IGNORECASE)
    logfile.remove()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_after_metering_mode(accelize_drm, conf_json, cred_json, async_handler,
                            ws_admin, log_file_factory):
    """Test metering session is stopped when switching to nodelock mode"""

    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('test-metering')        # User with a single nodelock license
    logfile = log_file_factory.create(1)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.clean_user_db(conf_json, cred_json)

    # Set metering configuration
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        assert not drm_manager.get('session_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('session_status')
        session_id = drm_manager.get('session_id')
        assert len(session_id) > 0
        activators[0].generate_coin()
        drm_manager.deactivate()    # Pause session
        assert not drm_manager.get('session_status')
        assert session_id != drm_manager.get('session_id')

    # Switch to nodelock
    cred_json.set_user('test-nodelock')
    conf_json.addNodelock()
    ws_admin.clean_user_db(conf_json, cred_json)
    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            assert session_id != drm_manager.get('session_id')
            assert not drm_manager.get('session_status')
            drm_manager.activate()
            assert not drm_manager.get('session_status')
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            drm_manager.deactivate()
            async_cb.assert_NoError()
            log_content = logfile.read()
            assert 'A floating/metering session is still pending: trying to close it gracefully before switching to nodelocked license' in log_content
            logfile.remove()
    finally:
        driver.program_fpga()


@pytest.mark.on_2_fpga
@pytest.mark.minimum
@pytest.mark.hwtst
def test_nodelock_is_board_specific(accelize_drm, conf_json, cred_json,
                                  async_handler, ws_admin, log_file_factory):
    """
    Test a nodelock license can work only on one board.
    Using an existing nodelock license on another board returns an error.
    """
    if len(accelize_drm.pytest_fpga_driver) < 2:
        pytest.skip('Skip test because 2 FPGA are needed but only 1 is found')

    if accelize_drm.is_ctrl_sw:
        pytest.skip('Nodelock to Metering license switch a actually supported on SoM target')

    async_cb = async_handler.create()
    cred_json.set_user('test-nodelock')        # User with a single nodelock license
    logfile = log_file_factory.create(1, append=True)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set nodelock configuration
    conf_json.addNodelock()
    ws_admin.clean_user_db(conf_json, cred_json)

    # Using nodelock license on a board
    try:
        driver = accelize_drm.pytest_fpga_driver[0]
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            request_file_0 = drm_manager.get('nodelocked_request_file')
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert drm_manager.get('license_status')
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
    finally:
        # Reprogram FPGA
        driver.program_fpga()

    # Using the same nodelock license for another board
    try:
        driver = accelize_drm.pytest_fpga_driver[1]
        conf_json['licensing']['url'] = "bad_http" # provide bad url to be sure no HTTP request is sent
        conf_json.save()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            request_file_1 = drm_manager.get('nodelocked_request_file')
            assert request_file_0 == request_file_1
            with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
                drm_manager.activate()
            assert "DRM Controller is locked in Node-Locked licensing mode: " \
                   "To use other modes you must reprogram the FPGA device" in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMCtlrError.error_code
        async_cb.assert_Error(accelize_drm.exceptions.DRMCtlrError.error_code, 'DRM Controller is locked in Node-Locked licensing mode')
        async_cb.reset()
    finally:
        # Reprogram FPGA
        driver.program_fpga()

    log_content = logfile.read()
    assert search(r'Looking for local node-locked license file', log_content, IGNORECASE)
    assert search(r'Installed node-locked license successfully', log_content, IGNORECASE)
    logfile.remove()

