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

SAMPLES_DUPLICATE_THRESHOLD = 1
SAMPLES_DISPERSION_THRESHOLD = 10
DUPLICATE_THRESHOLD = 1.0/10000
DISPERSION_THRESHOLD = 0.5


def parse_and_save_challenge(logpath, pattern, save_path=None):
    # Parse log file
    with open(logpath, 'rt') as f:
        text = f.read()
    request_json = dict()
    request_json['requests'] = list()
    for challenge in finditer(r'^({.*?%s.*?\n})$' % pattern, text, re.I | re.MULTILINE | re.DOTALL):
        request_json['requests'].append(loads(challenge.group(0)))
    # Save to file
    if save_path is not None:
        with open(save_path, 'wt') as f:
            f.write(dumps(request_json, indent=4, sort_keys=True))
    return request_json


def check_duplicates(value_list):
    list_size = len(value_list)
    if list_size < SAMPLES_DUPLICATE_THRESHOLD:
        print('Not enough samples to compute duplicates')
        return None
    count_by_value = dict()
    for value in value_list:
        if value not in count_by_value.keys():
            count_by_value[value] = 0
        count_by_value[value] += 1
    print('Processed %d values' % list_size)
    if list_size != len(count_by_value):
        print("Found following duplicates:")
    else:
        print("No duplicates found")
    for v,d in count_by_value.items():
        if d > 1:
            print("\t%s appears %d times" % (v,d))
    duplication_percent = float(list_size - len(count_by_value))/ list_size * 100
    print("=> Percentage of duplicates: %0.1f%%" % duplication_percent)
    return duplication_percent


def check_bit_dispersion(value_list):
    """Compute the dispersion per bit on the results of the previous tests"""
    list_size = len(value_list)
    # Convert to binary string
    value_list = list(map(lambda x: bin(int(x, 16))[2:], value_list))
    value_len = max(map(lambda x: len(x), value_list))
    print('value_len=', value_len)
    if list_size < SAMPLES_DISPERSION_THRESHOLD:
        print('Not enough samples to compute bit dispersion')
        return None
    value_list = list(map(lambda x: x.zfill(value_len), value_list))
    # Compute dispersion
    count_per_bit = [0]*value_len
    for value in value_list:
        for bit in range(value_len):
            count_per_bit[bit] += int(value[bit])
    if list_size < value_len:
        disp_per_bit = list(filter(lambda x: x!=0 and x!=list_size, count_per_bit))
    disp_per_bit = list(map(lambda x: float(x)/list_size, disp_per_bit))
    err = 0
    gap = 0.0
    for i, disp in enumerate(disp_per_bit):
        gap += abs(disp-0.5)
        if 0.4 < disp < 0.6:
            res = "OK"
        else:
            res = "KO !!!"
            err += 1
        print('#%d: %f => %s' %(i, disp, res))
    bit_tested_percent = float(len(disp_per_bit)) / value_len * 100
    print('Processed %d values' % list_size)
    print('Percentage of tested bits: %0.1f%%' % bit_tested_percent)
    print('Number of bad bits: %d' % err)
    score = gap/len(disp_per_bit)
    print('=> Dispersion score: %f' % score)
    return score


@pytest.mark.security
@pytest.mark.last
def test_global_challenge_quality():
    print()
    value_json = {'requests' : list()}
    for filename in glob("./*.json"):
        print('Analyzing file: %s ...' % filename)
        with open(filename, 'rt') as f:
            res_json = loads(f.read())
        value_json['requests'].extend(res_json['requests'])
        print('... adding %d more requests' % len(res_json['requests']))
    value_list = [e['saasChallenge'] for e in res_json['requests'] if e['request']!='close']
    # Check duplicates
    dupl_score = check_duplicates(value_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD
    # Check dispersion
    disp_score = check_bit_dispersion(value_list)
    if disp_score:
        assert disp_score < DISPERSION_THRESHOLD


@pytest.mark.security
def test_first_challenge_duplication(accelize_drm, conf_json, cred_json, async_handler):
    """Run 'num_samples' times the drmlib application to evaluate the quality of the first challenge.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_05')
    try:
        num_samples = accelize_drm.pytest_params['num_samples']
    except:
        raise Exception('Missing argument: num_samples')

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
        for e in range(num_samples):
            activators.autotest(is_activated=False)
            drm_manager.activate()
            activators.autotest(is_activated=True)
            drm_manager.deactivate()
            activators.autotest(is_activated=False)
            async_cb.assert_NoError()
    finally:
        del drm_manager
        gc.collect()

    # Parse log file
    request_json = parse_and_save_challenge(logpath, 'challenge', 'test_first_challenge_duplication.json')
    # Keep only the 'open' requests
    request_json['requests'] = list(filter(lambda x: x['request'] == 'open', request_json['requests']))
    # Check validity
    assert len(request_json['requests']) >= num_samples
    # Check duplicates
    challenge_list = [e['saasChallenge'] for e in request_json['requests']]
    dupl_score = check_duplicates(challenge_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD


@pytest.mark.security
def test_intra_challenge_duplication(accelize_drm, conf_json, cred_json, async_handler):
    """Run drmlib application long enough to generate 'num_samples' license request to License WS.
    Purpose is to evaluate the quality of the SAAS Challenge.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_05')
    try:
        num_samples = accelize_drm.pytest_params['num_samples']
    except:
        raise Exception('Missing argument: num_samples')

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
        assert drm_manager.get('license_status')
        activators.autotest(is_activated=True)
        license_duration = drm_manager.get('license_duration')
        print('Waiting %d seconds (license duration=%d)' % (num_samples*license_duration + 1, license_duration))
        sleep(num_samples*license_duration + 1)
    finally:
        drm_manager.deactivate()
        assert not drm_manager.get('license_status')
        activators.autotest(is_activated=False)
        async_cb.assert_NoError()
        del drm_manager
        gc.collect()

    # Parse log file
    request_json = parse_and_save_challenge(logpath, 'challenge', 'test_intra_challenge_duplication.json')
    # Remove close request because they repeat the last challenge
    request_json['requests'] = list(filter(lambda x: x['request'] != 'close', request_json['requests']))
    # Check validity
    assert len(request_json['requests']) >= num_samples
    # Check duplicates
    challenge_list = [e['saasChallenge'] for e in request_json['requests']]
    dupl_score = check_duplicates(challenge_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD


@pytest.mark.security
def test_inter_challenge_duplication(accelize_drm, conf_json, cred_json, async_handler):
    """Run multiple runs of drmlib application to generate 'num_sessions' sessions of 'num_samples' license requests to License WS.
    Purpose is to evaluate the quality of the SAAS Challenge.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_05')
    try:
        num_samples = accelize_drm.pytest_params['num_samples']
        num_sessions = accelize_drm.pytest_params['num_sessions']
    except:
        raise Exception('Missing argument: num_samples')

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
        for e in range(num_sessions):
            print('Running session #%d/%d...' % (e, num_sessions))
            for e in range(num_samples):
                activators.autotest(is_activated=False)
                assert not drm_manager.get('license_status')
                drm_manager.activate()
                assert drm_manager.get('license_status')
                activators.autotest(is_activated=True)
                license_duration = drm_manager.get('license_duration')
                print('Waiting %d seconds (license duration=%d)' % (num_samples*license_duration + 1, license_duration))
                sleep(num_samples*license_duration + 1)
                drm_manager.deactivate()
                assert not drm_manager.get('license_status')
                activators.autotest(is_activated=False)
        async_cb.assert_NoError()
    finally:
        del drm_manager
        gc.collect()

    # Parse log file
    request_json = parse_and_save_challenge(logpath, 'challenge', 'test_inter_challenge_duplication.json')
    # Remove close request because they repeat the last challenge
    request_json['requests'] = list(filter(lambda x: x['request'] != 'close', request_json['requests']))
    # Check validity
    assert len(request_json['requests']) >= num_samples
    # Check duplicates
    challenge_list = [e['saasChallenge'] for e in request_json['requests']]
    dupl_score = check_duplicates(challenge_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD

