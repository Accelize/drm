import pytest

def test_drm_manager_instantiation(accelize_drm, conf_json, cred_json):
    """Test errors when bad arguments are given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver

    def error_handler(msg):
        global error_is_called, error_msg
        error_is_called = True
        error_msg = msg

    # Test when no configuration file is given
    error_is_called = False
    error_msg = ""
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelier_drm.DrmManager(
            "wrong_path_to_conf.json",
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            error_handler
        )
    assert 'configuration' in str(excinfo.value)
    assert error_is_called, "Asynchronous error callback NOT called"
    assert len(error_msg) == 0, "Asynchronous error callback has NO message"
    assert '???' in error_msg, "Asynchronous error callback has NO message"

    # Test when web service url in configuration file is wrong
    error_is_called = False
    error_msg = ""

    # Test when authentication url in configuration file is wrong
    error_is_called = False
    error_msg = ""

    # Test when no DRM section is not specified
    error_is_called = False
    error_msg = ""

    # Test when no DRM frequency is not specified
    error_is_called = False
    error_msg = ""

    # In NodeLocked, test when no license_dir is not specified
    error_is_called = False
    error_msg = ""

    # Test when no credentials file is given
    error_is_called = False
    error_msg = ""
    with pytest.raises(accelize_drm.exceptions.DRMBadArg):
        accelize_drm.DrmManager("conf_wrong_path.json", )

    # Test when credentials client ID is wrong
    error_is_called = False
    error_msg = ""

    # Test when credentials Secret ID is wrong
    error_is_called = False
    error_msg = ""



    # Test everything is fine
    # In NodeLocked, test everything is fine


