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
from time import time


LOG_FORMAT_SHORT = "[%^%=8l%$] %-6t, %v"
LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

REGEX_FORMAT_SHORT = r'\[\s*(\w+)\s*\] \s*\d+\s*, %s'
REGEX_FORMAT_LONG  = r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3} - \s*\S+:\d+\s* \[\s*(\w+)\s*\] \s*\d+\s*, %s'


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


def ordered_json(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered_json(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered_json(x) for x in obj)
    else:
        return obj


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


@pytest.mark.minimum
def test_wrong_drm_controller_address(accelize_drm, conf_json, cred_json, async_handler):
    """Test when a wrong DRM Controller offset is given"""
    async_cb = async_handler.create()
    async_cb.reset()
    driver = accelize_drm.pytest_fpga_driver[0]
    ctrl_base_addr_backup = driver._drm_ctrl_base_addr
    driver._drm_ctrl_base_addr += 0x10000
    try:
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert 'Unable to find DRM Controller registers.' in str(excinfo.value)
        assert 'Please verify' in str(excinfo.value)
    finally:
        driver._drm_ctrl_base_addr = ctrl_base_addr_backup


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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('license_type') == 'Floating/Metering'
    drmLicType = drm_manager.get('drm_license_type')
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert "License Web Service error 400" in str(excinfo.value)
    assert "DRM WS request failed" in str(excinfo.value)
    assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
    assert "User account has no entitlement. Purchase additional licenses via your portal" in str(excinfo.value)
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
        assert drm_manager.get('drm_license_type') == drmLicType
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "License Web Service error 400" in str(excinfo.value)
        assert "DRM WS request failed" in str(excinfo.value)
        assert search(r'\\"No Entitlement\\" with .+ for \S+_test_01@accelize.com', str(excinfo.value))
        assert "User account has no entitlement. Purchase additional licenses via your portal" in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-01 entitlements: PASS')

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
        assert "License Web Service error 400" in str(excinfo.value)
        assert "DRM WS request failed" in str(excinfo.value)
        assert search(r'\\"No Entitlement\\" with .+ for \S+_test_02@accelize.com', str(excinfo.value))
        assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-02 entitlements: PASS')

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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
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
        assert "License Web Service error 400" in str(excinfo.value)
        assert "DRM WS request failed" in str(excinfo.value)
        assert search(r'\\"No Entitlement\\" with .+ for \S+_test_04@accelize.com', str(excinfo.value))
        assert 'No valid NodeLocked entitlement found for your account' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
    finally:
        accelize_drm.clean_nodelock_env(conf_json=conf_json)
    print('Test user-04 entitlements: PASS')


@pytest.mark.minimum
def test_parameter_key_modification_with_config_file(accelize_drm, conf_json, cred_json,
                                                     async_handler):
    """Test accesses to parameters"""

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
    orig_retry_period_long = drm_manager.get('ws_retry_period_long')
    orig_retry_period_short = drm_manager.get('ws_retry_period_short')
    orig_response_timeout = drm_manager.get('ws_request_timeout')

    # Test parameter: log_verbosity
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
    assert drm_manager.get('log_verbosity') == exp_value
    drm_manager.set(log_verbosity=orig_log_verbosity)
    print("Test parameter 'log_verbosity': PASS")

    # Test parameter: log_format
    async_cb.reset()
    conf_json.reset()
    exp_value = LOG_FORMAT_LONG
    conf_json['settings']['log_format'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_format') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_format': PASS")

    # Test parameter: log_file_verbosity
    async_cb.reset()
    conf_json.reset()
    exp_value = 0
    conf_json['settings']['log_file_verbosity'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_file_verbosity') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_file_verbosity': PASS")

    # Test parameter: log_file_format
    async_cb.reset()
    conf_json.reset()
    exp_value = LOG_FORMAT_SHORT
    conf_json['settings']['log_file_format'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_file_format') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_file_format': PASS")

    # Test parameter: log_file_path
    async_cb.reset()
    conf_json.reset()
    exp_value = realpath("./drmlib.%d.%s.log" % (getpid(), time()))
    conf_json['settings']['log_file_path'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_file_path') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_file_path': PASS")

    # Test parameter: log_file_type
    async_cb.reset()
    conf_json.reset()
    exp_value = 1
    conf_json['settings']['log_file_type'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_file_type') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_file_type': PASS")

    # Test parameter: log_file_rotating_size
    async_cb.reset()
    conf_json.reset()
    exp_value = 1024
    conf_json['settings']['log_file_rotating_size'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_file_rotating_size') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_file_rotating_size': PASS")

    # Test parameter: log_file_rotating_num
    async_cb.reset()
    conf_json.reset()
    exp_value = 10
    conf_json['settings']['log_file_rotating_num'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('log_file_rotating_num') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'log_file_rotating_num': PASS")

    # Test parameter: frequency_detection_method
    if accelize_drm.pytest_new_freq_method_supported:
        async_cb.reset()
        conf_json.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('frequency_detection_method') == 1
        print("Test parameter 'frequency_detection_method': PASS")

    # Test parameter: bypass_frequency_detection
    if accelize_drm.pytest_new_freq_method_supported:
        async_cb.reset()
        conf_json.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert not drm_manager.get('bypass_frequency_detection')
        conf_json.reset()
        conf_json['drm']['bypass_frequency_detection'] = True
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('bypass_frequency_detection')
        print("Test parameter 'bypass_frequency_detection': PASS")

    # Test parameter: drm_frequency
    async_cb.reset()
    conf_json.reset()
    exp_value = 2*orig_frequency_mhz
    conf_json['drm']['bypass_frequency_detection'] = True
    conf_json['drm']['frequency_mhz'] = exp_value
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('drm_frequency') == exp_value
    async_cb.assert_NoError()
    print("Test parameter 'drm_frequency': PASS")

    # Test parameter: frequency_detection_period
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

    # Test parameter: ws_retry_period_long
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
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
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
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
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
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code

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


@pytest.mark.minimum
@pytest.mark.aws
def test_c_unittests(accelize_drm, exec_func):
    """Test errors when missing arguments are given to DRM Controller Constructor"""
    driver = accelize_drm.pytest_fpga_driver[0]
    if driver._name != 'aws':
        pytest.skip("C unit-tests are only supported with AWS driver.")

    exec_lib = exec_func.load('unittests', driver._fpga_slot_id)

    # Test when read register callback is null
    exec_lib.run('test_null_read_callback')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Read register callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test when write register callback is null
    exec_lib.run('test_null_write_callback')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Write register callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test when asynchronous error callback is null
    exec_lib.run('test_null_error_callback')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Asynchronous error callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test various types of get and set functions
    exec_lib.run('test_types_of_get_and_set_functions')
    assert exec_lib.returncode == 0
    assert exec_lib.asyncmsg is None

    # Test out of range of get function
    exec_lib.run('test_get_function_out_of_range')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Cannot find parameter with ID: ' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test get_json_string with bad format
    exec_lib.run('test_get_json_string_with_bad_format')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadFormat.error_code
    assert 'Cannot parse JSON string because' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test get_json_string with empty string
    exec_lib.run('test_get_json_string_with_empty_string')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadFormat.error_code
    assert 'Cannot parse an empty JSON string' in exec_lib.stdout
    assert exec_lib.asyncmsg is None


def test_parameter_key_modification_with_get_set(accelize_drm, conf_json, cred_json, async_handler,
                                                 ws_admin):
    """Test accesses to parameter"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()

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
    assert value['num_activators'] == activators.length
    assert value['license_type'] == 'Floating/Metering'
    async_cb.assert_NoError()
    print("Test when parameter is a list: PASS")


    # Test parameter: log_verbosity
    orig_val = drm_manager.get('log_verbosity')
    exp_val = 1 if orig_val == 0 else 0
    drm_manager.set(log_verbosity=exp_val)
    assert drm_manager.get('log_verbosity') == exp_val
    drm_manager.set(log_verbosity=orig_val)
    async_cb.assert_NoError()
    print("Test parameter 'log_verbosity': PASS")

    # Test parameter: log_format
    orig_val = drm_manager.get('log_format')
    exp_val = LOG_FORMAT_LONG
    drm_manager.set(log_format=exp_val)
    assert drm_manager.get('log_format') == exp_val
    drm_manager.set(log_format=orig_val)
    async_cb.assert_NoError()
    print("Test parameter 'log_format': PASS")

    # Test parameter: log_file_verbosity
    orig_val = drm_manager.get('log_file_verbosity')
    exp_val = 1 if orig_val == 0 else 0
    drm_manager.set(log_file_verbosity=exp_val)
    assert drm_manager.get('log_file_verbosity') == exp_val
    drm_manager.set(log_file_verbosity=orig_val)
    async_cb.assert_NoError()
    print("Test parameter 'log_file_verbosity': PASS")

    # Test parameter: log_file_format
    orig_val = drm_manager.get('log_file_format')
    assert orig_val == LOG_FORMAT_LONG
    exp_val = LOG_FORMAT_SHORT
    drm_manager.set(log_file_format=exp_val)
    assert drm_manager.get('log_file_format') == exp_val
    drm_manager.set(log_file_format=orig_val)
    async_cb.assert_NoError()
    print("Test parameter 'log_file_format': PASS")

    # Test parameter: log_file_path, log_file_type, log_file_rotating_size, log_file_rotating_num
    # => Cannot be written programmatically

    # Test parameter: license_type in metering
    assert drm_manager.get('license_type') == 'Floating/Metering'
    async_cb.assert_NoError()
    print("Test parameter 'license_type' in Metered: PASS")

    # Test parameter: drm_license_type, license_duration
    drm_manager.activate()
    assert drm_manager.get('drm_license_type') == 'Floating/Metering'
    assert drm_manager.get('license_duration') != 0
    drm_manager.deactivate()
    async_cb.assert_NoError()
    print("Test parameter 'drm_license_type', 'license_duration': PASS")

    # Test parameter: num_activators
    nb_activator = drm_manager.get('num_activators')
    assert nb_activator == activators.length, 'Unexpected number of activators'
    print("Test parameter 'num_activators': PASS")

    # Test parameter: session_id
    drm_manager.activate()
    sessionId = drm_manager.get('session_id')
    assert len(sessionId) == 16, 'Unexpected length of session ID'
    drm_manager.deactivate()
    async_cb.assert_NoError()
    print("Test parameter 'session_id': PASS")

    # Test parameter: session_status
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
    assert not drm_manager.get('license_status')
    drm_manager.activate()
    assert drm_manager.get('license_status')
    drm_manager.deactivate()
    assert not drm_manager.get('license_status')
    async_cb.assert_NoError()
    print("Test parameter 'license_status': PASS")

    # Test parameter: metered_data
    drm_manager.activate()
    activators[0].generate_coin(10)
    activators[0].check_coin(drm_manager.get('metered_data'))
    async_cb.assert_NoError()
    drm_manager.deactivate()
    activators[0].reset_coin()

    print("Test parameter 'metered_data': PASS")

    # Test parameter: page_ctrlreg
    page = drm_manager.get('page_ctrlreg')
    assert search(r'Register\s+@0x00:\s+0x00000000', page), 'Unexpected content of page_ctrlreg'
    print("Test parameter 'page_ctrlreg': PASS")

    # Test parameter: page_vlnvfile
    page = drm_manager.get('page_vlnvfile')
    assert search(r'Register\s+@0x00:\s+0x00000001', page), 'Unexpected content of page_vlnvfile'
    print("Test parameter 'page_vlnvfile': PASS")

    # Test parameter: page_licfile
    page = drm_manager.get('page_licfile')
    assert search(r'Register\s+@0x00:\s+0x00000002', page), 'Unexpected content of page_licfile'
    print("Test parameter 'page_licfile': PASS")

    # Test parameter: page_tracefile
    page = drm_manager.get('page_tracefile')
    assert search(r'Register\s+@0x00:\s+0x00000003', page), 'Unexpected content of page_tracefile'
    print("Test parameter 'page_tracefile': PASS")

    # Test parameter: page_meteringfile
    page = drm_manager.get('page_meteringfile')
    assert search(r'Register\s+@0x00:\s+0x00000004', page), 'Unexpected content of page_meteringfile'
    print("Test parameter 'page_meteringfile': PASS")

    # Test parameter: page_mailbox
    page = drm_manager.get('page_mailbox')
    assert search(r'Register\s+@0x00:\s+0x00000005', page), 'Unexpected content of page_mailbox'
    print("Test parameter 'page_mailbox': PASS")

    # Test parameter: hw_report
    hw_report = drm_manager.get('hw_report')
    nb_lines = len(tuple(finditer(r'\n', hw_report)))
    assert nb_lines > 10, 'Unexpected HW report content'
    print("Test parameter 'hw_report': PASS")

    # Test parameter: frequency_detection_threshold
    orig_freq_threhsold = drm_manager.get('frequency_detection_threshold')    # Save original threshold
    exp_freq_threhsold = orig_freq_threhsold * 2
    drm_manager.set(frequency_detection_threshold=exp_freq_threhsold)
    new_freq_threhsold = drm_manager.get('frequency_detection_threshold')
    assert new_freq_threhsold == exp_freq_threhsold, 'Unexpected frequency dectection threshold percentage'
    drm_manager.set(frequency_detection_threshold=orig_freq_threhsold)    # Restore original threshold
    print("Test parameter 'frequency_detection_threshold': PASS")

    # Test parameter: frequency_detection_period
    orig_freq_period = drm_manager.get('frequency_detection_period')    # Save original period
    exp_freq_period = orig_freq_period * 2
    drm_manager.set(frequency_detection_period=exp_freq_period)
    new_freq_period = drm_manager.get('frequency_detection_period')
    assert new_freq_period == exp_freq_period, 'Unexpected frequency dectection period'
    drm_manager.set(frequency_detection_period=orig_freq_period)    # Restore original period
    print("Test parameter 'frequency_detection_period': PASS")

    # Test parameter: drm_frequency
    freq_period = drm_manager.get('frequency_detection_period')    # Save original period
    drm_manager.activate()
    #sleep(2.0*freq_period/1000)
    freq_drm = drm_manager.get('drm_frequency')
    drm_manager.deactivate()
    assert 125 <= freq_drm <= 126, 'Unexpected frequency gap threshold'
    print("Test parameter 'drm_frequency': PASS")

    # Test parameter: product_info
    from pprint import pformat
    product_id = pformat(drm_manager.get('product_info'))
    exp_product_id = pformat(activators.product_id)
    assert product_id == exp_product_id, 'Unexpected product ID'
    print("Test parameter 'product_info': PASS")

    # Test parameter: mailbox_size
    mailbox_size = drm_manager.get('mailbox_size')
    assert mailbox_size == 14, 'Unexpected Mailbox size'
    print("Test parameter 'mailbox_size': PASS")

    # Test parameter: token_string, token_validity and token_time_left
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
    list_param = drm_manager.get('list_all')
    assert isinstance(list_param , list)
    assert len(list_param) == len(_PARAM_LIST)
    assert all(key in _PARAM_LIST for key in list_param)
    print("Test parameter 'list_all': PASS")

    # Test parameter: dump_all
    dump_param = drm_manager.get('dump_all')
    assert isinstance(dump_param, dict)
    assert len(dump_param) == _PARAM_LIST.index('dump_all')
    assert all(key in _PARAM_LIST for key in dump_param.keys())
    print("Test parameter 'dump_all': PASS")

    # Test parameter: custom_field
    from random import randint
    val_exp = randint(0,0xFFFFFFFF)
    val_init = drm_manager.get('custom_field')
    assert val_exp != val_init
    drm_manager.set(custom_field=val_exp)
    assert drm_manager.get('custom_field') == val_exp
    print("Test parameter 'custom_field': PASS")

    # Test parameter: mailbox_data
    from random import sample
    mailbox_size = drm_manager.get('mailbox_size')
    wr_msg = sample(range(0xFFFFFFFF), mailbox_size)
    drm_manager.set(mailbox_data=wr_msg)
    rd_msg = drm_manager.get('mailbox_data')
    assert type(rd_msg) == type(wr_msg) == list
    assert rd_msg == wr_msg
    print("Test parameter 'mailbox_data': PASS")

    # Test parameter: ws_retry_period_long
    orig_retry_period_long = drm_manager.get('ws_retry_period_long')  # Save original value
    orig_retry_period_short = drm_manager.get('ws_retry_period_short')
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(ws_retry_period_long=orig_retry_period_short)
    assert search(r'ws_retry_period_long .+ must be greater than ws_retry_period_short .+',
                  str(excinfo.value)) is not None
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    exp_value = orig_retry_period_long + 1
    drm_manager.set(ws_retry_period_long=exp_value)
    assert drm_manager.get('ws_retry_period_long') == exp_value
    drm_manager.set(ws_retry_period_long=orig_retry_period_long)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_retry_period_long': PASS")

    # Test parameter: ws_retry_period_short
    orig_retry_period_short = drm_manager.get('ws_retry_period_short')  # Save original value
    orig_retry_period_long = drm_manager.get('ws_retry_period_long')
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(ws_retry_period_short=orig_retry_period_long)
    assert search(r'ws_retry_period_long .+ must be greater than ws_retry_period_short .+',
                  str(excinfo.value)) is not None
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    exp_value = orig_retry_period_short + 1
    drm_manager.set(ws_retry_period_short=exp_value)
    assert drm_manager.get('ws_retry_period_short') == exp_value
    drm_manager.set(ws_retry_period_short=orig_retry_period_short)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_retry_period_short': PASS")

    # Test parameter: ws_request_timeout
    orig_response_timeout = drm_manager.get('ws_request_timeout')  # Save original value
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(ws_request_timeout=0)
    assert "ws_request_timeout must not be 0" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    exp_value = orig_response_timeout + 100
    drm_manager.set(ws_request_timeout=exp_value)
    assert drm_manager.get('ws_request_timeout') == exp_value
    drm_manager.set(ws_request_timeout=orig_response_timeout)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'ws_request_timeout': PASS")

    # Test parameter: log_message_level
    level = drm_manager.get('log_message_level')
    exp_level = 5 if level!=5 else 4
    drm_manager.set(log_message_level=exp_level)
    assert drm_manager.get('log_message_level') == exp_level
    async_cb.assert_NoError()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(log_message_level=100)
    assert 'log_message_level (100) is out of range [0:6]' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
    print("Test parameter 'log_message_level': PASS")

    # Test parameter: trigger_async_callback
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
    # => Skipped: Tested in test_configuration_file_bad_product_id

    # Test parameter: bad_oauth2_token
    # => Skipped: Tested in test_configuration_file_with_bad_authentication

    # Test parameter: ParameterKeyCount
    assert drm_manager.get('ParameterKeyCount') == len(_PARAM_LIST)
    async_cb.assert_NoError()
    print("Test parameter 'ParameterKeyCount': PASS")

    # Test parameter: log_message
    from os.path import isfile
    async_cb.reset()
    conf_json.reset()
    logpath = realpath("./drmlib.%d.%s.log" % (getpid(), time()))
    verbosity = 5
    conf_json['settings']['log_file_verbosity'] = verbosity
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.set(log_message_level=verbosity)
        msg = 'This line should appear in log file'
        drm_manager.set(log_message=msg)
        del drm_manager
        gc.collect()
        assert isfile(logpath)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        assert "critical" in log_content
        assert msg in log_content
    finally:
        if isfile(logpath):
            remove(logpath)
    async_cb.assert_NoError()
    print("Test parameter 'log_message': PASS")


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
        assert "OAuth2 Web Service error 404" in str(excinfo.value)
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
        assert "OAuth2 Web Service error 401" in str(excinfo.value)
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
        assert "OAuth2 Web Service error 401" in str(excinfo.value)
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
    # in configuration file differs from the DRM frequency by less than the threshold
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = int(floor(frequency * (100.0 + freq_threshold/2) / 100.0))
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
    # in configuration file differs from the DRM frequency by more than the threshold
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = int(ceil(frequency * (100.0 + 2*freq_threshold) / 100.0))
    assert abs(conf_json['drm']['frequency_mhz'] - frequency) * 100.0 / frequency > freq_threshold
    conf_json.save()

    if accelize_drm.pytest_new_freq_method_supported:
        with pytest.raises(accelize_drm.exceptions.DRMBadFrequency) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert search(r'DRM frequency .* differs from .* configuration file',
            str(excinfo.value)) is not None
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFrequency.error_code
    else:
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

    # Test web service detects a frequency underflow
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['bypass_frequency_detection'] = True
    conf_json['drm']['frequency_mhz'] = 40
    conf_json.save()
    assert conf_json['drm']['frequency_mhz'] == 40

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert 'License Web Service error 400' in str(excinfo.value)
    assert 'Ensure this value is greater than or equal to 50' in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
    print('Test frequency underflow: PASS')

    # Test web service detects a frequency overflow
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['bypass_frequency_detection'] = True
    conf_json['drm']['frequency_mhz'] = 400
    conf_json.save()
    assert conf_json['drm']['frequency_mhz'] == 400

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert 'License Web Service error 400' in str(excinfo.value)
    assert 'Ensure this value is less than or equal to 320' in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
    print('Test frequency overflow: PASS')


def test_mailbox_write_overflow(accelize_drm, conf_json, cred_json, async_handler):
    from random import sample

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    mb_size = drm_manager.get('mailbox_size')
    assert mb_size > 0

    mb_data = sample(range(0xFFFFFFFF), mb_size + 1)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(mailbox_data=mb_data)
    assert 'Trying to write out of Mailbox memory space' in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()


def test_mailbox_type_error(accelize_drm, conf_json, cred_json, async_handler):
    from random import sample

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(mailbox_data='this is bad type')
    assert 'Value must be an array of integers' in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()


def test_configuration_file_wrong_product_id(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when an incorrect product ID is requested to License Web Server"""

    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    # Test Web Service when an unexisting product ID is provided
    async_cb.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager.set(bad_product_id=1)
    product_id = drm_manager.get('product_info')
    pid_string = '{vendor}/{library}/{name}'.format(**product_id)
    assert pid_string == 'accelize.com/refdesign/BAD_NAME_JUST_FOR_TEST'
    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert 'License Web Service error 400' in str(excinfo.value)
    assert 'DRM WS request failed' in str(excinfo.value)
    assert search(r'\\"Unknown Product ID\\" \s*%s for' % pid_string, str(excinfo.value)) is not None
    assert search(r'Product ID \s*%s from license request is unknown' % pid_string, str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_NoError()


def test_configuration_file_empty_and_corrupted_product_id(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when an incorrect product ID is requested to License Web Server"""

    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    try:
        # Test Web Service when an empty product ID is provided
        empty_fpga_image = refdesign.get_image_id('empty_product_id')
        if empty_fpga_image is None:
            pytest.skip("No FPGA image found for 'empty_product_id'")
        driver.program_fpga(empty_fpga_image)
        async_cb.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('product_info') is None
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'License Web Service error 400' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Unknown Product ID\\" for ', str(excinfo.value)) is not None
        assert 'Product ID from license request is not set' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
        async_cb.assert_NoError()
        print('Test Web Service when an empty product ID is provided: PASS')

        # Test when a misformatted product ID is provided
        bad_fpga_image = refdesign.get_image_id('bad_product_id')
        if bad_fpga_image is None:
            pytest.skip("No FPGA image found for 'bad_product_id'")
        driver.program_fpga(bad_fpga_image)
        async_cb.reset()
        with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert 'Failed to parse Read-Only Mailbox in DRM Controller:' in str(excinfo.value)
        assert search(r'Cannot parse JSON string .* because ', str(excinfo.value))
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFormat.error_code
        async_cb.assert_NoError()
        print('Test Web Service when a misformatted product ID is provided: PASS')

    finally:
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


@pytest.mark.skip(reason='Not supported')
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


def test_readonly_and_writeonly_parameters(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test readonly parameter cannot be written and writeonly parameter cannot be read
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    # Test write-only parameter cannot be read
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.get('trigger_async_callback')
    assert "Parameter 'trigger_async_callback' cannot be read" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()

    # Test read-only parameter cannot be written
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(license_duration=10)
    assert "Parameter 'license_duration' cannot be overwritten" in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()


@pytest.mark.endurance
@pytest.mark.hwtst
def test_authentication_expiration(accelize_drm, conf_json, cred_json, async_handler):
    from random import sample
    driver = accelize_drm.pytest_fpga_driver[0]
    activator = accelize_drm.pytest_fpga_activators[0][0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    activator.generate_coin(1000)
    assert not drm_manager.get('license_status')
    activator.autotest(is_activated=False)
    drm_manager.activate()
    lic_duration = drm_manager.get('license_duration')
    assert drm_manager.get('license_status')
    activator.autotest(is_activated=True)
    activators[0].check_coin(drm_manager.get('metered_data'))
    start = datetime.now()
    expiration_period = 12000
    while True:
        seconds_left = (expiration_period + 2*lic_duration) - (datetime.now() - start).total_seconds()
        if seconds_left < 0:
            break
        assert drm_manager.get('license_status')
        assert activator.generate_coin(1)
        activators[0].check_coin(drm_manager.get('metered_data'))
        print('Remaining time: ', seconds_left, ' s / current coins: ', activators[0].metering_data)
        sleep(5)
    drm_manager.deactivate()
    assert not drm_manager.get('license_status')
    activator.autotest(is_activated=False)


def test_directory_creation(accelize_drm, conf_json, cred_json, async_handler):
    from shutil import rmtree
    from subprocess import check_call
    from os import makedirs, access, R_OK, W_OK
    from os.path import isdir, expanduser
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_type = 1
    log_dir = realpath(expanduser('~/tmp_log_dir.%s' % str(time())))
    if not isdir(log_dir):
        makedirs(log_dir)

    try:
        # Test error when creating directory

        # Create immutable folder
        check_call('sudo chattr +i %s' % log_dir, shell=True)
        assert not access(log_dir, W_OK)
        try:
            log_path = join(log_dir, "tmp", "drmlib.%d.%s.log" % (getpid(), str(time())))
            assert not isdir(dirname(log_path))
            async_cb.reset()
            conf_json.reset()
            conf_json['settings']['log_file_path'] = log_path
            conf_json['settings']['log_file_type'] = log_type
            conf_json.save()
            with pytest.raises(accelize_drm.exceptions.DRMExternFail) as excinfo:
                drm_manager = accelize_drm.DrmManager(
                        conf_json.path,
                        cred_json.path,
                        driver.read_register_callback,
                        driver.write_register_callback,
                        async_cb.callback
                    )
            assert "Failed to create log file %s" % log_path in str(excinfo.value)
        finally:
            check_call('sudo chattr -i %s' % log_dir, shell=True)
            assert access(log_dir, W_OK)
            if isfile(log_path):
                remove(log_path)
        print('Test folder creation error: PASS')

        # Test directory already exists

        assert isdir(log_dir)
        assert access(log_dir, W_OK)
        log_path = join(log_dir, "drmlib.%d.%s.log" % (getpid(), time()))
        assert not isfile(log_path)
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json.save()
        try:
            drm_manager = accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
            del drm_manager
            gc.collect()
            assert isfile(log_path)
        finally:
            if isfile(log_path):
                remove(log_path)
        print('Test already existing folder: PASS')

        # Test directory creation

        assert isdir(log_dir)
        assert access(log_dir, W_OK)
        intermediate_dir = join(log_dir, 'tmp')
        assert not isdir(intermediate_dir)
        log_path = join(intermediate_dir, "drmlib.%d.%s.log" % (getpid(), time()))
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json.save()
        try:
            drm_manager = accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
            del drm_manager
            gc.collect()
            assert isfile(log_path)
        finally:
            if isdir(intermediate_dir):
                rmtree(intermediate_dir)
        print('Test creation of new folder: PASS')

    finally:
        if isdir(log_dir):
            rmtree(log_dir)


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


@pytest.mark.minimum
def test_drm_manager_frequency_detection_method1(accelize_drm, conf_json, cred_json, async_handler):
    """Test method1 (based on dedicated counter in AXI wrapper) to estimate drm frequency is working"""

    if not accelize_drm.pytest_new_freq_method_supported:
        pytest.skip("New frequency detection method is not supported: test skipped")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.reset()
    logpath = realpath("./drmlib.%d.%s.log" % (getpid(), time()))
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('frequency_detection_method') == 1
    del drm_manager
    gc.collect()
    assert isfile(logpath)
    with open(logpath, 'rt') as f:
        log_content = f.read()
    assert "Use dedicated counter to compute DRM frequency (method 1)" in log_content


def test_drm_manager_frequency_detection_method1_exception(accelize_drm, conf_json, cred_json, async_handler):
    """Test method1 (based on dedicated counter in AXI wrapper) to estimate drm frequency is working"""

    if not accelize_drm.pytest_new_freq_method_supported:
        pytest.skip("New frequency detection method is not supported: test skipped")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    conf_json.reset()
    logpath = realpath("./drmlib.%d.%s.log" % (getpid(), time()))
    conf_json['settings']['frequency_detection_period'] = (int)(2**32 / 125000000 * 1000) + 2
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadFrequency) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'Frequency auto-detection failed: frequency_detection_period parameter \([^)]+\) is too long',
                  str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFrequency.error_code
    async_cb.assert_NoError()


@pytest.mark.minimum
def test_drm_manager_frequency_detection_method2(accelize_drm, conf_json, cred_json, async_handler):
    """Test method2 (based on license timer) to estimate drm frequency is still working"""

    refdesign = accelize_drm.pytest_ref_designs
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()

    # Program FPGA with HDK 3.x.x (with frequency detection method 2)
    hdk = list(filter(lambda x: x.startswith('3.'), refdesign.hdk_versions))[-1]
    assert hdk.startswith('3.')
    image_id = refdesign.get_image_id(hdk)
    try:
        driver.program_fpga(image_id)
        conf_json.reset()
        logpath = realpath("./drmlib.%d.%s.log" % (getpid(), time()))
        conf_json['settings']['log_file_verbosity'] = 1
        conf_json['settings']['log_file_type'] = 1
        conf_json['settings']['log_file_path'] = logpath
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('frequency_detection_method') == 2
        drm_manager.activate()
        assert drm_manager.get('frequency_detection_method') == 2
        drm_manager.deactivate()
        del drm_manager
        gc.collect()
        assert isfile(logpath)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        assert "Use license timer counter to compute DRM frequency (method 2)" in log_content
    finally:
        if isfile(logpath):
            remove(logpath)
        # Reprogram FPGA with original image
        driver.program_fpga(fpga_image_bkp)


def test_drm_manager_frequency_detection_bypass(accelize_drm, conf_json, cred_json, async_handler):
    """Test bypass of frequency detection"""

    if not accelize_drm.pytest_new_freq_method_supported:
        pytest.skip("New frequency detection method is not supported: test skipped")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test when bypass = True
    conf_json.reset()
    conf_json['drm']['bypass_frequency_detection'] = True
    conf_json['drm']['frequency_mhz'] = 80
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('drm_frequency') == 80
    drm_manager.activate()
    sleep(1)
    assert drm_manager.get('drm_frequency') == 80
    async_cb.assert_NoError()
    print('Test bypass_frequency_detection=true: PASS')

    # Test when bypass = False
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = 80
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadFrequency) as excinfo:
        drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert search(r'DRM frequency .* differs from .* configuration file',
            str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFrequency.error_code
    async_cb.assert_NoError()
    print('Test bypass_frequency_detection=false: PASS')


@pytest.mark.hwtst
def test_drm_manager_bist(accelize_drm, conf_json, cred_json, async_handler):
    """Test register access BIST"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test read callback error
    def my_wrong_read_callback(register_offset, returned_data):
        addr = register_offset
        if register_offset > 0 and register_offset <= 0x40:
            addr += 0x4
        return driver.read_register_callback(addr, returned_data, driver)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            my_wrong_read_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'DRM Communication Self-Test 2 failed: Could not access DRM Controller registers' in str(excinfo.value)
    assert 'Please verify' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()

    # Test write callback error
    def my_wrong_write_callback(register_offset, data_to_write):
        return driver.write_register_callback(register_offset*2, data_to_write, driver)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            my_wrong_write_callback,
            async_cb.callback
        )
    assert 'DRM Communication Self-Test 2 failed: Could not access DRM Controller registers' in str(excinfo.value)
    assert 'Please verify' in str(excinfo.value)
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
