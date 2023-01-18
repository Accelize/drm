# -*- coding: utf-8 -*-
"""
Test frequency detetion mechanism.
"""
import pytest
from time import sleep
from re import search


def test_configuration_file_with_bad_frequency(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when wrong frequency is given to DRM Controller Constructor"""
    from math import ceil, floor
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    conf_json.reset()

    # Before any test, get the real DRM frequency and the gap threshold
    async_cb.reset()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        freq_threshold = drm_manager.get('frequency_detection_threshold')
        freq_period = drm_manager.get('frequency_detection_period')
        drm_manager.activate()
        sleep(2)
        frequency = drm_manager.get('drm_frequency')
        drm_manager.deactivate()

    # Test no error is returned by asynchronous error callback when the frequency
    # in configuration file differs from the DRM frequency by less than the threshold
    async_cb.reset()
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = int(floor(frequency * (100.0 + freq_threshold/2) / 100.0))
    conf_json.save()
    assert abs(conf_json['drm']['frequency_mhz'] - frequency) * 100.0 / frequency < freq_threshold

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        sleep(2)
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

    if accelize_drm.pytest_freq_detection_version != 0xFFFFFFFF:
        with pytest.raises(accelize_drm.exceptions.DRMBadFrequency) as excinfo:
            accelize_drm.DrmManager(
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
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
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

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Accelize Web Service error 422' in str(excinfo.value)
        assert 'ensure this value is greater than or equal to 50' in str(excinfo.value)
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

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert 'Accelize Web Service error 422' in str(excinfo.value)
        assert 'ensure this value is less than or equal to 350' in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMWSReqError.error_code
        print('Test frequency overflow: PASS')


@pytest.mark.minimum
def test_drm_manager_frequency_detection_method1(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """Test method 1 (based on license timer) to estimate drm frequency is still working"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")
    refdesign = accelize_drm.pytest_ref_designs
    if refdesign is None:
        pytest.skip("No refdesign with HDK v3.x could be found in the testsuite")

    refdesign = accelize_drm.pytest_ref_designs
    if refdesign is None:
        pytest.skip("No refdesign with HDK v3.x could be found in the testsuite")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    fpga_image_bkp = driver.fpga_image
    image_id = None
    try:
        if accelize_drm.pytest_freq_detection_version != 0xFFFFFFFF:
            # Program FPGA with HDK 3.2.x (with frequency detection method 1)
            hdk = list(filter(lambda x: x.startswith('3.2'), refdesign.hdk_versions))
            if len(hdk) == 0:
                pytest.skip("No refdesign with HDK v3.x could be found in the testsuite")
            hdk = hdk[-1]
            assert hdk.startswith('3.')
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)

        conf_json.reset()
        logfile = log_file_factory.create(1)
        conf_json['settings'].update(logfile.json)
        conf_json.save()
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            assert drm_manager.get('frequency_detection_method') == 1
            drm_manager.activate()
            assert drm_manager.get('frequency_detection_method') == 1
            drm_manager.deactivate()
        log_content = logfile.read()
        assert "Use license timer counter to compute DRM frequency (method 1)" in log_content
        logfile.remove()
    finally:
        if image_id:
            # Reprogram FPGA with original image
            driver.program_fpga(fpga_image_bkp)


def test_drm_manager_frequency_detection_method2(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """Test method2 (based on dedicated counter in AXI wrapper) to estimate drm_aclk frequency is working"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")
    if accelize_drm.pytest_freq_detection_version != 0x60DC0DE0:
        pytest.skip("Frequency detection method 2 is not implemented in this design: test skipped")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert drm_manager.get('frequency_detection_method') == 2
        drm_manager.activate()
        assert drm_manager.get('frequency_detection_method') == 2
        drm_manager.deactivate()
    log_content = logfile.read()
    assert "Use dedicated counter to compute DRM frequency (method 2)" in log_content
    assert "Frequency detection of drm_aclk counter after" in log_content
    logfile.remove()


@pytest.mark.minimum
def test_drm_manager_frequency_detection_method3(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """Test method 3 (based on dedicated counter in AXI wrapper) to estimate drm_aclk and s_axi_aclk frequencies is working"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    fpga_image_bkp = driver.fpga_image
    image_id = None
    try:
        if accelize_drm.pytest_freq_detection_version != 0x60DC0DE1:
            # Program FPGA with HDK 4.2.1.2 (with frequency detection method 3)
            refdesign = accelize_drm.pytest_ref_designs
            hdk = list(filter(lambda x: x.startswith('4.2.1.2'), refdesign.hdk_versions))
            if len(hdk) == 0:
                pytest.skip("No refdesign with HDK v4.2.1.2 could be found in the testsuite")
            hdk = hdk[-1]
            assert hdk.startswith('4.2.1.2')
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)

        conf_json.reset()
        logfile = log_file_factory.create(1)
        conf_json['settings'].update(logfile.json)
        conf_json.save()
        with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
            assert drm_manager.get('frequency_detection_method') == 3
            drm_manager.activate()
            assert drm_manager.get('frequency_detection_method') == 3
            drm_manager.deactivate()
        log_content = logfile.read()
        assert "Use dedicated counter to compute DRM frequency (method 3)" in log_content
        assert "Frequency detection of drm_aclk counter after" in log_content
        assert "Frequency detection of s_axi_aclk counter after" in log_content
        logfile.remove()
    finally:
        if image_id:
            # Reprogram FPGA with original image
            driver.program_fpga(fpga_image_bkp)


def test_drm_manager_frequency_detection_method_2_and_3_exception(accelize_drm, conf_json, cred_json, async_handler):
    """Test method 2 and 3 (based on dedicated counter in AXI wrapper) to estimate drm aclk frequency is working"""
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")
    if accelize_drm.pytest_freq_detection_version == 0xFFFFFFFF:
        pytest.skip("Frequency detection method 2 and 3 are not implemented in this design: test skipped")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    conf_json.reset()
    conf_json['settings']['frequency_detection_period'] = (int)(2**32 / 121000000 * 1000) + 1000
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadFrequency) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'Frequency auto-detection .*? failed: frequency_detection_period parameter \([^)]+\) is too long',
                  str(excinfo.value)) is not None
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadFrequency.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadFrequency.error_code, 'Frequency auto-detection of drm_aclk failed')
    async_cb.reset()


def test_drm_manager_frequency_detection_bypass(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """Test bypass of frequency detection"""

    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")

    if accelize_drm.pytest_freq_detection_version != 0xFFFFFFFF:
        pytest.skip("New frequency detection method is not supported: test skipped")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test when bypass = True
    conf_json.reset()
    conf_json['drm']['bypass_frequency_detection'] = True
    conf_json['drm']['frequency_mhz'] = 80
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('drm_frequency') == 80
        drm_manager.activate()
        sleep(1)
        assert drm_manager.get('drm_frequency') == 80
        log_content = logfile.read()
        assert search(r'\[\s*warning\s*\] .*? DRM frequency auto-detection is disabled: .*? will be used to compute license timers', log_content)
        logfile.remove()
        drm_manager.deactivate()
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
