# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from os import remove, getpid
from os.path import isfile, realpath
from re import match, search, finditer
from time import sleep, time
from tests.conftest import wait_func_true


LOG_FORMAT_SHORT = "[%^%=8l%$] %-6t, %v"
LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

_PARAM_LIST = ('license_type',
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
               'log_message',
               'hdk_compatibility',
               'health_period',
               'health_retry',
               'health_retry_sleep'
)


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
    exp_value = accelize_drm.create_log_path(whoami())
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
    remove(exp_value)
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

    # Test parameter: ParameterKeyCount
    assert drm_manager.get('ParameterKeyCount') == len(_PARAM_LIST)
    async_cb.assert_NoError()
    print("Test parameter 'ParameterKeyCount': PASS")

    # Test parameter: log_message
    async_cb.reset()
    conf_json.reset()
    logpath = accelize_drm.create_log_path(whoami())
    verbosity = 5
    conf_json['settings']['log_file_verbosity'] = verbosity
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
    drm_manager.set(log_message_level=verbosity)
    msg = 'This line should appear in log file'
    drm_manager.set(log_message=msg)
    del drm_manager
    wait_func_true(lambda: isfile(logpath), 10)
    with open(logpath, 'rt') as f:
        log_content = f.read()
    assert "critical" in log_content
    assert msg in log_content
    async_cb.assert_NoError()
    remove(logpath)
    print("Test parameter 'log_message': PASS")

    # Test parameter: hdk_compatibility
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    hdk_limit = drm_manager.get('hdk_compatibility')
    assert match(r'\d+\.\d', hdk_limit)
    async_cb.assert_NoError()
    print("Test parameter 'hdk_compatibility': PASS")

    # Test parameter: health_period
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('health_period') == 0
    async_cb.assert_NoError()
    print("Test parameter 'health_period': PASS")

    # Test parameter: health_retry
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('health_retry') == drm_manager.get('ws_request_timeout')
    async_cb.assert_NoError()
    print("Test parameter 'health_retry': PASS")

    # Test parameter: health_retry_sleep
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('health_retry_sleep') == drm_manager.get('ws_retry_period_short')
    async_cb.assert_NoError()
    print("Test parameter 'health_retry_sleep': PASS")


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
        conf_json['licensing']['url'] = "http://acme.com"
        conf_json['settings']['ws_request_timeout'] = 5
        conf_json['settings']['ws_retry_period_short'] = 1
        conf_json.save()
        assert conf_json['licensing']['url'] == "http://acme.com"
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

    finally:
        if drm_manager:
            drm_manager.deactivate()
