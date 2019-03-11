"""
To run manually, move to the build directory and execute:
    sudo LD_LIBRARY_PATH=. pytest -v <path/to/tests/test_function_arguments.py> --cred <path/to/cred.json>
"""

from re import search

import pytest


def test_drm_manager_constructor_with_bad_arguments(pytestconfig, accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when missing arguments are given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    backend = pytestconfig.getoption("backend")

    # Test when no configuration file is given
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            "wrong_path_to_conf.json",
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Cannot find JSON file' in str(excinfo.value)
    print('Test when no configuration file is given: PASS')

    # Test when no credentials file is given
    conf_json.reset()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            "wrong_path_to_cred.json",
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Cannot find JSON file' in str(excinfo.value)
    print('Test when no credentials file is given: PASS')

    # Test when no hardware read register function is given
    conf_json.reset()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            None,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Read register callback function must not' in str(excinfo.value)
    print('Test when no hardware read register function is given: PASS')

    # Test when no hardware write register function is given
    conf_json.reset()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            None
        )
    assert 'Write register callback function must not' in str(excinfo.value)
    print('Test when no hardware write register function is given: PASS')

    # Test when read register function is not a callable
    conf_json.reset()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            0,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Read register callback function must not' in str(excinfo.value)
    print('Test when read register function is not a callable: PASS')


def test_drm_manager_with_incomplete_configuration_file(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when missing arguments are given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null configuration file
    async_cb.reset()
    conf_json.reset()
    conf_json._content = None
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'JSON file .* is empty', str(excinfo.value)) is not None
    errcode = async_handler.parse_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
    print('Test null config file: PASS')

    # Test with an empty configuration file
    async_cb.reset()
    conf_json.reset()
    conf_json._content = {}
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'JSON file .* is empty', str(excinfo.value)) is not None
    errcode = async_handler.parse_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
    print('Test empty config file: PASS')

    # Test with an invalid configuration file
    async_cb.reset()
    conf_json.reset()
    with open(conf_json.path) as fr:
        content = fr.read()
    new_content = content.replace('"', "'")
    with open(conf_json.path, 'w') as fw:
        fw.write(new_content)
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'Cannot parse ', str(excinfo.value)) is not None
    errcode = async_handler.parse_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_NoError()
    print('Test invalid config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'licensing' of type Object" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_NoError()
    print('Test no licensing node in config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Value of parameter 'licensing' is empty" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test empty licensing in config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'url' of type String" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test no url in config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'license_dir' of type String" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test in node-locked wheb no license_dir is in config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'License directory path .* is not existing on file system', str(excinfo.value)) is not None
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadArg.error_code
    print('Test in node-locked when license_dir in config file is not existing in filesystem: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'drm' of type Object" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test no DRM section in config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Value of parameter 'drm' is empty" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test empty DRM section in config file: PASS')

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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'frequency_mhz' of type Integer" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test no DRM frequency in config file: PASS')


def test_drm_manager_with_incomplete_credential_file(accelize_drm, conf_json, cred_json, async_handler):

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with a null crendential file
    async_cb.reset()
    cred_json.reset()
    cred_json._content = None
    cred_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'JSON file .* is empty', str(excinfo.value)) is not None
    errcode = async_handler.parse_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
    print('Test null crendential file: PASS')

    # Test with an empty crendential file
    async_cb.reset()
    cred_json.reset()
    cred_json._content = {}
    cred_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'JSON file .* is empty', str(excinfo.value)) is not None
    errcode = async_handler.parse_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_NoError()
    print('Test empty crendential file: PASS')

    # Test with an invalid crendential file
    async_cb.reset()
    cred_json.reset()
    with open(cred_json.path) as fr:
        content = fr.read()
    new_content = content.replace('"', "'")
    with open(cred_json.path, 'w') as fw:
        fw.write(new_content)
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'Cannot parse ', str(excinfo.value)) is not None
    errcode = async_handler.parse_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_NoError()
    print('Test invalid crendential file: PASS')

    # Test when no Client ID is specified in credential file
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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'client_id' of type String" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test when no Client ID is specified in credential file: PASS')

    # Test when no Client Secret ID is specified in credential file
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
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'client_secret' of type String" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test when no Client Secret ID is specified in credential file: PASS')


def test_drm_manager_get_and_set_bad_arguments(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when bad arguments are given to DRM Controller 'get' function"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Test when parameter is empty
    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        value = drm_manager.get('')
    assert 'Cannot find parameter:' in str(excinfo.value)
    async_cb.assert_NoError()
    print("Test empty parameter: PASS")

    # Test when bad argument is given to get
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        value = drm_manager.get('unknown_parameter')
    assert "Cannot find parameter: unknown_parameter" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadArg.error_code
    print("Test bad argument is given to get: PASS")

    # Test when bad argument is given to set
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        drm_manager.set(unknown_parameter=-1)
    assert "Cannot find parameter: unknown_parameter" in str(excinfo.value)
    errCode = async_handler.parse_error_code(str(excinfo.value))
    assert errCode == accelize_drm.exceptions.DRMBadArg.error_code
    print("Test when bad argument is given to set: PASS")
