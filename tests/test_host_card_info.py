# -*- coding: utf-8 -*-
"""
Test host and card information releated feature
"""
import pytest
from time import sleep
from random import randint, randrange
from datetime import datetime, timedelta
from re import search, findall
from os.path import realpath, isfile
from os import remove

from tests.conftest import wait_deadline, wait_func_true


def test_host_data_verbosity(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test all supported verbosity
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()

    # Get full data
    conf_json['settings']['host_data_verbosity'] = 0
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
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('host_data_verbosity') == 2
    data_partial = drm_manager.get('host_data')
    assert type(data_partial) == dict
    assert len(data_partial) == 0


def test_format(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test the format in the request is as expected
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['host_data_verbosity'] = 0
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('host_data_verbosity') == 0
    data = drm_manager.get('host_data_verbosity')
    assert type(data) == dict
    assert len(data)
    assert 'info' in data.keys()
    async_cb.assert_NoError()