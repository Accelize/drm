# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
from re import search
from json import loads
from glob import glob
from os.path import join

import pytest


@pytest.mark.no_parallel
def test_parameter_key_modification_with_get_set(accelize_drm, conf_json, cred_json,
                        async_handler, ws_admin):
    """Test accesses to parameter"""
    from os.path import isfile

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test parameter: license_type and drm_license_type in nodelocked and nodelocked_request_file
    # license_type: Read-only, return string with the license type: node-locked, floating/metering
    # drm_license_type: Read-only, return the license type of the DRM Controller: node-locked,
    #                   floating/metering
    # nodelocked_request_file: Read-only, return string with the path to the node-locked license
    #                          request JSON file.
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()
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
            lic_request_file = drm_manager.get('nodelocked_request_file')
            assert len(lic_request_file) > 0
            assert isfile(lic_request_file)
            with open(lic_request_file) as f:
                data = loads(f.read())
            assert data.get('device_id')
            assert data.get('drm_config')
            assert data['drm_config'].get('drm_type') == 1
            assert data['drm_config'].get('lgdn_version')
            assert data['drm_config'].get('metering_file')
            assert data['drm_config'].get('saas_challenge')
            assert data['drm_config'].get('vlnv_file')
            drm_manager.activate()
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        driver.program_fpga()


@pytest.mark.minimum
def test_nodelock_license_is_not_given_to_inactive_user(accelize_drm, conf_json, cred_json,
                                                        async_handler, ws_admin):
    """Test a user who has not bought a valid nodelocked license cannot get a license"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_01')
    conf_json.addNodelock()
    ws_admin.load(conf_json, cred_json)

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
def test_nodelock_normal_case(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()
    ws_admin.load(conf_json, cred_json)

    try:
        # Load a user who has a valid nodelock license
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            # Start application
            assert not drm_manager.get('license_status')
            activators.autotest()
            drm_manager.activate()
            activators.autotest()
            assert not drm_manager.get('license_status')
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            assert drm_manager.get('license_duration') == 0
            activators.check_coin(drm_manager.get('metered_data'))
            activators[0].generate_coin()
            activators.check_coin(drm_manager.get('metered_data'))
            drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        driver.program_fpga()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_reuse_existing_license(accelize_drm, conf_json, cred_json, async_handler,
                                         ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()
    activators.autotest()
    ws_admin.load(conf_json, cred_json)

    try:
        # Load a user who has a valid nodelock license
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            # Start application
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert not drm_manager.get('license_status')
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            assert drm_manager.get('license_duration') == 0
            # Stop application
            drm_manager.deactivate()
        async_cb.assert_NoError()
        # Recrete a new object with a bad url to verify it will reuse the existing license file
        conf_json['licensing']['url'] = "http://accelize.com"
        conf_json['settings']['log_verbosity'] = 1
        conf_json.save()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Node-Locked'
            # Start application
            assert not drm_manager.get('license_status')
            drm_manager.activate()
            assert not drm_manager.get('license_status')
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            assert drm_manager.get('license_duration') == 0
            # Stop application
            drm_manager.deactivate()
        async_cb.assert_NoError()

    finally:
        driver.program_fpga()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_without_server_access(accelize_drm, conf_json, cred_json, async_handler,
                                        ws_admin):
    """Test error is returned when no url is provided"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')  # User with a single nodelock license

    # Switch to nodelock
    conf_json.reset()
    conf_json.addNodelock()
    del conf_json['licensing']['url']
    conf_json.save()
    ws_admin.load(conf_json, cred_json)

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
            drm_manager.activate()
    assert search(r"Error with service configuration file .* Missing parameter 'url' of type String",
                  str(excinfo.value))
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, "Missing parameter 'url' of type String")
    async_cb.reset()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_without_malformed_license_file(accelize_drm, conf_json, cred_json, async_handler,
                                        ws_admin):
    """Test error is returned when the license file is malformed"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')  # User with a single nodelock license

    # Switch to nodelock
    conf_json.reset()
    conf_json.addNodelock()
    license_dir = conf_json['licensing']['license_dir']
    ws_admin.load(conf_json, cred_json)

    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            # Run a first time to create license file
            drm_manager.activate()
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

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()
    ws_admin.load(conf_json, cred_json)

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
            drm_manager0.activate()
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
    cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license
    logfile = log_file_factory.create(2, append=True)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set nodelock configuration
    conf_json.addNodelock()
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
            # Start application
            drm_manager.activate()
            assert drm_manager.get('drm_license_type') == 'Node-Locked'
            drm_manager.deactivate()

        # Set metering configuration
        conf_json.removeNodelock()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('license_type') == 'Floating/Metering'
            # Start application
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
        # Start application
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
    logfile.remove()


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_after_metering_mode(accelize_drm, conf_json, cred_json, async_handler,
                            ws_admin, log_file_factory):
    """Test metering session is stopped when switching to nodelock mode"""

    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license
    logfile = log_file_factory.create(1)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    ws_admin.load(conf_json, cred_json)

    # Set metering configuration
    drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
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
    conf_json.addNodelock()
    ws_admin.load(conf_json, cred_json)
    try:
        drm_manager = accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
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


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_parsing_of_nodelock_files(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test metering session is stopped when switching to nodelock mode"""

    from shutil import copyfile, move
    from os.path import isfile


    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')  # User with a single nodelock license
    conf_json.reset()
    conf_json.addNodelock()
    ws_admin.load(conf_json, cred_json)

    try:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            req_file_path = drm_manager.get('nodelocked_request_file')
            # Test nodelocked license request file not found
            req_file_path_bad = req_file_path + '.bad'
            move(req_file_path, req_file_path_bad)
            with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
                drm_manager.activate()
            assert 'Path is not a valid file' in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
            async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'Path is not a valid file')
            async_cb.reset()
            move(req_file_path_bad, req_file_path)

            # Test nodelocked license request file with bad format
            req_file_path_cpy = req_file_path + '.cpy'
            copyfile(req_file_path, req_file_path_cpy)
            with open(req_file_path_cpy) as fr:
                req_json = fr.read()
            with open(req_file_path, 'w') as fw:
                fw.write(req_json.replace(':', '='))
            with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
                drm_manager.activate()
            assert 'Cannot parse JSON string ' in str(excinfo.value)
            err_code = async_handler.get_error_code(str(excinfo.value))
            assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
            async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, 'Cannot parse JSON string')
            async_cb.reset()
            move(req_file_path_cpy, req_file_path)
    finally:
        driver.program_fpga()
