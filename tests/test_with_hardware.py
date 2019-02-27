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


def test_configuration_file_with_bad_frequency(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when wrong url is given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver

    # Test a BADFrequency error is returned by asynchronous error callback when frequency in configuration file is different from the detected frequency
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
        async_handler.callback
    )
    drm_manager.activate()
    time.sleep(2)
    drm_manager.deactivate()
    assert async_handler.was_called == True, 'Asynchronous callback has NOT been called'
    assert re.search(r'DRM frequency .* differs from .* configuration file', async_handler.message) is not None, 'A wrong message has been reported by asynchronous callback'
    assert async_handler.errcode == accelize_drm.exceptions.DRMBadFrequency.error_code, 'An wrong error code has been reported by asynchronous callback'

    # TODO: Remove the following line when web service is handling it
    return

    # Test web service detects a frequency underflow
    pytest.xfail('Web service is not checking DRM frequency underflow yet')
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
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMWSMayRetry.error_code

    # Test web service detects a frequency overflow
    pytest.xfail('Web service is not checking DRM frequency overflow yet')
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
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMWSMayRetry.error_code


#def test_configuration_file_with_bad_frequency(accelize_drm, conf_json, cred_json, async_handler):
#    pass
