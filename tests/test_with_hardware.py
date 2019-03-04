"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_with_hardware.py> --cred <path/to/cred.json>
"""

import pytest
from re import search, finditer
from time import sleep
from json import loads, dumps


def getErrorCode( msg ):
    match = search(r'\[errCode=(\d+)\]', msg)
    assert match, "Could not find 'errCode' in exception message: %s" % msg
    return int(match.group(1))


def ordered_json(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered_json(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered_json(x) for x in obj)
    else:
        return obj


def generateCoins( driver, coins ):
    from ctypes import c_uint32, byref
    val = c_uint32()
    for i in range(coins):
        driver.read_register_callback( 0x10000, byref(val) )


def assert_NoErrorCallback( async_cb ):
    assert async_cb.message is None, 'Asynchronous callback reports a message: %s' % async_cb.message
    assert async_cb.errcode is None, 'Asynchronous callback returned error code: %d' % async_cb.errcode
    assert not async_cb.was_called, 'Asynchronous callback has been called.'


@pytest.mark.skip
def test_bad_authentifaction(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when bad authentication parameters are provided to DRM Manager Constructor or Web Service."""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = None
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
        assert getErrorCode(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
        drm_manager.deactivate()
        assert_NoErrorCallback( async_cb )

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
        assert getErrorCode(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code

    finally:
        del drm_manager

#@pytest.mark.skip
def test_configuration_file_with_bad_frequency(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when wrong url is given to DRM Controller Constructor"""

    from math import ceil, floor

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = None

    try:

        # Before any test, get the real DRM frequency and the gap threshold
        async_cb.reset()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        drm_manager.activate()
        sleep(1)
        threshold = drm_manager.get('frequency_detection_threshold')
        frequency = drm_manager.get('drm_frequency')
        drm_manager.deactivate()
        assert_NoErrorCallback( async_cb )
        print('DRM frequency measured=', frequency, ', authorized threshold=', threshold)

        # Test no error is returned by asynchronous error callback when the frequency
        # in configuration file differs from the DRM frequency by less than 2%
        async_cb.reset()
        conf_json.reset()
        init_freq = conf_json['drm']['frequency_mhz']
        conf_json['drm']['frequency_mhz'] = int(floor(frequency * (100.0 + threshold - 1) / 100.0))
        assert abs(conf_json['drm']['frequency_mhz'] - frequency) * 100.0 / frequency < threshold
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
        assert_NoErrorCallback(async_cb)
        print('Test frequency mismatch < threashold: PASS')

        # Test a BADFrequency error is returned by asynchronous error callback when the frequency
        # in configuration file differs from the DRM frequency by more than 2%
        async_cb.reset()
        conf_json.reset()
        init_freq = conf_json['drm']['frequency_mhz']
        conf_json['drm']['frequency_mhz'] = int(ceil(frequency * (100.0 + threshold + 1) / 100.0))
        assert abs(conf_json['drm']['frequency_mhz'] - frequency) * 100.0 / frequency > threshold
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
        assert getErrorCode(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
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
        assert getErrorCode(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
        print('Test frequency overflow: PASS')

    finally:
        del drm_manager

@pytest.mark.skip(reason='WebService is not handling Product ID information yet')
def test_bad_product_id(accelize_drm, conf_json, cred_json):
    """Test errors when an incorrect product ID is requested to License Web Server"""

    driver = accelize_drm.pytest_fpga_driver[0]

    # Test Web Service when a bad product ID is provided
    async_cb.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )

    drm_manager.set( bad_product_id=None )
    pid = drm_manager.get('product_id')
    print('product_id=', pid)

    with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
        drm_manager.activate()
    assert "Authentication credentials" in str(excinfo.value)
    assert getErrorCode(str(excinfo.value)) == accelize_drm.exceptions.DRMWSReqError.error_code

#@pytest.mark.skip
def test_parameter_accesses(accelize_drm, conf_json, cred_json, async_handler):
    """Test accesses to parameter"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    drm_manager = None

    try:
        ## Test parameter: license_type
        ## Read-only: return string with the license type: node-locked, floating/metering
        ## Set node-lock
        #async_cb.reset()
        #cred_json.reset()
        #cred_json.set_user('active_nodelocked_user')
        #cred_json.save()
        #drm_manager = accelize_drm.DrmManager(
        #    conf_json.path,
        #    cred_json.path,
        #    driver.read_register_callback,
        #    driver.write_register_callback,
        #    async_cb.callback
        #)
        #licType = drm_manager.get(license_type)
        #assert licType == 'Node-Locked'
        ## Set floating
        #async_cb.reset()
        #cred_json.reset()
        #cred_json.set_user('metered_user')
        #cred_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        #licType = drm_manager.get(license_type)
        #assert licType == 'Floating/Metering'
        #
        ## Test parameter: nodelocked_request_file
        ## Read-only: return string with the path to the node-locked license request JSON file.
        #async_cb.reset()
        #cred_json.reset()
        #cred_json.set_user('nodelocked')
        #cred_json.save()
        #drm_manager = accelize_drm.DrmManager(
        #    conf_json.path,
        #    cred_json.path,
        #    driver.read_register_callback,
        #    driver.write_register_callback,
        #    async_cb.callback
        #)
        #licRequest = drm_manager.get(license_type)
        #assert len(licRequest) > 0, 'Unexpected size of license request'
        #assert '???' in licRequest, 'Unexpected content of license request'
        #print("Test parameter 'license_type': PASS")

        # Test parameter: license_duration
        # Read-only: return uint32 with the duration in seconds of the current or last license.
        drm_manager.activate()
        licDuration = drm_manager.get('license_duration')
        assert licDuration != 0
        drm_manager.deactivate()
        assert_NoErrorCallback( async_cb )
        print("Test parameter 'license_duration': PASS")

        # Test parameter: num_activators
        # Read-only: return uint32_t/string with the number of activators detected by the DRM controller
        nbActivator = drm_manager.get('num_activators')
        assert nbActivator == 1, 'Unexpected number of activators: ' \
            'function returns %d but expect %d' % (nbActivator, 1)
        print("Test parameter 'num_activators': PASS")

        # Test parameter: session_id
        # Read-only: return string with the current session ID
        drm_manager.activate()
        sessionId = drm_manager.get('session_id')
        assert len(sessionId) == 16, 'Unexpected length of session ID: ' \
            'function returns %d but expect %d' % (len(sessionId), 16)
        drm_manager.deactivate()
        assert_NoErrorCallback( async_cb )
        print("Test parameter 'session_id': PASS")

#        # Test parameter: metering_data
#        # Read-only: return uint64_t or string with the current value of the metering data counter
#        drm_manager.activate()
#        generateCoins( driver, 50 )
#        metered_data = drm_manager.get('metering_data')
#        drm_manager.deactivate()
#        assert_NoErrorCallback( async_cb )
#        assert metered_data == 50, 'Unexpected value of metered data: ' \
#            'function returns %d but expect %d' % (metered_data, 50)
#        print("Test parameter 'metering_data': PASS")

        # Test parameter: page_ctrlreg
        # Read-only: return nothing, print all registers in the DRM Controller Registry page
        page = drm_manager.get('page_ctrlreg')
        assert search(r'Register\s+@0x00:\s+0x00000000', page), 'Unexpected content of page_ctrlreg'
        print("Test parameter 'page_ctrlreg': PASS")

        # Test parameter: page_vlnvfile
        # Read-only: return nothing, print all registers in the VLNV File page
        page = drm_manager.get('page_vlnvfile')
        assert search(r'Register\s+@0x00:\s+0x00000001', page), 'Unexpected content of page_vlnvfile'
        print("Test parameter 'page_vlnvfile': PASS")

        # Test parameter: page_licfile
        # Read-only: return nothing, print all registers in the License File page
        page = drm_manager.get('page_licfile')
        assert search(r'Register\s+@0x00:\s+0x00000002', page), 'Unexpected content of page_licfile'
        print("Test parameter 'page_licfile': PASS")

        # Test parameter: page_tracefile
        # Read-only: return nothing, print all registers in the Trace File page
        page = drm_manager.get('page_tracefile')
        assert search(r'Register\s+@0x00:\s+0x00000003', page), 'Unexpected content of page_tracefile'
        print("Test parameter 'page_tracefile': PASS")

        # Test parameter: page_meteringfile
        # Read-only: return nothing, print all registers in the Metering File page
        page = drm_manager.get('page_meteringfile')
        assert search(r'Register\s+@0x00:\s+0x00000004', page), 'Unexpected content of page_meteringfile'
        print("Test parameter 'page_meteringfile': PASS")

        # Test parameter: page_mailbox
        # Read-only: return nothing, print all registers in the Mailbox page
        page = drm_manager.get('page_mailbox')
        assert search(r'Register\s+@0x00:\s+0x00000005', page), 'Unexpected content of page_mailbox'
        print("Test parameter 'page_mailbox': PASS")

        # Test parameter: hw_report
        # Read-only: return nothing, print the Algodone HW report
        hwReport = drm_manager.get('hw_report')
        nb_lines = len( tuple(finditer(r'\n', hwReport)) )
        assert nb_lines > 10, 'Unexpected HW report content'
        print("Test parameter 'hw_report': PASS")

        # Test parameter: drm_frequency
        # Read-only: return the measured DRM frequency
        freqDrm = drm_manager.get('drm_frequency')
        assert freqDrm == 125, 'Unexpected frequency gap threshold: ' \
            'function returns %d but expect %d' % (freqThrehsold, 2)
        print("Test parameter 'drm_frequency': PASS")

        # Test parameter: frequency_detection_threshold
        # Read-write: read and write frequency gap threshold (in percentage) used to measure the real DRM Controller frequency
        origFreqThrehsold = drm_manager.get('frequency_detection_threshold')    # Save original threshold
        expFreqThrehsold = origFreqThrehsold * 2
        drm_manager.set(frequency_detection_threshold=expFreqThrehsold)
        newFreqThrehsold = drm_manager.get('frequency_detection_threshold')
        assert newFreqThrehsold == expFreqThrehsold, 'Unexpected frequency dectection threshold percentage: ' \
            'function returns %f but expect %f' % (newFreqThrehsold, expFreqThrehsold)
        drm_manager.set(frequency_detection_threshold=origFreqThrehsold)    # Restore original threshold
        print("Test parameter 'frequency_detection_threshold': PASS")

        # Test parameter: frequency_detection_period
        # Read-write: read and write the period of time in milliseconds used to measure the real DRM Controller frequency
        origFreqPeriod = drm_manager.get('frequency_detection_period')    # Save original period
        expFreqPeriod = origFreqPeriod * 2
        drm_manager.set(frequency_detection_period=expFreqPeriod)
        newFreqPeriod = drm_manager.get('frequency_detection_period')
        assert newFreqPeriod == expFreqPeriod, 'Unexpected frequency dectection period percentage: ' \
            'function returns %f but expect %f' % (newFreqPeriod, expFreqPeriod)
        drm_manager.set(frequency_detection_period=origFreqPeriod)    # Restore original period
        print("Test parameter 'frequency_detection_period': PASS")

        # Test parameter: product_id
        # Read-only: return the product ID
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
        # Read-only: return the size of the Mailbox read-write memory in DRM Controller
        mailbox_size = drm_manager.get('mailbox_size')
        assert mailbox_size == 14, 'Unexpected Mailbox size: function returns: ' \
            '"%d" but expect "%d"' % (mailbox_size, 16)
        print("Test parameter 'mailbox_size': PASS")

        # Test parameter: custom_field
        # Read-write: only for testing, any uint32_t register accessible to the user for any purpose.
        import random
        valexp = random.randint(0,0xFFFFFFFF)
        valinit = drm_manager.get('custom_field')
        assert valexp != valinit
        drm_manager.set(custom_field=valexp)
        valback = drm_manager.get('custom_field')
        assert valexp == valback
        print("Test parameter 'custom_field': PASS")

# NEED LGDN TO FIX THE ISSUE
#        # Test parameter: mailbox_data
#        # Read-write: only for testing, read or write values to Mailbox read-write memory in DRM Controller
#        import random
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
        print("Test parameter 'retry_deadline': PASS")

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
        print("Test parameter 'trigger_async_callback': PASS")

        # Test parameter: bad_authentication_token
        # Write-only: only for testing, uses a bad authentication token.
        # => Skipped: Tested in test_bad_authentifaction

        # Test parameter: bad_product_id
        # Write-only: only for testing, uses a bad product ID.
        # => Skipped: Tested in test_bad_product_id

    finally:
        del drm_manager

@pytest.mark.skip(reason='Wait until ACA creates the user and associated entitlments in DB')
def test_nodelock_cases(accelize_drm, conf_json, cred_json, async_handler):
    """Test node-locked cases using 1 FPGA"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test an inactive node-locked user cannot get a license
    async_cb.reset()
    cred_json.reset()
    cred_json.set_user('inactive_nodelocked_user')
    cred_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb
    )
    licType = drm_manager.get(license_type)
    assert licType == 'Node-Locked'
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    assert '???' in str(excinfo.value)
    assert getErrorCode(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    drm_manager.deactivate()
    assert_NoErrorCallback( async_cb )

    # Test an active node-locked user can get a license
    async_cb.reset()
    cred_json.reset()
    cred_json.set_user('active_nodelocked_user')
    cred_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )
    licType = drm_manager.get(license_type)
    assert licType == 'Node-Locked'
    drm_manager.activate()
    generateCoins(50)
    metered_data = drm_manager.get('metering_data')
    drm_manager.deactivate()
    assert_NoErrorCallback( async_cb )
    assert metered_data == 50, 'Unexpected value of metered data: ' \
        'function returns %d but expect %d' % (metered_data, 50)

@pytest.mark.integration
@pytest.mark.skip
def test_nodelock_limit(accelize_drm, conf_json, cred_json, async_handler):
    """Test node-locked cases using 2 FPGA: cannot be handled yet
    because there is currently no way to instantiate dynamically
    2 drm managers addressing 2 different slots"""

    driver1 = accelize_drm.pytest_fpga_driver[0]
    driver2 = accelize_drm.pytest_fpga_driver[1]

    async_cb = async_handler.create()

    # Test first node-locked user can get a license
    async_cb.reset()
    cred_json.reset()
    cred_json.set_user('active_nodelocked_user')
    cred_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb
    )
    licType = drm_manager.get(license_type)
    assert licType == 'Node-Locked'
    drm_manager.activate()
    for i in range(50):
        drm_manager.get('custom_field')
    metered_data = drm_manager.get('metering_data')
    drm_manager.deactivate()
    assert_NoErrorCallback( async_cb )
    assert metered_data == 50, 'Unexpected value of metered data: ' \
        'function returns %d but expect %d' % (metered_data, 50)

    # Test a second node-locked user cannot get a license
    async_cb.reset()
    cred_json.reset()
    cred_json.set_user('active_nodelocked_user')
    cred_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver2.read_register_callback,
        driver2.write_register_callback
    )
    licType = drm_manager.get(license_type)
    assert licType == 'Node-Locked'
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    assert '???' in str(excinfo.value)

@pytest.mark.skip(reason='Not sure what is the behavior of the DRM Manager in case of 2 instances: seems to create issues like the timerEnable timeout')
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

