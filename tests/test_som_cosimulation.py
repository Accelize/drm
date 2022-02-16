# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
import pytest
from time import sleep
from random import randint, choice
from datetime import datetime, timedelta
from re import search, findall
from os.path import realpath, isfile

from tests.conftest import wait_deadline, wait_func_true


@pytest.mark.cosim
def test_hybrid_fast_start_stop(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """
    Test no error occurs witha quick start/stop
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_06')
    conf_json.reset()
    logfile = log_file_factory.create(2)
    conf_json['settings'].update(logfile.json)
    conf_json['drm']['drm_software'] = True
    conf_json.save()
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        assert not drm_manager.get('license_status')
        drm_manager.activate()
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        async_cb.assert_NoError()
    logfile.remove()

