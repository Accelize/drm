# -*- coding: utf-8 -*-
"""
Test host and card information releated feature
"""
import pytest
from os import environ


def test_host_data_verbosity(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test all supported verbosity
    """
    if 'XRT_PATH' not in environ:
        pytest.skip("XRT_PATH is not defined: skip host and card tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()

    # Get full data
    conf_json['settings']['host_data_verbosity'] = 0
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('host_data_verbosity') == 0
    data_full = drm_manager.get('host_data')
    assert type(data_full) == dict
    assert len(data_full)
    async_cb.assert_NoError()

    # Get partial data
    conf_json['settings']['host_data_verbosity'] = 1
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('host_data_verbosity') == 1
    data_partial = drm_manager.get('host_data')
    assert type(data_partial) == dict
    assert len(data_partial)

    # Get none data
    conf_json['settings']['host_data_verbosity'] = 2
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('host_data_verbosity') == 2
    data_none = drm_manager.get('host_data')
    assert type(data_none) == type(None)
    assert len(data_full) > len(data_partial) > 0


def test_format(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test the format in the request is as expected
    """
    if 'XRT_PATH' not in environ:
        pytest.skip("XRT_PATH is not defined: skip host and card tests")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['host_data_verbosity'] = 0
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('host_data_verbosity') == 0
    data = drm_manager.get('host_data')
    assert type(data) == dict
    assert len(data)
    assert 'board' in data.keys()
    assert 'error' in data['board'].keys()
    assert 'info' in data['board'].keys()
    assert 'dsa_name' in data['board']['info'].keys()
    assert 'xclbin' in data['board'].keys()
    assert 'runtime' in data.keys()
    assert 'system' in data.keys()
    async_cb.assert_NoError()
