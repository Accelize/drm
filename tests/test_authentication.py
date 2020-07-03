# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import match, search, finditer, MULTILINE, IGNORECASE
from time import sleep, time
from json import loads, dumps
from datetime import datetime, timedelta
from random import randint
from multiprocessing import Process
import requests

from tests.conftest import wait_func_true


def test_authentication_bad_token(accelize_drm, conf_json, cred_json, async_handler):
    """Test when a bad authentication token is used"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    file_log_level = 3
    file_log_type = 1
    file_log_path = realpath("./drmlib-%d.log" % getpid())
    if isfile(file_log_path):
        remove(file_log_path)
    assert not isfile(file_log_path)

    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['log_file_verbosity'] = file_log_level
    conf_json['settings']['log_file_path'] = file_log_path
    conf_json['settings']['log_file_type'] = file_log_type
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.set(bad_oauth2_token=1)
        assert drm_manager.get('token_string') == 'BAD_TOKEN'
        assert drm_manager.get('token_validity') == 1000
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager.activate()
        assert search(r'Timeout on License request after \d+ attempts', str(excinfo.value))
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        async_cb.assert_NoError()
        del drm_manager
        drm_manager = None
        assert wait_func_true(lambda: isfile(file_log_path), 10)
        with open(file_log_path, 'rt') as f:
            file_log_content = f.read()
        assert search(r'\bAuthentication credentials were not provided\b', file_log_content)

    finally:
        if drm_manager:
            drm_manager.deactivate()


def test_authentication_validity_after_deactivation(accelize_drm, conf_json, cred_json, async_handler):
    """Test authentication token is still valid after deactivate"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    file_log_level = 3
    file_log_type = 1
    file_log_path = realpath("./drmlib-%d.log" % getpid())
    if isfile(file_log_path):
        remove(file_log_path)
    assert not isfile(file_log_path)

    async_cb.reset()
    conf_json.reset()
    cred_json.set_user('accelize_accelerator_test_02')
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
        token_time_left = drm_manager.get('token_time_left')
        if token_time_left <= 15:
            drm_manager.deactivate()
            # Wait expiration of current oauth2 token before starting test
            sleep(16)
            drm_manager.activate()
        token_validity = drm_manager.get('token_validity')
        assert token_validity > 15
        exp_token_string = drm_manager.get('token_string')
        drm_manager.deactivate()
        token_string = drm_manager.get('token_string')
        assert token_string == exp_token_string
        drm_manager.activate()
        token_string = drm_manager.get('token_string')
        assert token_string == exp_token_string
        drm_manager.deactivate()
        token_string = drm_manager.get('token_string')
        assert token_string == exp_token_string
        async_cb.assert_NoError()
        print('Test token validity after deactivate: PASS')
    finally:
        drm_manager.deactivate()


@pytest.mark.long_run
@pytest.mark.hwtst
def test_authentication_token_renewal(accelize_drm, conf_json, cred_json, async_handler):
    """Test a different authentication token is given after expiration"""

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    cred_json.set_user('accelize_accelerator_test_02')
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
        token_string = drm_manager.get('token_string')
        token_time_left = drm_manager.get('token_time_left')
        sleep(token_time_left + 1)
        assert drm_manager.get('token_string') != token_string
    finally:
        drm_manager.deactivate()


@pytest.mark.endurance
def test_authentication_endurance(accelize_drm, conf_json, cred_json, async_handler):
    """Test the continuity of service for a long period"""
    from random import sample
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()
    cred_json.set_user('accelize_accelerator_test_02')

    # Get test duration
    try:
        test_duration = accelize_drm.pytest_params['duration']
    except:
        test_duration = 14000
        print('Warning: Missing argument "duration". Using default value %d' % test_duration)

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    activators[0].generate_coin(1000)
    assert not drm_manager.get('license_status')
    activators[0].autotest(is_activated=False)
    drm_manager.activate()
    try:
        lic_duration = drm_manager.get('license_duration')
        assert drm_manager.get('license_status')
        activators[0].autotest(is_activated=True)
        activators[0].check_coin(drm_manager.get('metered_ta'))
        start = datetime.now()
        while True:
            assert drm_manager.get('license_status')
            activators[0].generate_coin(1)
            activators[0].check_coin(drm_manager.get('metered_data'))
            seconds_left = test_duration - (datetime.now() - start).total_seconds()
            print('Remaining time: %0.1fs  /  current coins=%d' % (seconds_left, activators[0].metering_data))
            if seconds_left < 0:
                break
            sleep(60)
    finally:
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators[0].autotest(is_activated=False)
        print('Endurance test has completed')
