# -*- coding: utf-8 -*-
"""
Test DRM Library with bad arguments. Make sure errors are detected and reported as expected.
"""
import pytest
from re import search
from flask import request


def test_drm_manager_constructor_with_bad_arguments(accelize_drm, conf_json, cred_json,
                                                    async_handler, tmpdir):
    """Test errors when missing arguments are given to DRM Controller Constructor"""
    from os.path import isdir

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test when unexisting configuration file is given
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            "wrong_path_to_conf.json",
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Path is not a valid file:' in str(excinfo.value)
    print('Test when unexisting configuration file is given: PASS')

    # Test when unexisting credentials file is given
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            "wrong_path_to_cred.json",
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Path is not a valid file:' in str(excinfo.value)
    print('Test when unexisting credentials file is given: PASS')

    # Test when configuration path is a directory
    tmp_dir = str(tmpdir)
    assert isdir(tmp_dir)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            tmp_dir,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Path is not a valid file:' in str(excinfo.value)
    print('Test when configuration path is a directory: PASS')

    # Test when credentials path is a directory
    tmp_dir = str(tmpdir)
    assert isdir(tmp_dir)
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            tmp_dir,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Path is not a valid file:' in str(excinfo.value)
    print('Test when credentials path is a directory: PASS')

    # Test when no hardware read register function is given
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            None,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Read register callback function must' in str(excinfo.value)
    print('Test when no hardware read register function is given: PASS')

    # Test when no hardware write register function is given
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            None
        )
    assert 'Write register callback function must' in str(excinfo.value)
    print('Test when no hardware write register function is given: PASS')

    # Test when read register function is not a callable
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            0,
            driver.write_register_callback,
            async_cb.callback
        )
    assert 'Read register callback function must' in str(excinfo.value)
    print('Test when read register function is not a callable: PASS')

    # Test when write register function is not a callable
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            0,
            async_cb.callback
        )
    assert 'Write register callback function must' in str(excinfo.value)
    print('Test when write register function is not a callable: PASS')

    # Test when asynchronous error function is not a callable
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            0
        )
    assert 'Asynchronous error callback function must' in str(excinfo.value)
    print('Test when asynchronous error function is not a callable: PASS')


def test_drm_manager_with_bad_configuration_file(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when missing arguments are given to DRM Controller Constructor"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    # Test with an empty configuration file
    async_cb.reset()
    conf_json.reset()
    conf_json._content = {}
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'JSON file .* is empty', str(excinfo.value)) is not None
    errcode = async_handler.get_error_code(str(excinfo.value))
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
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'Cannot parse ', str(excinfo.value)) is not None
    errcode = async_handler.get_error_code(str(excinfo.value))
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
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'licensing' of type Object" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_NoError()
    print('Test no licensing node in config file: PASS')

    # Test when licensing node is empty in configuration file
    conf_json.reset()
    del conf_json['licensing']
    conf_json['licensing'] = {}
    conf_json.save()
    assert conf_json['licensing'] is not None

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Value of parameter 'licensing' is empty" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test empty licensing in config file: PASS')

    # Test when no url is specified in configuration file
    conf_json.reset()
    del conf_json['licensing']['url']
    conf_json['licensing']['dummy'] = 0  # Add this node to be sure the licensing node is not empty
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['licensing']['url']
    assert 'url' in str(excinfo.value)

    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'url' of type String" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
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
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'license_dir' of type String" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test in node-locked when no license_dir is in config file: PASS')

    # In nodelocked, test when license_dir is empty
    conf_json.reset()
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = ''
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Value of parameter 'license_dir' is an empty string" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test in node-locked when license_dir is empty: PASS')

    # In nodelocked, test when license_dir in configuration file is not existing in file-system
    conf_json.reset()
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = 'nodelocked_licenses'
    conf_json.save()
    assert conf_json['licensing']['license_dir'] == 'nodelocked_licenses'

    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'License directory path .* is not existing on file system',
                  str(excinfo.value)) is not None
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    print('Test in node-locked when license_dir in config file is not existing: PASS')

    # In nodelocked, test when license_dir in configuration file is not a directory
    conf_json.reset()
    conf_json['licensing']['nodelocked'] = True
    conf_json['licensing']['license_dir'] = 'nodelocked_licenses'
    conf_json.save()
    assert conf_json['licensing']['license_dir'] == 'nodelocked_licenses'
    # Create file
    with open('nodelocked_licenses', 'w') as f:
        f.write('dummy file')
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'License directory path .* is not existing on file system',
                  str(excinfo.value)) is not None
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
    print('Test in node-locked when license_dir in config file is not a directory: PASS')

    # Test when no DRM section is specified
    conf_json.reset()
    del conf_json['drm']
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['drm']
    assert 'drm' in str(excinfo.value)
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'drm' of type Object" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test no DRM section in config file: PASS')

    # Test when DRM section is empty
    conf_json.reset()
    del conf_json['drm']
    conf_json['drm'] = {}
    conf_json.save()
    assert conf_json['drm'] is not None
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Value of parameter 'drm' is empty" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test empty DRM section in config file: PASS')

    # Test when no DRM frequency is specified
    conf_json.reset()
    del conf_json['drm']['frequency_mhz']
    conf_json['drm']['dummy'] = 0    # Add this node to be sure the drm node is not empty
    conf_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert conf_json['drm']['frequency_mhz']
    assert 'frequency_mhz' in str(excinfo.value)
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'frequency_mhz' of type Integer" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test no DRM frequency in config file: PASS')

    # Test when DRM frequency is a wrong type
    conf_json.reset()
    conf_json['drm']['frequency_mhz'] = 'this is wrong type'
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Wrong parameter type for 'frequency_mhz' = " in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test DRM frequency is a wrong type: PASS')

    # Test when settings is a wrong type
    conf_json.reset()
    conf_json['settings'] = 'this is wrong type'
    conf_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Wrong parameter type for 'settings' = " in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test settings is a wrong type: PASS')


def test_drm_manager_with_bad_credential_file(accelize_drm, conf_json, cred_json, async_handler):

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    # Test with an empty crendential file
    cred_json.reset()
    cred_json._content = {}
    cred_json.save()
    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'JSON file .* is empty', str(excinfo.value)) is not None
    errcode = async_handler.get_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadArg.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'Error with credential file')
    async_cb.reset()
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
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert search(r'Cannot parse ', str(excinfo.value)) is not None
    errcode = async_handler.get_error_code(str(excinfo.value))
    assert errcode == accelize_drm.exceptions.DRMBadFormat.error_code
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadFormat.error_code, 'Error with credential file')
    async_cb.reset()
    print('Test invalid crendential file: PASS')

    # Test when no Client ID is specified in credential file
    cred_json.reset()
    del cred_json['client_id']
    cred_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert cred_json['client_id']
    assert 'client_id' in str(excinfo.value)
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'client_id' of type String" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test when no Client ID is specified in credential file: PASS')

    # Test when no Client Secret ID is specified in credential file
    cred_json.reset()
    del cred_json['client_secret']
    cred_json.save()
    with pytest.raises(KeyError) as excinfo:
        assert cred_json['client_secret']
    assert 'client_secret' in str(excinfo.value)
    with pytest.raises(accelize_drm.exceptions.DRMBadFormat) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert "Missing parameter 'client_secret' of type String" in str(excinfo.value)
    err_code = async_handler.get_error_code(str(excinfo.value))
    assert err_code == accelize_drm.exceptions.DRMBadFormat.error_code
    print('Test when no Client Secret ID is specified in credential file: PASS')


def test_drm_manager_get_and_set_bad_arguments(accelize_drm, conf_json, cred_json, async_handler):
    """Test errors when bad arguments are given to DRM Controller 'get' function"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Test when parameter is None
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.get(None)
        assert 'keys argument is empty' in str(excinfo.value)
        async_cb.assert_NoError()
        print("Test when parameter is None: PASS")

        # Test when parameter is empty
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.get('')
        assert 'keys argument is empty' in str(excinfo.value)
        async_cb.assert_NoError()
        print("Test empty parameter: PASS")

        # Test when bad argument is given to get
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.get('unknown_parameter')
        assert "Cannot find parameter: unknown_parameter" in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
        print("Test bad argument is given to get: PASS")

        # Test when bad argument is given to set
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(unknown_parameter=-1)
        assert "Cannot find parameter: unknown_parameter" in str(excinfo.value)
        err_code = async_handler.get_error_code(str(excinfo.value))
        assert err_code == accelize_drm.exceptions.DRMBadArg.error_code
        print("Test when bad argument is given to set: PASS")


@pytest.mark.aws
def test_c_unittests(accelize_drm, exec_func):
    """Test errors when missing arguments are given to DRM Controller Constructor"""
    if 'aws' not in accelize_drm.pytest_fpga_driver_name:
        pytest.skip("C unit-tests are only supported with AWS driver.")

    driver = accelize_drm.pytest_fpga_driver[0]
    exec_lib = exec_func.load('unittests', driver._fpga_slot_id)

    # Test when read register callback is null
    exec_lib.run('test_null_read_callback')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Read register callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test when write register callback is null
    exec_lib.run('test_null_write_callback')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Write register callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test when asynchronous error callback is null
    exec_lib.run('test_null_error_callback')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Asynchronous error callback function must not be NULL' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test various types of get and set functions
    exec_lib.run('test_types_of_get_and_set_functions')
    assert exec_lib.returncode == 0
    assert exec_lib.asyncmsg is None

    # Test out of range of get function
    exec_lib.run('test_get_function_out_of_range')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadArg.error_code
    assert 'Cannot find parameter with ID: ' in exec_lib.stdout
    assert exec_lib.asyncmsg is None

    # Test get_json_string with bad format
    exec_lib.run('test_get_json_string_with_bad_format')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadFormat.error_code
    assert 'Cannot parse JSON string ' in exec_lib.stdout
    assert 'Cannot parse JSON string ' in exec_lib.asyncmsg

    # Test get_json_string with empty string
    exec_lib.run('test_get_json_string_with_empty_string')
    assert exec_lib.returncode == accelize_drm.exceptions.DRMBadFormat.error_code
    assert 'Cannot parse an empty JSON string' in exec_lib.stdout
    assert 'Cannot parse an empty JSON string' in exec_lib.asyncmsg
