# -*- coding: utf-8 -*-
"""
Test host and card information releated feature
"""
import pytest
from os import environ
from time import sleep
from re import match


def test_host_data_verbosity(accelize_drm, conf_json, cred_json, async_handler,
                    log_file_factory):
    """
    Test all supported verbosity
    """
    if 'XILINX_XRT' not in environ:
        pytest.skip("XILINX_XRT is not defined: skip host and card tests")

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
        data_partial = drm_manager.get('host_data')
        assert type(data_partial) == dict
        assert len(str(data_partial))
        assert len(str(data_full)) >= len(str(data_partial)) > 0

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
        data_none = drm_manager.get('host_data')
        assert type(data_none) == type(None)


def test_host_format(accelize_drm, conf_json, cred_json, async_handler,
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
        assert drm_manager.get('host_data_verbosity') == 0
        data = drm_manager.get('host_data')
    assert data.get('csp') is None
    host_card = data['host_card']
    assert host_card
    if host_card.get('xrt2'):
        host_card = host_card['xrt2']
        assert host_card['system']
        host = host_card['system']['host']
        assert host
        assert host['os']
        assert host['xrt']
    else:
        if host_card.get('xrt1'):
            host_card = host_card['xrt1']
        assert host_card.get('runtime')
        assert host_card.get('system')
        board = host_card.get('board')
        assert board
        if not accelize_drm.is_ctrl_sw:
            assert board.get('error')
        assert board.get('xclbin')
        info = board.get('info')
        assert info
        assert info.get('dsa_name') is not None
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
        assert drm_manager.get('host_data_verbosity') == 0
        data = drm_manager.get('host_data')
        drm_manager.deactivate()
    # Check CSP info
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
