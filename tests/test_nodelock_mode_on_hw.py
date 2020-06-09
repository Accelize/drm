# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
from re import search
from json import loads

import pytest


@pytest.mark.no_parallel
def test_parameter_key_modification_with_get_set(accelize_drm, conf_json, cred_json, async_handler,
                                                 ws_admin):
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
    accelize_drm.clean_nodelock_env(driver=driver, conf_json=conf_json, cred_json=cred_json,
                                    ws_admin=ws_admin)
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert drm_manager.get('license_type') == 'Node-Locked'
        lic_request_file = drm_manager.get('nodelocked_request_file')
        assert len(lic_request_file) > 0
        assert isfile(lic_request_file)
        with open(lic_request_file) as f:
            data = loads(f.read())
        mandatory_fields = ['dna', 'drmlibVersion', 'lgdnVersion', 'meteringFile', 'request',
                            'saasChallenge', 'vlnvFile', 'product']
        assert all([e in data.keys() for e in mandatory_fields])
        assert data['request'] == 'open'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(drm_manager, driver, conf_json, cred_json, ws_admin)


@pytest.mark.minimum
def test_nodelock_license_is_not_given_to_inactive_user(accelize_drm, conf_json, cred_json,
                                                        async_handler, ws_admin):
    """Test a user who has not bought a valid nodelocked license cannot get a license"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_01')
    conf_json.addNodelock()

    try:
        # Start application
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert not drm_manager.get('license_status')
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Metering Web Service error 400' in str(excinfo.value)
        assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
        assert 'User account has no entitlement. Purchase additional licenses via your portal.' in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)


@pytest.mark.minimum
@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_normal_case(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test normal nodelock license usage"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators[0].autotest()

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()

    try:
        # Load a user who has a valid nodelock license
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        assert not drm_manager.get('license_status')
        activators[0].autotest()
        drm_manager.activate()
        activators[0].autotest()
        assert not drm_manager.get('license_status')
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        assert drm_manager.get('license_duration') == 0
        activators[0].check_coin(drm_manager.get('metered_data'))
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)


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
    activators[0].autotest()

    try:
        # Load a user who has a valid nodelock license
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert not drm_manager.get('license_status')
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        assert drm_manager.get('license_duration') == 0
        activators[0].check_coin(drm_manager.get('metered_data'))
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Stop application
        drm_manager.deactivate()
        async_cb.assert_NoError()
        # Recrete a new object with a bad url to verify it will reuse the existing license file
        conf_json['licensing']['url'] = "http://accelize.com"
        conf_json['settings']['log_verbosity'] = 1
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert not drm_manager.get('license_status')
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        assert drm_manager.get('license_duration') == 0
        activators[0].generate_coin(10)
        activators[0].check_coin(drm_manager.get('metered_data'))
        # Stop application
        drm_manager.deactivate()
        async_cb.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)


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
    accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager.activate()
    assert search(r"Error with service configuration file .* Missing parameter 'url' of type String",
                  str(excinfo.value))
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_NoError()


@pytest.mark.on_2_fpga
@pytest.mark.minimum
@pytest.mark.hwtst
def test_nodelock_limits(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """
    Test behavior when limits are reached. 2 FPGA are required.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]
    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_03')
    conf_json.addNodelock()

    try:

        # Create the first instance
        async_cb0.reset()
        accelize_drm.clean_nodelock_env(None, driver0, conf_json, cred_json, ws_admin)
        drm_manager0 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver0.read_register_callback,
            driver0.write_register_callback,
            async_cb0.callback
        )
        assert drm_manager0.get('license_type') == 'Node-Locked'

        # Consume the single token available
        drm_manager0.activate()
        assert drm_manager0.get('drm_license_type') == 'Node-Locked'
        drm_manager0.deactivate()
        assert drm_manager0.get('drm_license_type') == 'Node-Locked'
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
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager1.activate()
        assert 'Metering Web Service error 400' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_03@accelize.com', str(excinfo.value))
        assert 'You have reached the maximum quantity of 1 nodes for Nodelocked entitlement' in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb1.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_env(None, driver0, conf_json, cred_json, ws_admin)


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_metering_mode_is_blocked_after_nodelock_mode(accelize_drm, conf_json, cred_json,
                                                      async_handler, ws_admin):
    """
    Test we cannot switch to metering mode when nodelock is already set.
    Board needs to be reprogramed
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license

    try:
        # Set nodelock configuration
        conf_json.addNodelock()
        accelize_drm.clean_nodelock_env(conf_json=conf_json, cred_json=cred_json, ws_admin=ws_admin)
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        drm_manager.deactivate()

        # Set metering configuration
        conf_json.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Floating/Metering'
        # Start application
        with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
            drm_manager.activate()
        assert "DRM Controller is locked in Node-Locked licensing mode: " \
               "To use other modes you must reprogram the FPGA device" in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMBadUsage.error_code
        async_cb.assert_NoError()

        # Reprogram FPGA
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)

        # Restart in metering
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        # Start application
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        assert drm_manager.get('license_status')
        assert drm_manager.get('license_type') == 'Floating/Metering'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        async_cb.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_nodelock_after_metering_mode(accelize_drm, conf_json, cred_json, async_handler, ws_admin):
    """Test metering session is stopped when switching to nodelock mode"""

    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_03')        # User with a single nodelock license

    try:
        accelize_drm.clean_nodelock_env(driver=driver, conf_json=conf_json, cred_json=cred_json,
                                        ws_admin=ws_admin)
        # Set metering configuration
        conf_json.reset()
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
        activators[0].generate_coin(15)
        drm_manager.deactivate(True)    # Pause session
        assert drm_manager.get('session_status')
        assert session_id == drm_manager.get('session_id')

        # Switch to nodelock
        conf_json.reset()
        conf_json.addNodelock()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert session_id != drm_manager.get('session_id')
        assert not drm_manager.get('session_status')
        drm_manager.activate()
        assert not drm_manager.get('session_status')
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        drm_manager.deactivate()
        async_cb.assert_NoError()

    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)


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

    try:
        print()

        accelize_drm.clean_nodelock_env(driver=driver, conf_json=conf_json, cred_json=cred_json,
                                        ws_admin=ws_admin)
        conf_json.reset()
        conf_json.addNodelock()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        req_file_path = drm_manager.get('nodelocked_request_file')
        # Test nodelocked license request file not found
        req_file_path_bad = req_file_path + '.bad'
        move(req_file_path, req_file_path_bad)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.activate()
        assert 'Cannot find JSON file' in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
        async_cb.assert_NoError()
        move(req_file_path_bad, req_file_path)
        print('Test nodelocked license request file not found: PASS')

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
        async_cb.assert_NoError()
        move(req_file_path_cpy, req_file_path)
        print('Test nodelocked license request file with bad format: PASS')

    finally:
        accelize_drm.clean_nodelock_env(None, driver, conf_json, cred_json, ws_admin)
