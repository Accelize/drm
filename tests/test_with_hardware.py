"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_function_arguments.py> --cred <path/to/cred.json>
"""

import pytest
import re
import time


def test_drm_manager_get_and_set_correctness(accelize_drm, conf_json, cred_json):
    """Test errors when bad arguments are given to DRM Controller 'get' function"""

    import random

    driver = accelize_drm.pytest_fpga_driver

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )

    # Test get and set correctness
    valexp = random.randint(0,0xFFFFFFFF)
    valinit = drm_manager.get('custom_field')
    print('Initial value in DRM custom field: ', valinit)
    assert valexp != valinit
    drm_manager.set(custom_field=valexp)
    valback = drm_manager.get('custom_field')
    assert valexp == valback


def test_configuration_file_with_wrong_url(accelize_drm, conf_json, cred_json):
    """Test errors when wrong url is given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver

    # Test when authentication url in configuration file is wrong
    conf_json.reset()
    conf_json['licensing']['url'] = "http://accelize.com"
    conf_json.save()
    assert conf_json['licensing']['url'] == "http://accelize.com"

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )

    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    assert "WSOAuth HTTP response code" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMWSMayRetry.error_code


@pytest.mark.skip
def test_configuration_file_with_bad_frequency(capsys, accelize_drm, conf_json, cred_json):
    """Test errors when wrong url is given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver

    async_callback_called = False
    async_callback_message = ''
    async_callback_errcode = -1

    def async_callback(message):
        global async_callback_called, async_callback_message, async_callback_errcode
        async_callback_called = True
        async_callback_message = message
        m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
        assert m, "Could not find 'errCode' in exception message"
        async_callback_errcode = int(m.group(1))

    # Test a BADFrequency error is returned by asynchronous error callback when frequency in configuration file is different from the detected frequency
    async_callback_called = False
    async_callback_message = ''
    async_callback_errcode = -1

    conf_json.reset()
    init_freq = conf_json['drm']['frequency_mhz']
    conf_json['drm']['frequency_mhz'] += 50
    assert conf_json['drm']['frequency_mhz'] == init_freq + 50
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_callback
    )
    drm_manager.activate()
    time.sleep(2)
    drm_manager.deactivate()

    assert async_callback_called, 'Asynchronous callback has NOT been called'
    assert len(async_callback_message) > 0, 'No message has been reported by asynchronous callback'
    assert re.search(r'DRM frequency .* differs from .* configuration file', async_callback_message) is not None, 'An wrong message has been reported by asynchronous callback'
    assert async_callback_errcode == accelize_drm.exceptions.DRMBadFrequency.error_code, 'An wrong error code has been reported by asynchronous callback'

    #assert re.search(r'WARNING: Detected DRM frequency .* differs', captured.out) is not None
    #WARNING: Detected DRM frequency (125 MHz) differs from the value (175 MHz) defined in the configuration file '/tmp/pytest-of-root/pytest-152/test_configuration_file_with_w1/conf.json': From now on the considered frequency is 125 MHz

    ## Test frequency minimum constraint
    #pytest.xfail('Web service is not checking DRM frequency yet')
    #conf_json.reset()
    #conf_json['drm']['frequency_mhz'] = 1000
    #conf_json.save()
    #assert conf_json['drm']['frequency_mhz'] == 1000
    #
    #drm_manager = accelize_drm.DrmManager(
    #    conf_json.path,
    #    cred_json.path,
    #    driver.read_register_callback,
    #    driver.write_register_callback
    #)
    #with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
    #    drm_manager.activate()
    #assert "????" in str(excinfo.value)
    #m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    #assert m, "Could not find 'errCode' in exception message"
    #errCode = int(m.group(1))
    #assert errCode == accelize_drm.exceptions.DRMWSMayRetry.error_code



