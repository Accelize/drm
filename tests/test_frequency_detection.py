# -*- coding: utf-8 -*-
"""
Test frequency detetion mechanism.
"""
import pytest
from os import remove, getpid
from os.path import isfile, realpath
from time import sleep, time
from re import search
from datetime import datetime, timedelta


def wait_for(f, timeout=None, sleep_time=1):
    start = datetime.now()
    while not f():
        if timeout:
            if datetime.now() - start > timedelta(seconds=sleep_time):
                return False
        sleep(sleep_time)
    return True


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
    if isfile(logpath):
        remove(logpath)
    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('frequency_detection_method') == 1
        del drm_manager
        assert wait_for(lambda: isfile(logpath), 10)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        assert "Use dedicated counter to compute DRM frequency (method 1)" in log_content
    finally:
        if isfile(logpath):
            remove(logpath)


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
    logpath = realpath("./drmlib.%d.%s.log" % (getpid(), time()))
    if isfile(logpath):
        remove(logpath)
    try:
        driver.program_fpga(image_id)
        conf_json.reset()
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
        assert wait_for(lambda: isfile(logpath), 10)
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
