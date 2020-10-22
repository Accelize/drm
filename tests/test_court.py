# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint, randrange
from datetime import datetime, timedelta
from re import search, findall
from os.path import realpath, isfile
from os import remove

from tests.conftest import wait_deadline, wait_func_true


@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.packages
def test_court_1(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test court 1
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
        drm_manager.activate()
    finally:
        drm_manager.deactivate()


@pytest.mark.no_parallel
@pytest.mark.minimum
@pytest.mark.hwtst
@pytest.mark.packages
def test_court_2(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test court 2
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
        drm_manager.activate()
    finally:
        drm_manager.deactivate()
