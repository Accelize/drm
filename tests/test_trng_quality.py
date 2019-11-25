# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
import re
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import search, findall, finditer, MULTILINE
from time import sleep, time
from json import loads, dumps
from datetime import datetime, timedelta


LOG_FORMAT_SHORT = "[%^%=8l%$] %-6t, %v"
LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

REGEX_FORMAT_SHORT = r'\[\s*(\w+)\s*\] \s*\d+\s*, %s'
REGEX_FORMAT_LONG  = r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3} - \s*\S+:\d+\s* \[\s*(\w+)\s*\] \s*\d+\s*, %s'


def ordered_json(obj):
    if isinstance(obj, dict):
        return sorted((k, ordered_json(v)) for k, v in obj.items())
    if isinstance(obj, list):
        return sorted(ordered_json(x) for x in obj)
    else:
        return obj


def compute_bit_dispersion(accelize_drm, conf_json, cred_json, async_handler):
    """Test the versions of the DRM Lib and its dependencies are well displayed"""
    versions = accelize_drm.get_api_version()
    assert search(r'\d+\.\d+\.\d+', versions.version) is not None


@pytest.mark.security
def test_chip_id(accelize_drm, conf_json, cred_json, async_handler):
    pass


@pytest.mark.security
def test_intra_challenge_duplication(accelize_drm, conf_json, cred_json, async_handler):
    """Run drmlib application long enough to generate 'num_request' license request to License WS.
    Purpose is to evaluate the quality of the SAAS Challenge.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_05')
    try:
        num_request = accelize_drm.pytest_params['num_request']
    except:
        raise Exception('Missing argument: num_request')

    async_cb.reset()
    conf_json.reset()
    logpath = realpath("./drmlib.%d.log" % getpid())
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        activators.autotest(is_activated=False)
        drm_manager.activate()
        start_time = datetime.now()
        activators.autotest(is_activated=True)
        license_duration = drm_manager.get('license_duration')
        print('license_duration=', license_duration)
        while datetime.now() - start_time < timedelta(seconds=num_request*license_duration):
            activators[0].generate_coin(1)
            print('Generate 1 more coin!')
            wait_period = datetime.now() - start_time
            sleep(license_duration - wait_period.total_seconds() % license_duration + 1)
    finally:
        print('Metered data= %d' % drm_manager.get('metered_data'))
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        del drm_manager
        gc.collect()
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()

    # Parse log file
    with open(logpath, 'rt') as f:
        text = f.read()
    request_json = dict()
    request_json['requests'] = list()
    for challenge in finditer(r'^({.*?saasChallenge.*?\n})$', text, re.MULTILINE | re.DOTALL):
        request_json['requests'].append(loads(challenge.group(0)))
    request_str = dumps(request_json, indent=4, sort_keys=True)
    print(request_str)
    challenge_count = dict()
    for e in finditer(r'"saasChallenge"\s*:\s*"(.+?)"\s*', request_str):
        challenge = e.group(1)
        if challenge not in challenge_count.keys():
            challenge_count[challenge] = 0
        challenge_count[challenge] += 1
    print('Processed %d challenges' % len(request_json['requests']))
    assert len(request_json['requests']) == len(challenge_count), \
        "Duplicates are:\n%s" % '\n'.join(["%s appears %d times" % (challenge, count) for challenge, count in challenge_count.items() if count > 1])


@pytest.mark.security
def test_first_challenge_duplication(accelize_drm, conf_json, cred_json, async_handler):
    """Run 'num_request' times the drmlib application to evaluate the quality of the first challenge.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_05')
    try:
        num_request = accelize_drm.pytest_params['num_request']
    except:
        raise Exception('Missing argument: num_request')

    async_cb.reset()
    conf_json.reset()
    logpath = realpath("./drmlib.%d.log" % getpid())
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    for e in range(num_request):
        activators.autotest(is_activated=False)
        drm_manager.activate()
        activators.autotest(is_activated=True)
        activators[0].generate_coin(1)
        print('Generate 1 more coin!')
        drm_manager.deactivate()
        activators.autotest(is_activated=False)
    del drm_manager
    gc.collect()
    async_cb.assert_NoError()

    # Parse log file
    with open(logpath, 'rt') as f:
        text = f.read()
    request_json = dict()
    request_json['requests'] = list()
    for challenge in finditer(r'^({.*?saasChallenge.*?\n})$', text, re.MULTILINE | re.DOTALL):
        request_json['requests'].append(loads(challenge.group(0)))
    # Keep only the 'open' requests
    request_json['requests'] = list(filter(lambda x: x['request'] == 'open', request_json['requests']))
    request_str = dumps(request_json, indent=4, sort_keys=True)
    challenge_count = dict()
    for e in finditer(r'"saasChallenge"\s*:\s*"(.+?)"\s*', request_str):
        challenge = e.group(1)
        if challenge not in challenge_count.keys():
            challenge_count[challenge] = 0
        challenge_count[challenge] += 1
    print('Processed %d challenges' % len(request_json['requests']))
    assert len(request_json['requests']) == len(challenge_count), \
        "Duplicates are:\n%s" % '\n'.join(["%s appears %d times" % (challenge, count) for challenge, count in challenge_count.items() if count > 1])

