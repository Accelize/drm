"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_with_hardware.py> --cred <path/to/cred.json> --library_verbosity=3 --server='dev' --backend='c++' -s
"""

import pytest
from re import search, finditer
from time import sleep
from json import loads
from datetime import datetime, timedelta
from ctypes import c_uint32, byref

ACTIVATOR_0_BASE_ADDR = 0x10000
ACTIVATOR_RANGE_ADDR = 0x10000

_PARAM_LIST = ['license_type',
               'license_duration',
               'num_activators',
               'session_id',
               'session_status',
               'license_status',
               'metered_data',
               'nodelocked_request_file',
               'drm_frequency',
               'product_id',
               'mailbox_size',
               'log_verbosity',
               'log_format',
               'frequency_detection_threshold',
               'frequency_detection_period',
               'custom_field',
               'mailbox_data',
               'retry_deadline',
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
               'bad_authentication_token',
               'bad_product_id',
               'log_message',
           ]


def ordered_json(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered_json(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered_json(x) for x in obj)
    else:
        return obj


def generate_coin(driver, activator_index, coins):
    value = c_uint32()
    for i in range(coins):
        driver.read_register_callback( ACTIVATOR_0_BASE_ADDR +
                       activator_index * ACTIVATOR_RANGE_ADDR, byref(value) )


def get_activator_status( driver, activator_index=None ):
    if isinstance(activator_index, list):
        activator_index_list = activator_index
    elif isinstance(activator_index, int):
        activator_index_list = [activator_index]
    else:
        raise TypeError('Unsupported type: %s' % type(activator_index))
    regvalue = c_uint32(0)
    status_list = []
    for i in activator_index_list:
        driver.read_register_callback( ACTIVATOR_0_BASE_ADDR +
                       i * ACTIVATOR_RANGE_ADDR, byref(regvalue) )
        value = regvalue.value
        code_rdy = (value >> 1) & 1
        active = value & 1
        status_list.append(active)
    if len(status_list) == 1 and activator_index is None or isinstance(activator_index, int):
        return status_list[0]
    return status_list


#@pytest.mark.skip
def test_logging(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging mechanism and enhance coverage"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test logging with long format
    async_cb.reset()
    conf_json['settings']['log_format'] = 1
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
        drm_manager.set(log_message_level=5)
        msg = 'This should be a DEBUG2 message'
        drm_manager.set(log_message=msg)
#        assert 'DEBUG2' in captured.out
#        assert msg in captured.out
        async_cb.assert_NoError()

    finally:
        drm_manager.deactivate()

#@pytest.mark.skip
def test_configuration_file_with_bad_authentication(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when bad authentication parameters are provided to
    DRM Manager Constructor or Web Service."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = None
    print()
    try:
        # Test when authentication url in configuration file is wrong
        async_cb.reset()
        conf_json.reset()
        conf_json['licensing']['url'] = "http://accelize.com"
        conf_json.save()
        assert conf_json['licensing']['url'] == "http://accelize.com"
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager.activate()
        assert "WSOAuth HTTP response code" in str(excinfo.value)
        assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
        async_cb.assert_NoError()

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
        drm_manager.set( bad_authentication_token=None )
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert "Authentication credentials" in str(excinfo.value)
        assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code

    finally:
        if drm_manager:
            drm_manager.deactivate()

#@pytest.mark.skip
def test_configuration_file_with_bad_frequency(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when wrong url is given to DRM Controller Constructor"""

    from math import ceil, floor

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

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
    print('Test frequency mismatch < threashold: PASS')

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
    print('Test frequency mismatch > threashold: PASS')

    # TODO: Remove the following line when web service is handling it
    return

    # Test web service detects a frequency underflow
    pytest.xfail('Web service is not checking DRM frequency underflow yet')
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = 10
    conf_json.save()
    assert conf_json['drm']['frequency_mhz'] == 10

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    assert '???' in str(excinfo.value)
    assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    print('Test frequency underflow: PASS')

    # Test web service detects a frequency overflow
    pytest.xfail('Web service is not checking DRM frequency overflow yet')
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
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    assert '???' in str(excinfo.value)
    assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    print('Test frequency overflow: PASS')


@pytest.mark.skip(reason='WebService is not handling Product ID information yet')
def test_configuration_file_bad_product_id(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when an incorrect product ID is requested to License Web Server"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test Web Service when a bad product ID is provided
    async_cb.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    drm_manager.set( bad_product_id=None )
    pid = drm_manager.get('product_id')

    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert "Authentication credentials" in str(excinfo.value)
    assert async_handler.parse_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code
    async_cb.assert_NoError()

#@pytest.mark.skip
def test_parameter_key_modification_with_config_file(accelize_drm, conf_json, cred_json, async_handler):
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
    origLogVerbosity = drm_manager.get('log_verbosity')
    origLogFormat = drm_manager.get('log_format')
    origFrequencyMHz = drm_manager.get('drm_frequency')
    origFrequencyDetectPeriod = drm_manager.get('frequency_detection_period')
    origFrequencyDetectThreshold = drm_manager.get('frequency_detection_threshold')
    origRetryDeadline = drm_manager.get('retry_deadline')

    # Test parameter: log_verbosity
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    from random import choice
    async_cb.reset()
    conf_json.reset()
    logLevelChoice = list(range(0,6))
    logLevelChoice.remove(origLogVerbosity)
    expValue = choice(logLevelChoice)
    assert expValue != origLogVerbosity
    conf_json['settings']['log_verbosity'] = expValue
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('log_verbosity')
    assert value == expValue
    drm_manager.set(log_verbosity=origLogVerbosity)
    print("Test parameter 'log_verbosity': PASS")

    # Test parameter: log_format
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    async_cb.reset()
    conf_json.reset()
    expValue = 1
    assert expValue != origLogFormat
    conf_json['settings']['log_format'] = expValue
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('log_format')
    assert value == expValue
    async_cb.assert_NoError()
    print("Test parameter 'log_format': PASS")

    # Test parameter: drm_frequency
    # Read-only, return the measured DRM frequency
    async_cb.reset()
    conf_json.reset()
    expValue = 2*origFrequencyMHz
    conf_json['drm']['frequency_mhz'] = expValue
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('drm_frequency')
    assert value == expValue
    async_cb.assert_NoError()
    print("Test parameter 'frequency_mhz': PASS")

    # Test parameter: frequency_detection_period
    # Read-write: read and write the period of time in milliseconds used to measure the real DRM Controller frequency
    async_cb.reset()
    conf_json.reset()
    expValue = 2*origFrequencyDetectPeriod
    conf_json['settings'] = {'frequency_detection_period': expValue}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('frequency_detection_period')
    assert value == expValue
    async_cb.assert_NoError()
    print("Test parameter 'frequency_detection_period': PASS")

    # Test parameter: frequency_detection_threshold
    # Read-write: read and write frequency gap threshold (in percentage) used to measure the real DRM Controller frequency
    async_cb.reset()
    conf_json.reset()
    expValue = 2*origFrequencyDetectThreshold
    conf_json['settings'] = {'frequency_detection_threshold': expValue}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('frequency_detection_threshold')
    assert value == expValue
    async_cb.assert_NoError()
    print("Test parameter 'frequency_detection_threshold': PASS")

    # Test parameter: retry_deadline
    # Read-write: read and write the retry period deadline in seconds from the license timeout during which no more retry is sent
    async_cb.reset()
    conf_json.reset()
    expValue = 2*origRetryDeadline
    conf_json['settings'] = {'retry_deadline': expValue}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    value = drm_manager.get('retry_deadline')
    assert value == expValue
    async_cb.assert_NoError()
    print("Test parameter 'retry_deadline': PASS")

    # Test unsupported parameter
    # Verify the parameter is simply ignored
    async_cb.reset()
    conf_json.reset()
    conf_json['settings'] = {'unsupported_param': 10.2}
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    async_cb.assert_NoError()
    print("Test empty parameter: PASS")

#@pytest.mark.skip
def test_parameter_key_modification_with_get_set(accelize_drm, conf_json, cred_json, async_handler):
    """Test accesses to parameter"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    print()

    # Test parameter: log_verbosity
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    orig_verbosity = drm_manager.get('log_verbosity')
    if orig_verbosity == 5:
        new_verbosity = 4
    else:
        new_verbosity = 5
    verbosity = drm_manager.set(log_verbosity=new_verbosity)
    verbosity = drm_manager.get('log_verbosity')
    assert verbosity == new_verbosity
    verbosity = drm_manager.set(log_verbosity=orig_verbosity)
    async_cb.assert_NoError()
    print("Test parameter 'log_verbosity': PASS")

    # Test parameter: log_format
    # Read-write, read and write the logging verbosity: 0=quiet, 5=debug2
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    format = drm_manager.get('log_format')
    assert format == 0
    format = drm_manager.set(log_format=4)
    format = drm_manager.get('log_format')
    assert format == 4
    async_cb.assert_NoError()
    print("Test parameter 'log_format': PASS")

#    # Test parameter: license_type in nodelocked and nodelocked_request_file
#    # license_type: Read-only, return string with the license type: node-locked, floating/metering
#    # nodelocked_request_file: Read-only, return string with the path to the node-locked license request JSON file.
#    async_cb.reset()
#    cred_json.reset()
#    cred_json.set_user('nodelocked')
#    cred_json.save()
#    drm_manager = accelize_drm.DrmManager(
#        conf_json.path,
#        cred_json.path,
#        driver.read_register_callback,
#        driver.write_register_callback,
#        async_cb.callback
#    )
#    licType = drm_manager.get('license_type')
#    assert licType == 'Node-Locked'
#    licRequest = drm_manager.get('license_type')
#    assert len(licRequest) > 0, 'Unexpected size of license request'
#    assert '???' in licRequest, 'Unexpected content of license request'
#    print("Test parameter 'license_type' and 'nodelocked_request_file' in Node-Locked: PASS")

    # Test parameter: license_type in metering
    # Read-only, return string with the license type: node-locked, floating/metering
    async_cb.reset()
    cred_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    licType = drm_manager.get('license_type')
    assert licType == 'Floating/Metering'
    async_cb.assert_NoError()
    print("Test parameter 'license_type' in Metered: PASS")

    # Test parameter: license_duration
    # Read-only, return uint32 with the duration in seconds of the current or last license.
    drm_manager.activate()
    licDuration = drm_manager.get('license_duration')
    assert licDuration != 0
    drm_manager.deactivate()
    async_cb.assert_NoError()
    print("Test parameter 'license_duration': PASS")

    # Test parameter: num_activators
    # Read-only, return uint32_t/string with the number of activators detected by the DRM controller
    nbActivator = drm_manager.get('num_activators')
    assert nbActivator == 1, 'Unexpected number of activators'
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
    sessionState = drm_manager.get('session_status')
    assert not sessionState
    drm_manager.activate()
    sessionState = drm_manager.get('session_status')
    assert sessionState
    drm_manager.deactivate()
    sessionState = drm_manager.get('session_status')
    assert not sessionState
    async_cb.assert_NoError()
    print("Test parameter 'session_status': PASS")

    # Test parameter: license_status
    # Read-only, return the current license status
    licenseState = drm_manager.get('license_status')
    assert not licenseState
    drm_manager.activate()
    licenseState  = drm_manager.get('license_status')
    assert licenseState
    drm_manager.deactivate()
    licenseState  = drm_manager.get('license_status')
    assert not licenseState
    async_cb.assert_NoError()
    print("Test parameter 'license_status': PASS")

    # Test parameter: metered_data
    # Read-only, return uint64_t or string with the current value of the metering data counter
    drm_manager.activate()
    generate_coin(driver, 0, 10)
    metered_data = drm_manager.get('metered_data')
    assert metered_data > 0, 'Unexpected metered_data'
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
    hwReport = drm_manager.get('hw_report')
    nb_lines = len( tuple(finditer(r'\n', hwReport)) )
    assert nb_lines > 10, 'Unexpected HW report content'
    print("Test parameter 'hw_report': PASS")

    # Test parameter: frequency_detection_threshold
    # Read-write: read and write frequency gap threshold (in percentage) used to measure the real DRM Controller frequency
    origFreqThrehsold = drm_manager.get('frequency_detection_threshold')    # Save original threshold
    expFreqThrehsold = origFreqThrehsold * 2
    drm_manager.set(frequency_detection_threshold=expFreqThrehsold)
    newFreqThrehsold = drm_manager.get('frequency_detection_threshold')
    assert newFreqThrehsold == expFreqThrehsold, 'Unexpected frequency dectection threshold percentage'
    drm_manager.set(frequency_detection_threshold=origFreqThrehsold)    # Restore original threshold
    print("Test parameter 'frequency_detection_threshold': PASS")

    # Test parameter: frequency_detection_period
    # Read-write: read and write the period of time in milliseconds used to measure the real DRM Controller frequency
    origFreqPeriod = drm_manager.get('frequency_detection_period')    # Save original period
    expFreqPeriod = origFreqPeriod * 2
    drm_manager.set(frequency_detection_period=expFreqPeriod)
    newFreqPeriod = drm_manager.get('frequency_detection_period')
    assert newFreqPeriod == expFreqPeriod, 'Unexpected frequency dectection period'
    drm_manager.set(frequency_detection_period=origFreqPeriod)    # Restore original period
    print("Test parameter 'frequency_detection_period': PASS")

    # Test parameter: drm_frequency
    # Read-only, return the measured DRM frequency
    freqPeriod = drm_manager.get('frequency_detection_period')    # Save original period
    freqthreshold = drm_manager.get('frequency_detection_threshold')    # Save original period
    drm_manager.activate()
    sleep(2.0*freqPeriod/1000)
    freqDrm = drm_manager.get('drm_frequency')
    drm_manager.deactivate()
    assert freqDrm == 125, 'Unexpected frequency gap threshold'
    print("Test parameter 'drm_frequency': PASS")

    # Test parameter: product_id
    # Read-only, return the product ID
    from pprint import pformat
    productID = pformat(drm_manager.get('product_id'))
    expProductID = pformat(loads("""{
            "vendor": "accelize.com",
            "library": "refdesign",
            "name": "drm_1activator",
            "sign":"31f4b20548d39d9cc8895ec7a52e68f0"
        }"""))
    assert productID == expProductID, 'Unexpected product ID'
    print("Test parameter 'product_id': PASS")

    # Test parameter: mailbox_size
    # Read-only, return the size of the Mailbox read-write memory in DRM Controller
    mailbox_size = drm_manager.get('mailbox_size')
    assert mailbox_size == 14, 'Unexpected Mailbox size'
    print("Test parameter 'mailbox_size': PASS")

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
    valexp = randint(0,0xFFFFFFFF)
    valinit = drm_manager.get('custom_field')
    assert valexp != valinit
    drm_manager.set(custom_field=valexp)
    valback = drm_manager.get('custom_field')
    assert valexp == valback
    print("Test parameter 'custom_field': PASS")

# NEED LGDN TO FIX THE ISSUE
#        # Test parameter: mailbox_data
#        # Read-write: only for testing, read or write values to Mailbox read-write memory in DRM Controller
#        from random import randint
#        mailbox_size = drm_manager.get('mailbox_size')
#        wr_msg = [random.randint(0,0xFFFFFFFF) for i in range(mailbox_size)]
#        drm_manager.set(mailbox_data=wr_msg)
#        rd_msg = drm_manager.get('mailbox_data')
#        assert type(rd_msg) == type(wr_msg) == list, 'Bad type returned by get(mailbox_data)'
#        assert rd_msg == wr_msg, 'Failed to write-read Mailbox'
#        print("Test parameter 'mailbox_data': PASS")

    # Test parameter: retry_deadline
    # Read-write: read and write the retry period deadline in seconds from the license timeout during which no more retry is sent
    origRetryDeadline = drm_manager.get('retry_deadline')  # Save original value
    expRetryDeadline = origRetryDeadline + 100
    drm_manager.set(retry_deadline=expRetryDeadline)
    newRetryDeadline = drm_manager.get('retry_deadline')
    assert newRetryDeadline == expRetryDeadline, 'Failed to write-read retry_deadline parameter'
    drm_manager.set(retry_deadline=origRetryDeadline)  # Restore original value
    async_cb.assert_NoError(async_cb.assert_NoError)
    print("Test parameter 'retry_deadline': PASS")

    # Test parameter: log_message_level
    # Read-write, only for testing, read and write the log level used with log_message parameter to set the message level
    level = drm_manager.get('log_message_level')
    assert level == 0
    expLevel = 5
    drm_manager.set(log_message_level=expLevel)
    level = drm_manager.get('log_message_level')
    assert level == expLevel
    async_cb.assert_NoError()
    print("Test parameter 'log_message_level': PASS")

    # Test parameter: trigger_async_callback
    # Write-only: only for testing, call the asynchronous error callback with the given message.
    drm_manager.activate()
    sleep(1)
    test_message = 'Test message'
    drm_manager.set(trigger_async_callback=test_message)
    assert async_cb.was_called, 'Asynchronous callback has not been called.'
    assert async_cb.message is not None, 'Asynchronous callback did not report any message'
    assert test_message in async_cb.message, \
        'Asynchronous callback has not received the correct message'
    assert async_cb.errcode == accelize_drm.exceptions.DRMDebug.error_code, \
        'Asynchronous callback has not received the correct error code'
    drm_manager.deactivate()
    print("Test parameter 'trigger_async_callback': PASS")

    # Test parameter: bad_authentication_token
    # Write-only: only for testing, uses a bad authentication token.
    # => Skipped: Tested in test_bad_authentifaction

    # Test parameter: bad_product_id
    # Write-only: only for testing, uses a bad product ID.
    # => Skipped: Tested in test_bad_product_id

    # Test parameter: log_message
    # Write-only, only for testing, insert a message with the value as content
    from time import time
    from os.path import isfile
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager.set(log_message_level=5)
    msg = 'This should be DEBUG2 message'
    drm_manager.set(log_message=msg)
    async_cb.assert_NoError()
    print("Test parameter 'log_message': PASS")


@pytest.mark.skip(reason='Not sure what is the expected behavior of the DRM Manager in case of 2 instances: seems to create issues like the timerEnable timeout')
def test_2_drm_manager_concurrently(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when 2 DrmManager instances are used."""

    driver = accelize_drm.pytest_fpga_driver[0]

    async_cb1 = async_handler.create()
    async_cb2 = async_handler.create()

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

#@pytest.mark.skip
def test_activation_and_license_status(accelize_drm, conf_json, cred_json, async_handler):
    """Test status of IP activators"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        print()
        nb_activators = drm_manager.get('num_activators')
        activator_indexes = list(range(nb_activators))

        # Test license status on start/stop

        # Check all activators are locked
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        # Activate all activators
        drm_manager.activate()
        # Check all activators are unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Deactivate all activators
        drm_manager.deactivate()
        # Check all activators are locked again
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on start/stop: PASS')

        # Test license status on start/pause

        # Check all activators are locked
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        # Activate all activators
        drm_manager.activate()
        start = datetime.now()
        # Check all activators are unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Pause all activators
        drm_manager.deactivate( True )
        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license status on start/pause: PASS')

        # Test license status on resume from valid license/pause

        # Check all activators are unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Resume all activators
        drm_manager.activate( True )
        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Pause all activators
        drm_manager.deactivate( True )
        # Check all activators are still unlocked
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Wait until license expires
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=2*lic_duration+1) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check all activators are now locked again
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on resume from valid license/pause: PASS')

        # Test license status on resume from expired license/pause

        # Check all activators are locked
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        # Resume all activators
        drm_manager.activate( True )
        # Check all activators are unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Pause all activators
        drm_manager.deactivate( True )
        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on resume from expired license/pause: PASS')

        # Test license status on resume/stop

        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        # Resume all activators
        drm_manager.activate( True )
        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Deactivate all activators
        drm_manager.deactivate()
        # Check all activators are locked again
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        print('Test license status on resume/stop: PASS')

        # Test license status on restart from paused session/stop

        # Check all activators are locked again
        lic_status = drm_manager.get('license_status')
        assert not lic_status, 'License is not inactive'
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is unlocked'
        async_cb.assert_NoError()
        # Activate all activators
        drm_manager.activate()
        # Check all activators are unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Pause activators
        drm_manager.deactivate( True )
        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        # Restart all activators
        drm_manager.activate()
        # Check all activators are still unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status, 'License is not active'
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license status on restart: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()

#@pytest.mark.skip
def test_session_status(accelize_drm, conf_json, cred_json, async_handler):
    """Test status of session"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        print()
        nb_activators = drm_manager.get('num_activators')

        # Test session status on start/stop

        # Check no session is running and no ID is available
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(id) == 0, 'A session ID exists'
        # Activate new session
        drm_manager.activate()
        # Check a session is running with a valid ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        # Deactivate current session
        drm_manager.deactivate()
        # Check session is closed
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(id) == 0, 'A session ID exists'
        print('Test session status on start/stop: PASS')

        # Test session status on start/pause

        # Check no session is running and no ID is available
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(id) == 0, 'A session ID exists'
        # Activate new session
        drm_manager.activate()
        start = datetime.now()
        # Check a session is running with a valid ID
        status = drm_manager.get('session_status')
        id_ref = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id_ref) == 16, 'No session ID is returned'
        # Pause current session
        drm_manager.deactivate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        async_cb.assert_NoError()
        print('Test session status on start/pause: PASS')

        # Test session status on resume from valid license/pause

        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Resume current session
        drm_manager.activate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Pause current session
        drm_manager.deactivate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Wait until license expires
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=2*lic_duration+1) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        async_cb.assert_NoError()
        print('Test session status on resume from valid license/pause: PASS')

        # Test session status on resume from expired license/pause

        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Resume current session
        drm_manager.activate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Pause current session
        drm_manager.deactivate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        async_cb.assert_NoError()
        print('Test session status on resume from expired license/pause: PASS')

        # Test session status on resume/stop

        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Resume current session
        drm_manager.activate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Close session
        drm_manager.deactivate()
        # Check session is closed
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(id) == 0, 'A session ID exists'
        async_cb.assert_NoError()
        print('Test session status on resume/stop: PASS')

        # Test session status on start from paused session/stop

        # Check no session is running
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(id) == 0, 'A session ID exists'
        # Start a new session
        drm_manager.activate()
        # Check a session is alive with a new ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id != id_ref, 'Return different session ID'
        id_ref = id
        # Pause session
        drm_manager.deactivate( True )
        # Check a session is still alive with the same ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id == id_ref, 'Return different session ID'
        # Start a new session
        drm_manager.activate()
        # Check a new session has been created with a new ID
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert status, 'No session is running'
        assert len(id) == 16, 'No session ID is returned'
        assert id != id_ref, 'Return different session ID'
        id_ref = id
        # Close session
        drm_manager.deactivate()
        # Check session is closed
        status = drm_manager.get('session_status')
        id = drm_manager.get('session_id')
        assert not status, 'A session is running'
        assert len(id) == 0, 'A session ID exists'
        async_cb.assert_NoError()
        print('Test session status on restart: PASS')

    finally:
        if drm_manager:
            drm_manager.deactivate()


#@pytest.mark.skip
def test_license_expiration(accelize_drm, conf_json, cred_json, async_handler):
    """Test license expiration"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    try:
        print()
        nb_activators = drm_manager.get('num_activators')
        activator_indexes = list(range(nb_activators))

        # Test license expires after 2 duration periods when start/pause

        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        # Start
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        # Pause
        sleep(lic_duration/2)
        drm_manager.deactivate( True )
        # Check license is still running and activator are all unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Wait right before expiration
        wait_period = start + timedelta(seconds=2*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running and activators are all unlocked
        lic_status = drm_manager.get('license_status')
        assert lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Wait a bit more time the expiration
        sleep(3)
        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        drm_manager.deactivate()
        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license expires after 2 duration periods when start/pause/stop')

        # Test license does not expire after 3 duration periods when start

        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        # Start
        drm_manager.activate()
        start = datetime.now()
        # Check license is running
        lic_status = drm_manager.get('license_status')
        assert lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Wait 3 duration periods
        lic_duration = drm_manager.get('license_duration')
        wait_period = start + timedelta(seconds=3*lic_duration+2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        lic_status = drm_manager.get('license_status')
        assert lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Stop
        drm_manager.deactivate()
        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        async_cb.assert_NoError()
        print('Test license does not expire after 3 duration periods when start')

        # Test license does not expire after 3 duration periods when start/pause

        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        # Start
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        # Check license is running
        lic_status = drm_manager.get('license_status')
        assert lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Wait 1 duration period
        wait_period = start + timedelta(seconds=lic_duration+2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check license is still running
        lic_status = drm_manager.get('license_status')
        assert lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert all(ip_status), 'At least one activator is unlocked'
        # Pause
        drm_manager.deactivate( True )
        # Wait right before the next 2 duration periods expire
        wait_period = start + timedelta(seconds=3*lic_duration-2) - datetime.now()
        sleep(wait_period.total_seconds())
        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        # Wait a bit more time the expiration
        sleep(3)
        # Check license has expired
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        drm_manager.deactivate()
        # Check no license is running
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        ip_status = get_activator_status( driver, activator_indexes )
        assert not any(ip_status), 'At least one activator is locked'
        async_cb.assert_NoError()

    finally:
        if drm_manager:
            drm_manager.deactivate()


#@pytest.mark.skip
def test_multiple_call(accelize_drm, conf_json, cred_json, async_handler):
    """Test multiple calls to activate and deactivate"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

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
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        # Start
        drm_manager.activate()
        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        # Resume
        drm_manager.activate( True )
        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Check a session is valid
        session_id2 = drm_manager.get('session_id')
        assert len(session_id2) == 16
        assert session_id2 == session_id
        # Start again
        drm_manager.activate()
        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        assert session_id != session_id2
        # Start again
        drm_manager.activate()
        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Check a session is valid
        session_id2 = drm_manager.get('session_id')
        assert len(session_id2) == 16
        assert session_id2 != session_id
        async_cb.assert_NoError()

        # Test multiple deactivate

        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Pause
        drm_manager.deactivate( True )
        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        assert session_id == session_id2
        # Resume
        drm_manager.deactivate( True )
        # Check license is active
        lic_status = drm_manager.get('license_status')
        assert lic_status
        # Check a session is valid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 16
        assert session_id == session_id2
        # Stop
        drm_manager.deactivate()
        # Check license is in active
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        # Check session ID is invalid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 0
        # Stop
        drm_manager.deactivate()
        # Check license is in active
        lic_status = drm_manager.get('license_status')
        assert not lic_status
        # Check session ID is invalid
        session_id = drm_manager.get('session_id')
        assert len(session_id) == 0
        async_cb.assert_NoError()

    finally:
        if drm_manager:
            drm_manager.deactivate()
