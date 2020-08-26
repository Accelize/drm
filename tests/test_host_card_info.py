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
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

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
    data = drm_manager.get('host_data_verbosity')
        activators.autotest(is_activated=False)
        activators[0].generate_coin(1000)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.activate()
        assert drm_manager.get('metered_data') == 0
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('metered_data') == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()


def test_format(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test the format in the request is as expected
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_02')

    async_cb.reset()
    conf_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        activators[0].generate_coin(1000)
        activators[0].check_coin(drm_manager.get('metered_data'))
        drm_manager.activate()
        activators[0].generate_coin(1)
        assert drm_manager.get('metered_data') == 1
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        assert drm_manager.get('metered_data') == 0
        async_cb.assert_NoError()
    finally:
        drm_manager.deactivate()
