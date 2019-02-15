"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_function_arguments.py> --cred <path/to/cred.json>
"""

import pytest
import re

def test_drm_manager_constructor_with_bad_arguments(accelize_drm, conf_json, cred_json):
    """Test errors when missing arguments are given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver

    # Test when no configuration file is given
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            "wrong_path_to_conf.json",
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert 'Cannot find JSON file' in str(excinfo.value)

    # Test when no credentials file is given
    conf_json.reset()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            "wrong_path_to_cred.json",
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert 'Cannot find JSON file' in str(excinfo.value)

    # Test when no hardware read register function is given
    conf_json.reset()
    with pytest.raises(TypeError) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            None,
            driver.write_register_callback
        )

    # Test when no hardware write register function is given
    conf_json.reset()
    with pytest.raises(TypeError) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            None
        )


def test_drm_manager_with_incomplete_configuration_file(accelize_drm, conf_json, cred_json):
    """Test errors when missing arguments are given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver

    # Test when no licensing node is specified in configuration file
    conf_json.reset()
    del conf_json['licensing']
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['licensing']
    assert 'licensing' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'licensing' of type Object" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # Test when licensing node is empty in configuration file
    conf_json.reset()
    del conf_json['licensing']
    conf_json['licensing'] = {}
    conf_json.save()
    assert conf_json['licensing'] is not None

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Value of parameter 'licensing' is empty" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # Test when no url is specified in configuration file
    conf_json.reset()
    del conf_json['licensing']['url']
    conf_json['licensing']['dummy'] = 0    # Add this node to be sure the licensing node is not empty (covered by another test)
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['licensing']['url']
    assert 'url' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'url' of type String" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # In nodelocked, test when no license_dir is specified in configuration file
    conf_json.reset()
    conf_json['licensing']['nodelocked'] = True
    if 'license_dir' in conf_json['licensing']:
        del conf_json['license_dir']
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['licensing']['license_dir']
    assert 'license_dir' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'license_dir' of type String" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # In nodelocked, test when license_dir in configuration file is not existing
    conf_json.reset()
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = 'nodelocked_licenses'
    conf_json.save()
    assert conf_json['licensing']['license_dir'] == 'nodelocked_licenses'

    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert re.search(r'License directory path .* is not existing on file system', str(excinfo.value)) is not None
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadArg.error_code

    # Test when no DRM section is not specified
    conf_json.reset()
    del conf_json['drm']
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['drm']
    assert 'drm' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'drm' of type Object" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # Test when no DRM section is empty
    conf_json.reset()
    del conf_json['drm']
    conf_json['drm'] = {}
    conf_json.save()
    assert conf_json['drm'] is not None

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Value of parameter 'drm' is empty" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # Test when no DRM frequency is not specified
    conf_json.reset()
    del conf_json['drm']['frequency_mhz']
    conf_json['drm']['dummy'] = 0    # Add this node to be sure the drm node is not empty (covered by another test)
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['drm']['frequency_mhz']
    assert 'frequency_mhz' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'frequency_mhz' of type Unsigned Integer" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code


def test_drm_manager_with_incomplete_credential_file(accelize_drm, conf_json, cred_json):

    driver = accelize_drm.pytest_fpga_driver

    # Test when credentials no Client ID is specified in credential file
    cred_json.reset()
    del cred_json['client_id']
    cred_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert cred_json['client_id']
    assert 'client_id' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'client_id' of type String" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code

    # Test when credentials no Client Secret ID is specified in credential file
    cred_json.reset()
    del cred_json['client_secret']
    cred_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert cred_json['client_secret']
    assert 'client_secret' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback
        )
    assert "Missing parameter 'client_secret' of type String" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code


def test_drm_manager_get_and_set_bad_arguments(accelize_drm, conf_json, cred_json):
    """Test errors when bad arguments are given to DRM Controller 'get' function"""

    driver = accelize_drm.pytest_fpga_driver

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback
    )

    # Test when bad argument is given to get
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        value = drm_manager.get('unknown_parameter')
    assert "Cannot find parameter:" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadArg.error_code

    # Test when bad argument is given to set
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(unknown_parameter=-1)
    assert "Cannot find parameter:" in str(excinfo.value)
    m = re.search(r'\[errCode=(\d+)\]', str(excinfo.value))
    assert m, "Could not find 'errCode' in exception message"
    errCode = int(m.group(1))
    assert errCode == accelize_drm.exceptions.DRMBadArg.error_code


@pytest.mark.skip
def test_drm_manager_async_error_callback(accelize_drm, conf_json, cred_json):
    """Test asynchronous error callback has been called"""

    driver = accelize_drm.pytest_fpga_driver

    async_message = ''
    async_called = False

    def asyn_error_handler(msg):
        global async_message, async_called
        async_message = msg
        async_called = True

    # Test when no configuration file is given
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        asyn_error_handler
    )
    drm_manager.activate()
    test_message = 'Test message'
    drm_manager.set(call_async_error_callback=test_message)
    time.sleep(1)
    assert async_called
    assert test_message == async_message

