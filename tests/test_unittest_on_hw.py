# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from re import search, finditer
from time import sleep, time
from json import loads
from datetime import datetime, timedelta


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
               'log_file',
               'log_verbosity',
               'log_format',
               'frequency_detection_threshold',
               'frequency_detection_period',
               'custom_field',
               'mailbox_data',
               'ws_retry_deadline',
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


def ordered_json(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered_json(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered_json(x) for x in obj)
    else:
        return obj


@pytest.mark.minimum
def test_backward_compatibility(accelize_drm, conf_json, cred_json, async_handler):
    """Test API is not compatible with DRM HDK < 3.0"""
    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    drm_manager = None

    try:
        # Program FPGA with old HDK 2.x.x
        hdk = list(filter(lambda x: x.startswith('2.'), refdesign.hdk_versions))[0]
        assert hdk.startswith('2.')
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
        assert 'Failed to initialize DRM Controller' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
        async_cb.assert_NoError()

        # Program FPGA with old HDK 3.0.x
        hdk = list(filter(lambda x: x.startswith('3.0'), refdesign.hdk_versions))[0]
        assert hdk.startswith('3.0.')
        image_id = refdesign.get_image_id(hdk)
        driver.program_fpga(image_id)
        # Test compatibility issue
        async_cb = async_handler.create()
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            async_cb.reset()
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert search(r'This DRM Library version .* is not compatible with the DRM HDK version .*',
                      str(excinfo.value)) is not None
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
        async_cb.assert_NoError()

    finally:
        if drm_manager:
            del drm_manager
        # Reprogram FPGA with original image
        fpga_image = driver.fpga_image if driver.fpga_image is not None \
            else refdesign.get_image_id()
        driver.program_fpga(fpga_image)


@pytest.mark.minimum
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('license_type') == 'Floating/Metering'
    assert drm_manager.get('drm_license_type') == 'Floating/Metering'
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert "No valid entitlement found for your account." in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_NoError()
    # Request nodelock license
    try:
        async_cb.reset()
        cred_json.set_user('accelize_accelerator_test_01')
        conf_json.reset()
        conf_json.addNodelock()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "No valid entitlement found for your account." in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('# Test user-01 entitlements: PASS')

    # Test user-02 entitlements
    # Request metering license
    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
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
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
#        assert "No valid entitlement found for your account." in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('# Test user-02 entitlements: PASS')

    # Test user-03 entitlements
    # Request metering license
    cred_json.set_user('accelize_accelerator_test_03')
    async_cb.reset()
    conf_json.reset()
    accelize_drm.clean_metering_env(cred_json, ws_admin)
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
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
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        # Start application
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        drm_manager.activate()
        assert drm_manager.get('drm_license_type') == 'Node-Locked'
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(drm_manager, driver, conf_json, cred_json, ws_admin)
    print('# Test user-03 entitlements: PASS')

    # Test user-04 entitlements
    # Request metering license
    cred_json.set_user('accelize_accelerator_test_04')
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
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
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('license_type') == 'Node-Locked'
        assert drm_manager.get('drm_license_type') == 'Floating/Metering'
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
#        assert "No valid entitlement found for your account." in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-04 entitlements: PASS')


@pytest.mark.minimum
def test_parameter_key_modification_with_config_file(accelize_drm, conf_json, cred_json,
                                                     async_handler):
    """Test accesses to parameter"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # First get all default value for all tested parameters
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    orig_log_verbosity = drm_manager.get('log_verbosity')
    orig_log_format = drm_manager.get('log_format')
    orig_frequency_mhz = drm_manager.get('drm_frequency')
    orig_frequency_detect_period = drm_manager.get('frequency_detection_period')
    orig_frequency_detect_threshold = drm_manager.get('frequency_detection_threshold')
    orig_retry_deadline = drm_manager.get('ws_retry_deadline')
    orig_retry_period_long = drm_manager.get('ws_retry_period_long')
    orig_retry_period_short = drm_manager.get('ws_retry_period_short')
    orig_response_timeout = drm_manager.get('ws_request_timeout')

    # Test parameter: log_verbosity
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    from random import choice
    async_cb.reset()
    conf_json.reset()
    log_level_choice = list(range(0,6))
    log_level_choice.remove(orig_log_verbosity)
    exp_value = choice(log_level_choice)
    assert exp_value != orig_log_verbosity
    conf_json['settings']['log_verbosity'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('log_verbosity')
    assert value == exp_value
    drm_manager.set(log_verbosity=orig_log_verbosity)
    print("Test parameter 'log_verbosity': PASS")

    # Test parameter: log_format
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    async_cb.reset()
    conf_json.reset()
    exp_value = 0 if orig_log_format else 1
    conf_json['settings']['log_format'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('log_format')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_format': PASS")

#    # Test parameter: log_file
#    # Read-only, read the logging file path: null=stdout, any string=path to file.
#    # Can be set only from configuration file
#    from time import time
#    from os.path import isfile
#    async_cb.reset()
#    conf_json.reset()
#    expLogPath = "log_%s.txt" % time()
#    conf_json['settings']['log_file'] = expLogPath
#    conf_json.save()
#    drm_manager = accelize_drm.DrmManager(
#        conf_json.path,
#        cred_json.path,
#        driver.read_register_callback,
#        driver.write_register_callback,
#        async_cb.callback
#    )
#    logPath = drm_manager.get('log_file')
#    assert logPath == expLogPath
#    drm_manager.set(log_message_level=1)
#    msg = 'This should be ERROR message'
#    drm_manager.set(log_message=msg)
#    assert isfile(logPath)
#    with open(logPath) as f:
#        log_content = f.read()
#    assert "ERROR" in log_content
#    assert msg in log_content
#    assert_NoErrorCallback(async_cb)
#    print("Test parameter 'log_file': PASS")

    # Test parameter: drm_frequency
    # Read-only, return the measured DRM frequency
    async_cb.reset()
    conf_json.reset()
    exp_value = 2*orig_frequency_mhz
    conf_json['drm']['frequency_mhz'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('drm_frequency')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'frequency_mhz': PASS")

    # Test parameter: frequency_detection_period
    # Read-write: read and write the period of time in milliseconds used to measure the
    # real DRM Controller frequency
    async_cb.reset()
    conf_json.reset()
    exp_value = 2*orig_frequency_detect_period
    conf_json['settings'] = {'frequency_detection_period': exp_value}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('frequency_detection_period')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'frequency_detection_period': PASS")

    # Test parameter: frequency_detection_threshold
    # Read-write: read and write frequency gap threshold (in percentage) used to measure
    # the real DRM Controller frequency
    async_cb.reset()
    conf_json.reset()
    exp_value = 2*orig_frequency_detect_threshold
    conf_json['settings'] = {'frequency_detection_threshold': exp_value}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('frequency_detection_threshold')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'frequency_detection_threshold': PASS")

    # Test parameter: ws_retry_deadline
    # Read-write: read and write the retry period deadline in seconds from the license
    # timeout during which no more retry is sent
    async_cb.reset()
    conf_json.reset()
    exp_value = 2*orig_retry_deadline
    conf_json['settings'] = {'ws_retry_deadline': exp_value}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('ws_retry_deadline')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'ws_retry_deadline': PASS")

    # Test parameter: ws_retry_period_long
    # Read-write: read and write the time in seconds before the next request attempt to
    # the Web Server when the time left before timeout is large
    async_cb.reset()
    conf_json.reset()
    # Check error: ws_retry_period_long must be != ws_retry_period_short
    conf_json['settings'] = {'ws_retry_period_long': orig_retry_period_short}
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'ws_retry_period_long .+ must be greater than ws_retry_period_short .+',
                  str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()

    async_cb.reset()
    conf_json.reset()
    exp_value = orig_retry_period_long + 1
    conf_json['settings'] = {'ws_retry_period_long': exp_value}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('ws_retry_period_long')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'ws_retry_period_long': PASS")

    # Test parameter: ws_retry_period_short
    # Read-write: read and write the time in seconds before the next request attempt to
    # the Web Server when the time left before timeout is short
    async_cb.reset()
    conf_json.reset()
    # Check error: ws_retry_period_long must be != ws_retry_period_short
    conf_json['settings'] = {'ws_retry_period_short': orig_retry_period_long}
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'ws_retry_period_long .+ must be greater than ws_retry_period_short .+',
                  str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()

    async_cb.reset()
    conf_json.reset()
    exp_value = orig_retry_period_short + 1
    conf_json['settings'] = {'ws_retry_period_short': exp_value}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('ws_retry_period_short')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'ws_retry_period_short': PASS")

    # Test parameter: ws_request_timeout
    # Read-write: read and write the web service request timeout in seconds during which
    # the response is waited
    async_cb.reset()
    conf_json.reset()
    conf_json['settings'] = {'ws_request_timeout': 0}
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "ws_request_timeout must not be 0" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code

    async_cb.reset()
    conf_json.reset()
    exp_value = 2*orig_response_timeout
    conf_json['settings'] = {'ws_request_timeout': exp_value}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('ws_request_timeout')
    assert value == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'ws_request_timeout': PASS")

    # Test unsupported parameter
    # Verify the parameter is simply ignored
    async_cb.reset()
    conf_json.reset()
    conf_json['settings'] = {'unsupported_param': 10.2}
    conf_json.save()
    accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    async_cb.assert_NoError()
    print("Test unsupported parameter: PASS")

    # Test empty parameter
    # Verify the parameter is simply ignored
    async_cb.reset()
    conf_json.reset()
    conf_json['settings'] = {'': 10.2}
    conf_json.save()
    accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    async_cb.assert_NoError()
    print("Test empty parameter: PASS")


def test_parameter_key_modification_with_get_set(accelize_drm, conf_json, cred_json, async_handler,
                                                 ws_admin):
    """Test accesses to parameter"""
    from os.path import isfile

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activator = accelize_drm.pytest_fpga_activators[0][0]

    print()

    # Test with a nodelocked user

    # Test parameter: license_type and drm_license_type in nodelocked and nodelocked_request_file
    # => Done in test_nodelock_mode_on_hw

    # Test with a floating/metered user

    async_cb.reset()
    cred_json.set_user('accelize_accelerator_test_02')
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Test when parameter is a list
    value = drm_manager.get('num_activators','license_type')
    assert isinstance(value, dict)
    assert value['num_activators'] == 1
    assert value['license_type'] == 'Floating/Metering'
    async_cb.assert_NoError()
    print("Test when parameter is a list: PASS")


    # Test parameter: log_verbosity
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    orig_verbosity = drm_manager.get('log_verbosity')
    new_verbosity = 4 if orig_verbosity == 5 else 5
    drm_manager.set(log_verbosity=new_verbosity)
    assert drm_manager.get('log_verbosity') == new_verbosity
    drm_manager.set(log_verbosity=orig_verbosity)
    async_cb.assert_NoError()
    print("Test parameter 'log_verbosity': PASS")

    # Test parameter: log_format
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    format = drm_manager.get('log_format')
    exp_format = 0 if format else 1
    drm_manager.set(log_format=exp_format)
    assert drm_manager.get('log_format') == exp_format
    async_cb.assert_NoError()
    print("Test parameter 'log_format': PASS")

    # Test parameter: license_type in metering
    # Read-only, return string with the license type: node-locked, floating/metering
    assert drm_manager.get('license_type') == 'Floating/Metering'
    async_cb.assert_NoError()
    print("Test parameter 'license_type' in Metered: PASS")

    # Test parameter: drm_license_type, license_duration
    # drm_license_type: Read-only, return the license type of the DRM Controller: node-locked, floating/metering
    # license_duration: Read-only, return uint32 with the duration in seconds of the current or last license
    drm_manager.activate()
    assert drm_manager.get('drm_license_type') == 'Floating/Metering'
    assert drm_manager.get('license_duration') != 0
    drm_manager.deactivate()
    async_cb.assert_NoError()
    print("Test parameter 'drm_license_type', 'license_duration': PASS")

    # Test parameter: num_activators
    # Read-only, return uint32_t/string with the number of activators detected by the DRM controller
    nb_activator = drm_manager.get('num_activators')
    assert nb_activator == 1, 'Unexpected number of activators'
    print("Test parameter 'num_activators': PASS")

    # Test parameter: session_id
    # Read-only, return string with the current session ID
    drm_manager.activate()
    sessionId = drm_manager.get('session_id')
    assert len(sessionId) == 16, 'Unexpected length of session ID'
    drm_manager.deactivate()
    async_cb.assert_NoError()
    print("Test parameter 'session_id': PASS")

    # Test parameter: session_status
    # Read-only, return boolean to indicate if a session is currently running
    session_state = drm_manager.get('session_status')
    assert not session_state
    drm_manager.activate()
    session_state = drm_manager.get('session_status')
    assert session_state
    drm_manager.deactivate()
    session_state = drm_manager.get('session_status')
    assert not session_state
    async_cb.assert_NoError()
    print("Test parameter 'session_status': PASS")

    # Test parameter: license_status
    # Read-only, return the current license status
    assert not drm_manager.get('license_status')
    drm_manager.activate()
    assert drm_manager.get('license_status')
    drm_manager.deactivate()
    assert not drm_manager.get('license_status')
    async_cb.assert_NoError()
    print("Test parameter 'license_status': PASS")

    # Test parameter: metered_data
    # Read-only, return uint64_t or string with the current value of the metering data counter
    drm_manager.activate()
    orig_coins = drm_manager.get('metered_data')
    new_coins = 10
    activator.generate_coin(new_coins)
    coins = drm_manager.get('metered_data')
    assert coins == orig_coins + new_coins
    async_cb.assert_NoError()
    drm_manager.deactivate()

    print("Test parameter 'metered_data': PASS")

    # Test parameter: page_ctrlreg
    # Read-only, return nothing, print all registers in the DRM Controller Registry page
    page = drm_manager.get('page_ctrlreg')
    assert search(r'Register\s+@0x00:\s+0x00000000', page), 'Unexpected content of page_ctrlreg'
    print("Test parameter 'page_ctrlreg': PASS")

    # Test parameter: page_vlnvfile
    # Read-only, return nothing, print all registers in the VLNV File page
    page = drm_manager.get('page_vlnvfile')
    assert search(r'Register\s+@0x00:\s+0x00000001', page), 'Unexpected content of page_vlnvfile'
    print("Test parameter 'page_vlnvfile': PASS")

    # Test parameter: page_licfile
    # Read-only, return nothing, print all registers in the License File page
    page = drm_manager.get('page_licfile')
    assert search(r'Register\s+@0x00:\s+0x00000002', page), 'Unexpected content of page_licfile'
    print("Test parameter 'page_licfile': PASS")

    # Test parameter: page_tracefile
    # Read-only, return nothing, print all registers in the Trace File page
    page = drm_manager.get('page_tracefile')
    assert search(r'Register\s+@0x00:\s+0x00000003', page), 'Unexpected content of page_tracefile'
    print("Test parameter 'page_tracefile': PASS")

    # Test parameter: page_meteringfile
    # Read-only, return nothing, print all registers in the Metering File page
    page = drm_manager.get('page_meteringfile')
    assert search(r'Register\s+@0x00:\s+0x00000004', page), 'Unexpected content of page_meteringfile'
    print("Test parameter 'page_meteringfile': PASS")

    # Test parameter: page_mailbox
    # Read-only, return nothing, print all registers in the Mailbox page
    page = drm_manager.get('page_mailbox')
    assert search(r'Register\s+@0x00:\s+0x00000005', page), 'Unexpected content of page_mailbox'
    print("Test parameter 'page_mailbox': PASS")

    # Test parameter: hw_report
    # Read-only, return nothing, print the Algodone HW report
    hw_report = drm_manager.get('hw_report')
    nb_lines = len(tuple(finditer(r'\n', hw_report)))
    assert nb_lines > 10, 'Unexpected HW report content'
    print("Test parameter 'hw_report': PASS")

    # Test parameter: frequency_detection_threshold
    # Read-write: read and write frequency gap threshold (in percentage) used to measure the real DRM Controller frequency
    orig_freq_threhsold = drm_manager.get('frequency_detection_threshold')    # Save original threshold
    exp_freq_threhsold = orig_freq_threhsold * 2
    drm_manager.set(frequency_detection_threshold=exp_freq_threhsold)
    new_freq_threhsold = drm_manager.get('frequency_detection_threshold')
    assert new_freq_threhsold == exp_freq_threhsold, 'Unexpected frequency dectection threshold percentage'
    drm_manager.set(frequency_detection_threshold=orig_freq_threhsold)    # Restore original threshold
    print("Test parameter 'frequency_detection_threshold': PASS")

    # Test parameter: frequency_detection_period
    # Read-write: read and write the period of time in milliseconds used to measure the real DRM Controller frequency
    orig_freq_period = drm_manager.get('frequency_detection_period')    # Save original period
    exp_freq_period = orig_freq_period * 2
    drm_manager.set(frequency_detection_period=exp_freq_period)
    new_freq_period = drm_manager.get('frequency_detection_period')
    assert new_freq_period == exp_freq_period, 'Unexpected frequency dectection period'
    drm_manager.set(frequency_detection_period=orig_freq_period)    # Restore original period
    print("Test parameter 'frequency_detection_period': PASS")

    # Test parameter: drm_frequency
    # Read-only, return the measured DRM frequency
    freq_period = drm_manager.get('frequency_detection_period')    # Save original period
    drm_manager.activate()
    sleep(2.0*freq_period/1000)
    freq_drm = drm_manager.get('drm_frequency')
    drm_manager.deactivate()
    assert freq_drm == 125, 'Unexpected frequency gap threshold'
    print("Test parameter 'drm_frequency': PASS")

    # Test parameter: product_info
    # Read-only, return the product information
    from pprint import pformat
    product_id = pformat(drm_manager.get('product_info'))
    exp_product_id = pformat(loads("""{
            "vendor": "accelize.com",
            "library": "refdesign",
            "name": "drm_1activator",
            "sign":"31f4b20548d39d9cc8895ec7a52e68f0"
        }"""))
    assert product_id == exp_product_id, 'Unexpected product ID'
    print("Test parameter 'product_info': PASS")

    # Test parameter: mailbox_size
    # Read-only, return the size of the Mailbox read-write memory in DRM Controller
    mailbox_size = drm_manager.get('mailbox_size')
    assert mailbox_size == 14, 'Unexpected Mailbox size'
    print("Test parameter 'mailbox_size': PASS")

    # Test parameter: token_string, token_validity and token_time_left
    # Read-only, return the token string
    # Read-only, return the validity in seconds of the current token as provided by the WOAuth2 WS
    # Read-only, return the number of seconds left until the current token expires
    drm_manager.activate()
    token_time_left = drm_manager.get('token_time_left')
    if token_time_left < 15:
        drm_manager.deactivate()
        sleep(16)
        drm_manager.activate()
    token_string = drm_manager.get('token_string')
    assert len(token_string) > 0
    token_validity = drm_manager.get('token_validity')
    assert token_validity > 15
    token_time_left = drm_manager.get('token_time_left')
    sleep(2)
    assert 2 <= token_time_left - drm_manager.get('token_time_left') <= 3
    assert drm_manager.get('token_validity') == token_validity
    assert token_string == drm_manager.get('token_string')
    drm_manager.deactivate()
    print("Test parameter 'token_string', 'token_validity' and 'token_time_left': PASS")

    # Test parameter: list_all
    # Read-only, list all parameter keys
    list_param = drm_manager.get('list_all')
    assert isinstance(list_param , list)
    assert len(list_param) == len(_PARAM_LIST)
    assert all(key in _PARAM_LIST for key in list_param)
    print("Test parameter 'list_all': PASS")

    # Test parameter: dump_all
    # Read-only, read all parameter key values
    dump_param = drm_manager.get('dump_all')
    assert isinstance(dump_param, dict)
    assert len(dump_param) == _PARAM_LIST.index('dump_all')
    assert all(key in _PARAM_LIST for key in dump_param.keys())
    print("Test parameter 'dump_all': PASS")

    # Test parameter: custom_field
    # Read-write: only for testing, any uint32_t register accessible to the user for any purpose.
    from random import randint
    val_exp = randint(0,0xFFFFFFFF)
    val_init = drm_manager.get('custom_field')
    assert val_exp != val_init
    drm_manager.set(custom_field=val_exp)
    assert drm_manager.get('custom_field') == val_exp
    print("Test parameter 'custom_field': PASS")

# NEED LGDN TO FIX THE ISSUE
    # Test parameter: mailbox_data
    # Read-write: only for testing, read or write values to Mailbox read-write memory in
    # DRM Controller
    from random import sample
    mailbox_size = drm_manager.get('mailbox_size')
    wr_msg = sample(range(0xFFFFFFFF), mailbox_size - 3)
    drm_manager.set(mailbox_data=wr_msg)
    rd_msg = drm_manager.get('mailbox_data')
    assert type(rd_msg) == type(wr_msg) == list, 'Bad type returned by get(mailbox_data)'
    assert rd_msg[:len(wr_msg)] == wr_msg, 'Failed to write-read Mailbox'
    print("Test parameter 'mailbox_data': PASS")

    # Test parameter: ws_retry_deadline
    # Read-write: read and write the retry period deadline in seconds from the license
    # timeout during which no more retry is sent
    orig_retry_deadline = drm_manager.get('ws_retry_deadline')  # Save original value
    exp_value = orig_retry_deadline + 100
    drm_manager.set(ws_retry_deadline=exp_value)
    assert drm_manager.get('ws_retry_deadline') == exp_value
    drm_manager.set(ws_retry_deadline=orig_retry_deadline)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_retry_deadline': PASS")

    # Test parameter: ws_retry_period_long
    # Read-write: read and write the time in seconds before the next request attempt to
    # the Web Server when the time left before timeout is large
    orig_retry_period_long = drm_manager.get('ws_retry_period_long')  # Save original value
    orig_retry_period_short = drm_manager.get('ws_retry_period_short')
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(ws_retry_period_long=orig_retry_period_short)
    assert search(r'ws_retry_period_long .+ must be greater than ws_retry_period_short .+',
                  str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    exp_value = orig_retry_period_long + 1
    drm_manager.set(ws_retry_period_long=exp_value)
    assert drm_manager.get('ws_retry_period_long') == exp_value
    drm_manager.set(ws_retry_period_long=orig_retry_period_long)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_retry_period_long': PASS")

    # Test parameter: ws_retry_period_short
    # Read-write: read and write the time in seconds before the next request attempt to the
    # Web Server when the time left before timeout is short
    orig_retry_period_short = drm_manager.get('ws_retry_period_short')  # Save original value
    orig_retry_period_long = drm_manager.get('ws_retry_period_long')
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(ws_retry_period_short=orig_retry_period_long)
    assert search(r'ws_retry_period_long .+ must be greater than ws_retry_period_short .+',
                  str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    exp_value = orig_retry_period_short + 1
    drm_manager.set(ws_retry_period_short=exp_value)
    assert drm_manager.get('ws_retry_period_short') == exp_value
    drm_manager.set(ws_retry_period_short=orig_retry_period_short)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_retry_period_short': PASS")

    # Test parameter: ws_request_timeout
    # Read-write: read and write the web service request timeout in seconds during which
    # the response is waited
    orig_response_timeout = drm_manager.get('ws_request_timeout')  # Save original value
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(ws_request_timeout=0)
    assert "ws_request_timeout must not be 0" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    exp_value = orig_response_timeout + 100
    drm_manager.set(ws_request_timeout=exp_value)
    assert drm_manager.get('ws_request_timeout') == exp_value
    drm_manager.set(ws_request_timeout=orig_response_timeout)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_request_timeout': PASS")

    # Test parameter: log_message_level
    # Read-write, only for testing, read and write the log level used with log_message parameter
    # to set the message level
    level = drm_manager.get('log_message_level')
    exp_level = 5 if level!=5 else 4
    drm_manager.set(log_message_level=exp_level)
    assert drm_manager.get('log_message_level') == exp_level
    async_cb.assert_NoError()
    print("Test parameter 'log_message_level': PASS")

    # Test parameter: trigger_async_callback
    # Write-only: only for testing, call the asynchronous error callback with the given message.
    drm_manager.activate()
    test_message = 'Test message'
    drm_manager.set(trigger_async_callback=test_message)
    assert async_cb.was_called, 'Asynchronous callback has not been called.'
    assert async_cb.message is not None, 'Asynchronous callback did not report any message'
    assert test_message in async_cb.message, 'Asynchronous callback has not received the correct message'
    assert async_cb.errcode == accelize_drm.exceptions.DRMDebug.error_code, \
        'Asynchronous callback has not received the correct error code'
    drm_manager.deactivate()
    async_cb.reset()
    print("Test parameter 'trigger_async_callback': PASS")

    # Test parameter: bad_product_id
    # Write-only: only for testing, uses a bad product ID.
    # => Skipped: Tested in test_configuration_file_bad_product_id

    # Test parameter: bad_oauth2_token
    # Write-only: only for testing, uses a bad OAuth2 token.
    # => Skipped: Tested in test_configuration_file_with_bad_authentication

#    # Test parameter: log_message
#    # Write-only, only for testing, insert a message with the value as content
#    from time import time
#    from os.path import isfile
#    async_cb.reset()
#    conf_json.reset()
#    logpath = "log_%s.txt" % time()
#    conf_json['settings']['log_file'] = logpath
#    conf_json.save()
#    drm_manager = accelize_drm.DrmManager(
#        conf_json.path,
#        cred_json.path,
#        driver.read_register_callback,
#        driver.write_register_callback,
#        async_cb.callback
#    )
#    drm_manager.set(log_message_level=5)
#    msg = 'This should be DEBUG2 message'
#    drm_manager.set(log_message=msg)
#    assert isfile(logpath)
#    with open(logpath) as f:
#        log_content = f.read()
#    assert "DEBUG2" in log_content
#    assert msg in log_content
#    assert_NoErrorCallback(async_cb)
#    print("Test parameter 'log_message': PASS")

    # Test parameter: ParameterKeyCount
    # Read-only, gives number of key elements
    assert drm_manager.get('ParameterKeyCount') == len(_PARAM_LIST)
    async_cb.assert_NoError()
    print("Test parameter 'ParameterKeyCount': PASS")


#@pytest.mark.skip(reason='Logging to file is not yet implemented')
def test_logging(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging mechanism and enhance coverage"""
    from os.path import isfile
    from os import remove

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

#    regex_short = r'(\S+)\s*: \[Thread \d+\] (.*)'
#    regex_long = r'(\S+)\s* \[DRM-Lib\] \d{4}-\d{2}-\d{2}/\d{2}:\d{2}:\d{2}, \S+:\d+: \[Thread \d+\] (.+)'
#
#    # Test logging with short format
#    try:
#        async_cb.reset()
#        log_file = "log_%s.txt" % time()
#        conf_json.reset()
#        conf_json['settings']['log_verbosity'] = 1
#        conf_json['settings']['log_format'] = 0
#        conf_json['settings']['log_file'] = log_file
#        conf_json.save()
#        drm_manager = accelize_drm.DrmManager(
#            conf_json.path,
#            cred_json.path,
#            driver.read_register_callback,
#            driver.write_register_callback,
#            async_cb.callback
#        )
#        msg = 'This is a message'
#        drm_manager.set(log_message_level=1)
#        drm_manager.set(log_message=msg)
#        assert isfile(log_file)
#        with open(log_file) as f:
#            log_content = f.read()
#        m = match(regex_short, log_content)
#        assert m is not None, log_content
#        assert m.group(1) == 'ERROR'
#        assert m.group(2) == msg
#        async_cb.assert_NoError()
#    finally:
#        del drm_manager
#        if isfile(log_file):
#            remove(log_file)
#    print('Test logging short format: PASS')
#
#    # Test logging with short format
#    try:
#        async_cb.reset()
#        log_file = "log_%s.txt" % time()
#        conf_json.reset()
#        conf_json['settings']['log_verbosity'] = 1
#        conf_json['settings']['log_format'] = 1
#        conf_json['settings']['log_file'] = log_file
#        conf_json.save()
#        drm_manager = accelize_drm.DrmManager(
#            conf_json.path,
#            cred_json.path,
#            driver.read_register_callback,
#            driver.write_register_callback,
#            async_cb.callback
#        )
#        msg = 'This is a message'
#        drm_manager.set(log_message_level=1)
#        drm_manager.set(log_message=msg)
#        assert isfile(log_file)
#        with open(log_file) as f:
#            log_content = f.read()
#        m = match(regex_long, log_content)
#        assert m is not None, log_content
#        assert m.group(1) == 'ERROR'
#        assert m.group(2) == msg
#        async_cb.assert_NoError()
#    finally:
#        del drm_manager
#        if isfile(log_file):
#            remove(log_file)
#    print('Test logging long format: PASS')

    # Test verbosity filter
    msg = 'This is a %s message'
    level_dict = {5:'DEBUG2', 4:'DEBUG', 3:'INFO', 2:'WARNING', 1:'ERROR'}
    for verbosity in range(6):
        async_cb.reset()
        log_file = "log_%s.txt" % time()
        conf_json.reset()
        conf_json['settings']['log_format'] = 1
#            conf_json['settings']['log_file'] = log_file
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        try:
            exp_level = [v for k,v in level_dict.items() if k<=verbosity]
            drm_manager.set(log_verbosity=verbosity)
            verbosity_back = drm_manager.get('log_verbosity')
            assert verbosity_back == verbosity
            for i in sorted(level_dict.keys()):
                drm_manager.set(log_message_level=i)
                drm_manager.set(log_message=msg % level_dict[i])
#            assert isfile(log_file)
#            with open(log_file) as f:
#                log_lines = f.readlines()
#            trace_hit = dict.fromkeys(exp_level, 0)
#            for line in log_lines:
#                m = match(regex_short, line)
#                if m is not None:
#                    trace_msg = m.group(2)
#                    trace_lvl = m.group(1)
#                    assert trace_lvl in exp_level
#                    if trace_msg == msg % trace_lvl:
#                        trace_hit[trace_lvl] += 1
#            assert sum(trace_hit.values()) == verbosity
#            assert all(trace_hit.values())
            async_cb.assert_NoError()
        finally:
            if isfile(log_file):
                remove(log_file)
    print('Test verbosity filter: PASS')


def test_configuration_file_with_bad_authentication(accelize_drm, conf_json, cred_json,
                                                    async_handler):
    """Test errors when bad authentication parameters are provided to
    DRM Manager Constructor or Web Service."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = None
    print()
    try:
        # Test when authentication url in configuration file is wrong
        async_cb.reset()
        cred_json.set_user('accelize_accelerator_test_02')
        conf_json.reset()
        conf_json['licensing']['url'] = "http://accelize.com"
        conf_json['settings']['ws_request_timeout'] = 5
        conf_json['settings']['ws_retry_period_short'] = 1
        conf_json.save()
        assert conf_json['licensing']['url'] == "http://accelize.com"
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "HTTP response code from OAuth2 Web Service: 404" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
        print('Test when authentication url in configuration file is wrong: PASS')

        # Test when client_id is wrong
        async_cb.reset()
        conf_json.reset()
        cred_json.set_user('accelize_accelerator_test_02')
        orig_client_id = cred_json.client_id
        replaced_char = 'A' if orig_client_id[0]!='A' else 'B'
        cred_json.client_id = orig_client_id.replace(orig_client_id[0], replaced_char)
        assert orig_client_id != cred_json.client_id
        cred_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "HTTP response code from OAuth2 Web Service: 401" in str(excinfo.value)
        assert "invalid_client" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
        print('Test when client_id is wrong: PASS')

        # Test when client_secret is wrong
        async_cb.reset()
        conf_json.reset()
        cred_json.set_user('accelize_accelerator_test_02')
        orig_client_secret = cred_json.client_secret
        replaced_char = 'A' if orig_client_secret[0]!='A' else 'B'
        cred_json.client_secret = orig_client_secret.replace(orig_client_secret[0], replaced_char)
        cred_json.save()
        assert orig_client_secret != cred_json.client_secret
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "HTTP response code from OAuth2 Web Service: 401" in str(excinfo.value)
        assert "invalid_client" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
        print('Test when client_secret is wrong: PASS')

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
        assert "Authentication credentials" in str(excinfo.value)
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


def test_configuration_file_with_bad_frequency(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when wrong frequency is given to DRM Controller Constructor"""

    from math import ceil, floor

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    # Before any test, get the real DRM frequency and the gap threshold
    async_cb.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    freq_threshold = drm_manager.get('frequency_detection_threshold')
    freq_period = drm_manager.get('frequency_detection_period')
    drm_manager.activate()
    sleep(2.0*freq_period/1000)
    frequency = drm_manager.get('drm_frequency')
    drm_manager.deactivate()

    # Test no error is returned by asynchronous error callback when the frequency
    # in configuration file differs from the DRM frequency by less than threshold
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = int(floor(frequency * (100.0 + freq_threshold - 1) / 100.0))
    assert abs(conf_json['drm']['frequency_mhz'] - frequency) * 100.0 / frequency < freq_threshold
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager.activate()
    sleep(2.0*freq_period/1000)
    drm_manager.deactivate()
    async_cb.assert_NoError('freq_period=%d ms, freq_threshold=%d%%, frequency=%d MHz'
                            % (freq_period, freq_threshold, frequency))
    print('Test frequency mismatch < threshold: PASS')

    # Test a BADFrequency error is returned by asynchronous error callback when the frequency
    # in configuration file differs from the DRM frequency by more than 2%
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = int(ceil(frequency * (100.0 + freq_threshold + 1) / 100.0))
    assert abs(conf_json['drm']['frequency_mhz'] - frequency) * 100.0 / frequency > freq_threshold
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager.activate()
    sleep(1)
    drm_manager.deactivate()

    assert async_cb.was_called, 'Asynchronous callback NOT called'
    assert async_cb.message is not None, 'Asynchronous callback did not report any message'
    assert search(r'DRM frequency .* differs from .* configuration file',
        async_cb.message) is not None, 'Unexpected message reported by asynchronous callback'
    assert async_cb.errcode == accelize_drm.exceptions.DRMBadFrequency.error_code, \
        'Unexpected error code reported by asynchronous callback'
    print('Test frequency mismatch > threshold: PASS')

#    # Test web service detects a frequency underflow
#    async_cb.reset()
#    conf_json.reset()
#    conf_json['drm']['frequency_mhz'] = 10
#    conf_json.save()
#    assert conf_json['drm']['frequency_mhz'] == 10
#
#    drm_manager = accelize_drm.DrmManager(
#        conf_json.path,
#        cred_json.path,
#        driver.read_register_callback,
#        driver.write_register_callback
#    )
#    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
#        drm_manager.activate()
#    assert '???' in str(excinfo.value)
#    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
#    print('Test frequency underflow: PASS')

    # Test web service detects a frequency overflow
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = 1000
    conf_json.save()
    assert conf_json['drm']['frequency_mhz'] == 1000

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    print('msg=', str(excinfo.value))
    assert search(r'HTTP response code from License Web Service: 400 .*Ensure this value '
                  r'is less than or equal to 250', str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    print('Test frequency overflow: PASS')


def test_configuration_file_bad_product_id(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when an incorrect product ID is requested to License Web Server"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    # Test Web Service when a bad product ID is provided
    async_cb.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    drm_manager.set(bad_product_id=1)
    drm_manager.get('product_info')
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert 'HTTP response code from License Web Service: 400 (Server has detected an incoherency ' \
           'related to the product object.)' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_NoError()


@pytest.mark.skip(reason='Not currently specified')
def test_2_drm_manager_concurrently(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when 2 DrmManager instances are used."""

    driver = accelize_drm.pytest_fpga_driver[0]

    async_cb1 = async_handler.create()
    async_cb2 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_02')

    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb1.callback
    )

    with pytest.raises(accelize_drm.exceptions.DRMBadUsage) as excinfo:
        drm_manager2 = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb2.callback
        )
    assert 'Another instance of the DRM Manager is currently owning the HW' in str(excinfo.value)


@pytest.mark.long_run
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
        assert not activators.is_activated(), 'At least one activator is unlocked'
        # Activate all activators
        drm_manager.activate()
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Deactivate all activators
        drm_manager.deactivate()
        # Check all activators are locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        assert not activators.is_activated(), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on start/stop: PASS')

        # Test license status on start/pause

        # Check all activators are locked
        assert not drm_manager.get('license_status'), 'License is not inactive'
        assert not activators.is_activated(), 'At least one activator is unlocked'
        # Activate all activators
        drm_manager.activate()
        start = datetime.now()
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Pause all activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license status on start/pause: PASS')

        # Test license status on resume from valid license/pause

        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Resume all activators
        drm_manager.activate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Pause all activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert activators.is_activated(), 'At least one activator is locked'
        # Wait until license expires
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=2 * lic_duration + 1) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check all activators are now locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        assert not activators.is_activated(), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on resume from valid license/pause: PASS')

        # Test license status on resume from expired license/pause

        # Check all activators are locked
        assert not drm_manager.get('license_status'), 'License is not inactive'
        assert not activators.is_activated(), 'At least one activator is unlocked'
        # Resume all activators
        drm_manager.activate(True)
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Pause all activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license status on resume from expired license/pause: PASS')

        # Test license status on resume/stop

        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        async_cb.assert_NoError()
        # Resume all activators
        drm_manager.activate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Deactivate all activators
        drm_manager.deactivate()
        # Check all activators are locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        assert not activators.is_activated(), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on resume/stop: PASS')

        # Test license status on restart from paused session/stop

        # Check all activators are locked again
        assert not drm_manager.get('license_status'), 'License is not inactive'
        assert not activators.is_activated(), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        # Activate all activators
        drm_manager.activate()
        # Check all activators are unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Pause activators
        drm_manager.deactivate(True)
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        # Restart all activators
        drm_manager.activate()
        # Check all activators are still unlocked
        assert drm_manager.get('license_status'), 'License is not active'
        assert activators.is_activated(), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license status on restart: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


@pytest.mark.long_run
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
        assert not activators.is_activated(), 'At least one activator is locked'
        # Start
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        # Pause
        sleep(lic_duration/2)
        drm_manager.deactivate(True)
        # Check license is still running and activator are all unlocked
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is unlocked'
        # Wait right before expiration
        wait_period = start + timedelta(seconds=2*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running and activators are all unlocked
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is unlocked'
        # Wait a bit more time the expiration
        sleep(3)
        # Check no license is running
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is locked'
        drm_manager.deactivate()
        # Check no license is running
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license expires after 2 duration periods when start/pause/stop: PASS')

        # Test license does not expire after 3 duration periods when start

        # Check no license is running
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is locked'
        # Start
        drm_manager.activate()
        start = datetime.now()
        # Check license is running
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is locked'
        # Wait 3 duration periods
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=3*lic_duration+2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is locked'
        # Stop
        drm_manager.deactivate()
        # Check no license is running
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license does not expire after 3 duration periods when start: PASS')

        # Test license does not expire after 3 duration periods when start/pause

        # Check no license is running
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is unlocked'
        # Start
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        # Check license is running
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is locked'
        # Wait 1 full duration period
        wait_period = start + timedelta(seconds=lic_duration+lic_duration/2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is locked'
        # Pause
        drm_manager.deactivate(True)
        # Wait right before the next 2 duration periods expire
        wait_period = start + timedelta(seconds=3*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        assert drm_manager.get('license_status')
        assert activators.is_activated(), 'At least one activator is locked'
        # Wait a bit more time the expiration
        sleep(3)
        # Check license has expired
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is unlocked'
        drm_manager.deactivate()
        # Check no license is running
        assert not drm_manager.get('license_status')
        assert not activators.is_activated(), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license does not expire after 3 duration periods when start/pause: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


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
        assert 'HTTP response code from License Web Service: 470' in str(excinfo.value)
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
    conf_json['settings']['ws_retry_deadline'] = 0
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
