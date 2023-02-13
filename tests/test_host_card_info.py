# -*- coding: utf-8 -*-
"""
Test host and card information releated feature
"""
import pytest
from os import environ, getcwd, walk
from time import sleep
from re import match, compile

from tests.conftest import wait_until_true



def find_files(root_dir=getcwd(), regex=r'.*'):
    list_files = []
    r = compile(regex)
    for root, dirs, files in walk(root_dir):
        for f in files:
            if r.match(f):
                list_files.append(f)
    return list_files


def test_host_data_verbosity(accelize_drm, conf_json, cred_json, async_handler,
                    log_file_factory):
    """
    Test all supported verbosity
    """
    #if 'XILINX_XRT' not in environ:
    #    pytest.skip("XILINX_XRT is not defined: skip host and card tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)

    # Get full data
    conf_json['settings']['host_data_verbosity'] = 0
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('host_data_verbosity') == 0
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        data_full = drm_manager.get('host_data')
        assert type(data_full) == dict
        assert len(str(data_full))
    async_cb.assert_NoError()

    # Get partial data
    conf_json['settings']['host_data_verbosity'] = 1
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('host_data_verbosity') == 1
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        data_partial = drm_manager.get('host_data')
        assert type(data_partial) == dict
        assert len(str(data_partial))
        assert len(str(data_full)) >= len(str(data_partial)) > 0
    async_cb.assert_NoError()

    # Get none data
    conf_json['settings']['host_data_verbosity'] = 2
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('host_data_verbosity') == 2
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        data_none = drm_manager.get('host_data')
        assert type(data_none) == type(None)


def test_diagnostics_full_format(accelize_drm, conf_json, cred_json, async_handler,
                     log_file_factory):
    """
    Test the full format in the diagnostics
    """
    #if 'XILINX_XRT' not in environ:
    #    pytest.skip("XILINX_XRT is not defined: skip host and card data tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['host_data_verbosity'] = 0
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('host_data_verbosity') == 0
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        data = drm_manager.get('host_data')
    assert 'drm_library_version' in data
    assert 'os_version' in data
    assert 'os_kernel_version' in data
    assert 'cpu_architecture' in data
    assert 'drm_controller_version' in data
    assert 'device_driver_version' in data

    if data.get('xrt_details'):
        xrt_details = data['xrt_details']
        assert xrt_details
        for xrt in xrt_details:
            if xrt['method'] == 2:
                assert xrt['system']
                host = xrt['system']['host']
                assert host
                assert host['os']
                assert host['xrt']
                assert host['xrt']['version']
                assert host['xrt']['drivers']
                assert host['xrt']['hash']
            else:
                assert xrt['method'] == 1
                assert xrt.get('runtime')
                assert xrt.get('system')
                board = xrt.get('board')
                assert board
                if not accelize_drm.is_ctrl_sw:
                    assert board.get('error')
                assert board.get('xclbin')
                info = board.get('info')
                assert info
                assert info.get('dsa_name') is not None
    # Check no xbutil log files are left over
    assert len(find_files( regex=r'xbutil.*\.log')) == 0
    async_cb.assert_NoError()


def test_diagnostics_partial_format(accelize_drm, conf_json, cred_json, async_handler,
                     log_file_factory):
    """
    Test the partial format in the diagnostics
    """
    #if 'XILINX_XRT' not in environ:
    #    pytest.skip("XILINX_XRT is not defined: skip host and card data tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['host_data_verbosity'] = 1
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        assert drm_manager.get('host_data_verbosity') == 1
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        data = drm_manager.get('host_data')
    assert 'drm_library_version' in data
    assert 'os_version' in data
    assert 'os_kernel_version' in data
    assert 'cpu_architecture' in data
    assert 'drm_controller_version' in data
    assert 'device_driver_version' in data

    if data.get('xrt_details'):
        xrt_details = data['xrt_details']
        assert xrt_details
        for xrt in xrt_details:
            if xrt['method'] == 2:
                assert xrt['version']
                assert xrt['drivers']
                assert xrt['hash']
                assert not xrt.get('system')
            else:
                assert xrt['method'] == 1
                assert xrt.get('runtime')
                assert not xrt.get('system')
                assert board
                if not accelize_drm.is_ctrl_sw:
                    assert board.get('error')
                assert board.get('xclbin')
                info = board.get('info')
                assert info
                assert info.get('dsa_name') is not None
    # Check no xbutil log files are left over
    assert len(find_files( regex=r'xbutil.*\.log')) == 0
    async_cb.assert_NoError()


def test_csp_format(accelize_drm, conf_json, cred_json, async_handler,
                    log_file_factory):
    """
    Test the format in the request is as expected
    """
    if 'XILINX_XRT' not in environ:
        pytest.skip("XILINX_XRT is not defined: skip host and card data tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['host_data_verbosity'] = 1
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        license_duration = drm_manager.get('license_duration')
        wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, license_duration)
        data = drm_manager.get('host_data')
        assert drm_manager.get('host_data_verbosity') == 1
        data = drm_manager.get('host_data')
        drm_manager.deactivate()
    # Check CSP info
    if not accelize_drm.is_ctrl_sw:
        assert data.get('instance_provider')
        if data.get('instance_provider') != 'AWS':
            pytest.skip("CSP '%s' is not supported yet" % data["instance_provider"])
        else:
            assert data['instance_provider'] == 'AWS'
            assert match(r'ami-.*', data.get('instance_image'))
            assert match(r'f1.\d+xlarge', data.get('instance_type'))
            assert data.get('instance_region')
    async_cb.assert_NoError()


@pytest.mark.skip
def test_settings_format(accelize_drm, conf_json, cred_json, async_handler,
                    log_file_factory):
    """
    Test the format in the request is as expected
    """
    if 'XILINX_XRT' not in environ:
        pytest.skip("XILINX_XRT is not defined: skip host and card data tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['host_data_verbosity'] = 0
    conf_json.save()
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        sleep(4)
        data = drm_manager.get('host_data')
        drm_manager.deactivate()
    # Check settings info
    if accelize_drm.is_ctrl_sw:
        assert not data.get('csp')
    else:
        csp = data['csp']
        assert csp
        assert csp.get('csp_name') == 'AWS'
        assert match(r'ami-.*', csp.get('ami-id'))
        assert match(r'f1.\d+xlarge', csp.get('instance-type'))
        assert csp.get('region')
    async_cb.assert_NoError()
