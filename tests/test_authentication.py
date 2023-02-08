# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
from os import remove
from os.path import isfile, join, expanduser
from re import search, findall
from time import sleep
from json import loads
from datetime import datetime
from flask import request as _request

from tests.proxy import get_context, set_context
from tests.conftest import wait_until_true

@pytest.mark.no_parallel
def test_authentication_bad_token(accelize_drm, conf_json, cred_json,
                    async_handler, live_server, log_file_factory, request):
    """Test when a bad authentication token is used"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(3)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    access_token = 'BAD_TOKEN'
    context = {'access_token':access_token}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSReqError) as excinfo:
            drm_manager.activate()
        assert accelize_drm.exceptions.DRMWSReqError.error_code in async_handler.get_error_code(str(excinfo.value))
        assert drm_manager.get('token_string') == access_token
    log_content = logfile.read()
    assert search(r'Unauthorized', log_content)
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSReqError.error_code, 'Unauthorized')
    async_cb.reset()
    logfile.remove()


def test_authentication_validity_after_deactivation(accelize_drm, conf_json, cred_json, async_handler):
    """Test authentication token is still valid after deactivate"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
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


@pytest.mark.no_parallel
@pytest.mark.hwtst
def test_authentication_token_renewal(accelize_drm, conf_json, cred_json,
                async_handler, live_server, request, log_file_factory):
    """Test a different authentication token is given after expiration"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    # Set initial context on the live server
    expires_in = 6
    context = {'expires_in': expires_in}
    set_context(context)
    assert get_context() == context

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        start = datetime.now()
        lic_duration = drm_manager.get('license_duration')
        token_string = drm_manager.get('token_string')
        wait_until_true(lambda: drm_manager.get('token_string') != token_string, 2*lic_duration)
        assert drm_manager.get('token_string') != token_string
    log_content = logfile.read()
    assert len(findall(r'Starting Authentication request', log_content)) >= 2
    token_list = findall(r'"access_token" : "([^"]+)"', log_content)
    assert len(token_list) >= 2
    assert token_list.sort() == list(set(token_list)).sort()
    async_cb.assert_NoError()
    logfile.remove()


@pytest.mark.endurance
def test_authentication_endurance(accelize_drm, conf_json, cred_json, async_handler):
    """Test the continuity of service for a long period"""
    driver = accelize_drm.pytest_fpga_driver[0]
    activators = accelize_drm.pytest_fpga_activators[0]
    async_cb = async_handler.create()

    # Get test duration
    try:
        test_duration = accelize_drm.pytest_params['duration']
    except:
        test_duration = 14000
        print('Warning: Missing argument "duration". Using default value %d' % test_duration)

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        activators[0].generate_coin(1000)
        assert not drm_manager.get('license_status')
        activators[0].autotest(is_activated=False)
        drm_manager.activate()
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
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
    activators[0].autotest(is_activated=False)
    print('Endurance test has completed')


@pytest.mark.no_parallel
def test_authentication_cache(accelize_drm, conf_json, cred_json,
                            async_handler, log_file_factory):
    """Test if the authentication token is cached"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    assert not isfile(cred_json.cache_file)
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        token_ref = drm_manager.get('token_string')
        assert isfile(cred_json.cache_file)
        drm_manager.deactivate()
    assert isfile(cred_json.cache_file)
    with open(cred_json.cache_file, 'rt') as f:
        token_json = loads(f.read())
    assert 'access_token' in token_json.keys()
    assert 'expires_in' in token_json.keys()
    assert 'expires_at' in token_json.keys()
    assert token_ref == token_json['access_token']
    log_content = logfile.read()
    assert search(r'Saved token to file ' + cred_json.cache_file, log_content)
    assert search(token_ref, log_content)
    # Open a second time and verify the cache file is used.
    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        drm_manager.activate()
        assert token_ref == drm_manager.get('token_string')
        assert isfile(cred_json.cache_file)
        drm_manager.deactivate()
    log_content = logfile.read()
    assert search(token_ref, log_content)
    assert search(r'Loaded token from file ' + cred_json.cache_file, log_content)
    async_cb.assert_NoError()
    logfile.remove()
