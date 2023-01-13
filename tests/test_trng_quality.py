# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import sys
import re
from glob import glob
from os import remove
from os.path import getsize, isfile, dirname, join, realpath, basename
from re import finditer
from time import sleep, time
from json import loads, dumps
from datetime import datetime, timedelta
from random import randrange

from tests.conftest import wait_until_true


SAMPLES_DUPLICATE_THRESHOLD = 2
SAMPLES_DISPERSION_THRESHOLD = 10
DUPLICATE_THRESHOLD = 1/10000*100.0
DISPERSION_THRESHOLD = 0.1

LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

REGEX_PATTERN = r'Starting Saas request to \S+ with data:\n(^{.+?\n}$)'


def parse_and_save_challenge(text, pattern, save_path=None):
    # Parse log file
    request_list = list()
    for challenge in finditer(pattern, text, re.I | re.MULTILINE | re.DOTALL):
        request_list.append(loads(challenge.group(1)))
    # Save to file
    if save_path is not None:
        if isfile(save_path):
            with open(save_path, 'rt') as f:
                new_request_list = loads(f.read())['requests']
            new_request_list.extend(request_list)
        else:
            new_request_list = request_list
        with open(save_path, 'wt') as f:
            f.write(dumps({'requests': new_request_list}, indent=4, sort_keys=True))
    return request_list


def check_duplicates(value_list):
    list_size = len(value_list)
    if list_size < SAMPLES_DUPLICATE_THRESHOLD:
        print('Not enough samples to compute duplicates')
        return -1
    count_by_value = dict()
    for value in value_list:
        if value not in count_by_value.keys():
            count_by_value[value] = 0
        else:
            count_by_value[value] += 1
    num_duplicates = sum(count_by_value.values())
    print('Processed %d values (with %d duplicates)' % (list_size, num_duplicates))
    if num_duplicates > 0:
        print("Found following duplicates:")
        for v,d in count_by_value.items():
            if d > 0:
                print("\t%s appears %d times" % (v,d+1))
    duplication_percent = float(num_duplicates)/ list_size * 100
    return duplication_percent


def check_bit_dispersion(value_list):
    """Compute the dispersion per bit on the results of the previous tests"""
    list_size = len(value_list)
    # Convert to binary string
    value_list = list(map(lambda x: bin(int(x, 16))[2:], value_list))
    value_len = max(map(lambda x: len(x), value_list))
    if list_size < SAMPLES_DISPERSION_THRESHOLD:
        print('Not enough samples to compute bit dispersion')
        return -1
    value_list = list(map(lambda x: x.zfill(value_len), value_list))
    # Compute dispersion
    count_per_bit = [0]*value_len
    for value in value_list:
        for bit in range(value_len):
            count_per_bit[bit] += int(value[bit])
    if list_size < value_len:
        count_per_bit = list(filter(lambda x: x!=0 and x!=list_size, count_per_bit))
    disp_per_bit = list(map(lambda x: float(x)/list_size, count_per_bit))
    err = 0
    gap = 0.0
    for i, disp in enumerate(disp_per_bit):
        gap += abs(disp-0.5)
        if 0.4 > disp or disp > 0.6:
            err += 1
            print('Bad bit #%d: %f' % (i, disp))
    bit_tested_percent = float(len(disp_per_bit)) / value_len * 100
    print('Processed %d values' % list_size)
    print('Percentage of tested bits: %0.1f%%' % bit_tested_percent)
    print('Percentage of bad bits: %0.3f%% (%d)' % (float(err)/value_len*100, err))
    score = gap/len(disp_per_bit)
    return score


@pytest.mark.security
@pytest.mark.no_parallel
@pytest.mark.last
def test_global_challenge_quality():
    print()
    value_json = {'requests' : list()}
    for filename in glob("./test_*.json"):
        print('Analyzing file: %s ...' % filename)
        with open(filename, 'rt') as f:
            res_json = loads(f.read())
        value_json['requests'].extend(res_json['requests'])
        print('... adding %d more requests' % len(res_json['requests']))

    # CHECK CHALLENGE QUALITY
    print('*** Check global Challenge quality ***')
    value_list = list()
    for e in value_json['requests']:
        try:
            if e['request']!='close' and 'saasChallenge' in e.keys():
                value_list.append(e['saasChallenge'])
        except:
            print("Bad format for following request: %s" % str(e))
    # Check duplicates
    chlg_dupl_score = check_duplicates(value_list)
    print("=> Percentage of global Challenge duplicates: %f%%" % chlg_dupl_score)
    # Check dispersion
    chlg_disp_score = check_bit_dispersion(value_list)
    print('=> Global Challenge dispersion score: %f' % chlg_disp_score)

    # CHECK DNA QUALITY
    print('*** Check global DNA quality ***')
    value_list = [e['dna'] for e in value_json['requests'] if e['request']=='open']
    value_list = list()
    for e in value_json['requests']:
        try:
            if e['request']=='open' and 'dna' in e.keys():
                value_list.append(e['dna'])
        except:
            print("Bad format for following request: %s" % str(e))
    # Check duplicates
    dna_dupl_score = check_duplicates(value_list)
    print("=> Percentage of global DNA duplicates: %f%%" % dna_dupl_score)
    # Check dispersion
    dna_disp_score = check_bit_dispersion(value_list)
    print('=> Global DNA dispersion score: %f' % chlg_disp_score)
    # Check assertion
    if chlg_dupl_score:
        assert chlg_dupl_score < DUPLICATE_THRESHOLD
    if chlg_disp_score:
        assert chlg_disp_score < DISPERSION_THRESHOLD
    if dna_dupl_score:
        assert dna_dupl_score < DUPLICATE_THRESHOLD
    if dna_disp_score:
        assert dna_disp_score < DISPERSION_THRESHOLD


@pytest.mark.security
def test_dna_and_challenge_duplication(accelize_drm, conf_json, cred_json, async_handler, log_file_factory):
    """Preprogram N times the board and start a session with M licenses.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()

    # Get number of sessions parameter
    try:
        num_sessions = accelize_drm.pytest_params['num_sessions']
    except:
        num_sessions = 100
        print('Warning: Missing argument "num_sessions". Using default value %d' % num_sessions)
    # Get number of samples parameter
    try:
        num_samples = accelize_drm.pytest_params['num_samples']
    except:
        num_samples = 4
        print('Warning: Missing argument "num_samples". Using default value %d' % num_samples)
    # Get access key parameter
    try:
        access_key = accelize_drm.pytest_params['access_key']
    except:
        access_key = "accelize_accelerator_test_05"
        print('Warning: Missing argument "access_key". Using default value %s' % access_key)

    print('num_sessions=', num_sessions)
    print('num_samples=', num_samples)
    print('access_key=', access_key)

    async_cb.reset()
    cred_json.set_user(access_key)
    conf_json.reset()
    logfile = log_file_factory.create(1, format=LOG_FORMAT_LONG)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    test_file_path = 'test_dna_and_challenge_duplication.%d.%d.json' % (time(), randrange(0xFFFFFFFF))
    session_cnt = 0
    while session_cnt < num_sessions:
        no_err = False
        try:
            print('Reseting #%d/%d...' % (session_cnt+1, num_sessions))
            driver.program_fpga(image_bkp)
            print('Starting session #%d/%d ...' % (session_cnt+1,num_sessions))
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            activators.autotest(is_activated=False)
            drm_manager.activate()
            activators.autotest(is_activated=True)
            license_duration = drm_manager.get('license_duration')
            sleep(1)
            sample_cnt = 0
            while sample_cnt < num_samples:
                sleep(license_duration)
                if async_cb.was_called:
                    print('Error occurred in %s at sample #%d/%d: background thread failed with message: %s' % (sys._getframe().f_code.co_name, s+1, num_samples, async_cb.message))
                    async_cb.reset()
                else:
                    sample_cnt += 1
                    print('%d/%d licenses passed' % (sample_cnt, num_samples))
            # Check validity
            assert sample_cnt >= num_samples
            no_err = True
        except:
            print('Session #%d/%d failed: retrying!' % (session_cnt+1,num_sessions))
        finally:
            print('Ending session #%d/%d ...' % (session_cnt+1,num_sessions))
            while True:
                try:
                    drm_manager.deactivate()
                    break
                except:
                    print('Error occurred in %s: deactivate failed with message: %s' % (sys._getframe().f_code.co_name, async_cb.message))
                    async_cb.reset()
                    sleep(1)
            activators.autotest(is_activated=False)
            del drm_manager
            log_content = logfile.read()
            if no_err:
                session_cnt += 1
        async_cb.assert_NoError()
        # Parse log file
        parse_and_save_challenge(log_content, REGEX_PATTERN, test_file_path)
        logfile.remove()

    # Check validity
    assert session_cnt >= num_sessions
    # Parse log file
    with open(test_file_path, 'rt') as f:
        request_list = loads(f.read())['requests']
    # Remove 'close' requests
    request_list = list(filter(lambda x: x['request'] != 'close', request_list))
    assert len(request_list) >= num_sessions * num_samples
    # Compute Challenges duplicates
    challenge_list = [e['saasChallenge'] for e in request_list]
    challenge_score = check_duplicates(challenge_list)
    print("=> Percentage of Challenge duplicates for current test: %f%%" % challenge_score)
    # Compute DNA duplicates
    dna_list = [e['dna'] for e in request_list if e['request']=='open']
    assert len(dna_list) >= num_sessions
    dna_score = check_duplicates(dna_list)
    print("=> Percentage of DNA duplicates for current test: %f%%" % dna_score)
    # Check duplicate
    if challenge_score:
        assert challenge_score < DUPLICATE_THRESHOLD
    if dna_score:
        assert dna_score < DUPLICATE_THRESHOLD


@pytest.mark.no_parallel
def test_saas_challenge_quality_through_activates(accelize_drm, conf_json,
            cred_json, async_handler, log_file_factory):
    nb_loop = 3
    nb_req = 2
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        for i in range(nb_loop):
            drm_manager.activate()
            lic_period = drm_manager.get('license_duration')
            wait_until_true(lambda: drm_manager.get('num_license_loaded') == 2, lic_period)
            drm_manager.deactivate()
    log_content = logfile.read()
    # Parse log file
    challenge_list = parse_and_save_challenge(log_content, REGEX_PATTERN)
    # Remove close request
    challenge_list = list(filter(lambda x: not x.get('is_closed', False), challenge_list))
    assert len(challenge_list) >= nb_loop * 2
    challenge_list = list(map(lambda x: x['drm_config']['saas_challenge'], challenge_list))
    challenge_set = set(challenge_list)
    try:
        assert len(challenge_list) == len( challenge_set), "Found duplicate saas challenge"
    except AssertionError as e:
        dupl_dict = {}
        for e in challenge_set:
            nb = challenge_list.count(e)
            if nb > 1:
                print(f'challenge {e} appears {nb} times')
        raise
    logfile.remove()
    async_cb.assert_NoError()


@pytest.mark.no_parallel
def test_saas_challenge_quality_through_instances(accelize_drm, conf_json,
            cred_json, async_handler, log_file_factory):
    nb_loop = 5
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_file_append'] = True
    conf_json.save()
    for i in range(nb_loop):
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            drm_manager.activate()
    log_content = logfile.read()
    challenge_list = parse_and_save_challenge(log_content, REGEX_PATTERN)
    challenge_list = list(filter(lambda x: not x.get('is_closed', False), challenge_list))
    assert len(challenge_list) >= nb_loop * 2
    challenge_list = list(map(lambda x: x['drm_config']['saas_challenge'], challenge_list))
    challenge_set = set(challenge_list)
    try:
        assert len(challenge_list) == len( challenge_set), "Found duplicate saas challenge"
    except AssertionError as e:
        dupl_dict = {}
        for e in challenge_set:
            nb = challenge_list.count(e)
            if nb > 1:
                print(f'challenge {e} appears {nb} times')
        raise
    logfile.remove()
    async_cb.assert_NoError()

