# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import sys
import gc
import re
from glob import glob
from os import remove
from os.path import getsize, isfile, dirname, join, realpath, basename
from re import search, findall, finditer, MULTILINE
from time import sleep, time
from json import loads, dumps
from datetime import datetime, timedelta
from random import randrange


SAMPLES_DUPLICATE_THRESHOLD = 2
SAMPLES_DISPERSION_THRESHOLD = 10
DUPLICATE_THRESHOLD = 1/10000*100.0
DISPERSION_THRESHOLD = 0.1

LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

REGEX_PATTERN = r'Starting license request to \S+ with request:\n(^{.+?\n}$)'


def parse_and_save_challenge(logpath, pattern, save_path=None):
    # Parse log file
    print(f'Parsing requests from log file: {logpath}')
    with open(logpath, 'rt') as f:
        text = f.read()
    request_json = dict()
    request_json['results'] = list()
    for challenge in finditer(pattern, text, re.I | re.MULTILINE | re.DOTALL):
        request_json['results'].append(loads(challenge.group(1)))
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
    num_duplicates = len(list(filter(lambda x: x>1, count_by_value.values())))
    duplication_percent = float(num_duplicates)/ list_size * 100
    print("=> Percentage of duplicates: %f%%" % duplication_percent)
    return duplication_percent


def check_bit_dispersion(value_list):
    """Compute the dispersion per bit on the results of the previous tests"""
    list_size = len(value_list)
    # Convert to binary string
    value_list = list(map(lambda x: bin(int(x, 16))[2:], value_list))
    value_len = max(map(lambda x: len(x), value_list))
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
    print('=> Dispersion score: %f' % score)
    return score


@pytest.mark.security
@pytest.mark.no_parallel
@pytest.mark.last
def test_global_challenge_quality():
    print()
    value_json = {'results' : list()}
    for filename in glob("./test_*.json"):
        if basename(filename).startswith('test_dna'):
            continue
        print('Analyzing file: %s ...' % filename)
        with open(filename, 'rt') as f:
            res_json = loads(f.read())
        value_json['results'].extend(res_json['results'])
        print('... adding %d more results' % len(res_json['results']))
    value_list = [e['saasChallenge'] for e in value_json['results'] if e['request']!='close']
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
        num_sessions = accelize_drm.pytest_params['num_open_sessions']
    except:
        num_sessions = 100
        print('Warning: Missing argument "num_open_sessions". Using default value %d' % num_sessions)
    try:
        num_samples = accelize_drm.pytest_params['num_open_samples']
    except:
        num_samples = 4
        print('Warning: Missing argument "num_open_samples". Using default value %d' % num_samples)
    print('num_open_sessions=', num_sessions)
    print('num_open_samples=', num_samples)

    async_cb.reset()
    conf_json.reset()
    logpath = realpath("./drmlib.%d.%d.log" % (time(), randrange()))
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json['settings']['log_file_format'] = LOG_FORMAT_LONG
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        session_cnt = 0
        while session_cnt < num_sessions:
            try:
                print('Starting session #%d/%d ...' % (session_cnt+1,num_sessions))
                activators.autotest(is_activated=False)
                drm_manager.activate()
                activators.autotest(is_activated=True)
                license_duration = drm_manager.get('license_duration')
                session_cnt += 1
                sleep(1)
                sample_cnt = 0
                for s in range(num_samples):
                    print('Waiting %d seconds' % license_duration)
                    sleep(license_duration)
                    if async_cb.was_called:
                        print('Error occurred in %s: background thread failed with message: %s' % (sys._getframe().f_code.co_name, async_cb.message))
                        break
            except:
                print('Session #%d/%d failed: retrying!' % (session_cnt+1,num_sessions))
            finally:
                while True:
                    try:
                        drm_manager.deactivate()
                        break
                    except:
                        print('Error occurred in %s: deactivate failed with message: %s' % (sys._getframe().f_code.co_name, async_cb.message))
                        sleep(1)
                activators.autotest(is_activated=False)
                async_cb.reset()
    finally:
        # Check validity
        assert session_cnt >= num_sessions
        del drm_manager
        gc.collect()
    # Parse log file
    request_json = parse_and_save_challenge(logpath, REGEX_PATTERN, 'test_first_challenge_duplication.%d.%d.json' % (time(), randrange()))
    # Keep only the 'open' requests
    request_json['results'] = list(filter(lambda x: x['request'] == 'open', request_json['results']))
    # Check validity
    assert len(request_json['results']) >= num_sessions
    # Check duplicates
    challenge_list = [e['saasChallenge'] for e in request_json['results']]
    dupl_score = check_duplicates(challenge_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD
    async_cb.assert_NoError()


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
        num_samples = accelize_drm.pytest_params['num_intra_samples']
    except:
        num_samples = 100
        print('Warning: Missing argument "num_intra_samples". Using default value %d' % num_samples)
    print('num_intra_samples=', num_samples)

    async_cb.reset()
    conf_json.reset()
    logpath = realpath("./drmlib.%d.%d.log" % (time(), randrange()))
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json['settings']['log_file_format'] = LOG_FORMAT_LONG
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
        activators.autotest(is_activated=True)
        license_duration = drm_manager.get('license_duration')
        print('License duration=%d, num of samples=%d' % (license_duration, num_samples))
        sleep(1)
        sample_cnt = 0
        while sample_cnt < num_samples:
            sleep(license_duration)
            if async_cb.was_called:
                print('Error occurred in %s: background thread failed with message: %s' % (sys._getframe().f_code.co_name, async_cb.message))
                async_cb.reset()
            else:
                sample_cnt += 1
                print('Num of samples=', sample_cnt)
    finally:
        while True:
            try:
                drm_manager.deactivate()
                break
            except:
                print('Error occurred in %s: deactivate failed with message: %s' % (sys._getframe().f_code.co_name, async_cb.message))
                sleep(1)
        activators.autotest(is_activated=False)
        # Check validity
        assert sample_cnt >= num_samples
        del drm_manager
        gc.collect()
    # Parse log file
    request_json = parse_and_save_challenge(logpath, REGEX_PATTERN, 'test_intra_challenge_duplication.%d.%d.json' % (time(), randrange()))
    # Remove close request because they repeat the last challenge
    request_json['results'] = list(filter(lambda x: x['request'] != 'close', request_json['results']))
    # Check validity
    assert len(request_json['results']) >= num_samples
    # Check duplicates
    challenge_list = [e['saasChallenge'] for e in request_json['results']]
    dupl_score = check_duplicates(challenge_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD
    async_cb.assert_NoError()


@pytest.mark.security
def test_dna_duplication(accelize_drm, conf_json, cred_json, async_handler):
    """Reprogram FPGA and display DNA.
    Purpose is to evaluate the quality of the DNA Challenge.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    activators = accelize_drm.pytest_fpga_activators[0]
    activators.autotest()
    cred_json.set_user('accelize_accelerator_test_05')
    try:
        num_samples = accelize_drm.pytest_params['num_dna_samples']
    except:
        num_samples = 50
        print('Warning: Missing argument "num_dna_samples". Using default value %d' % num_samples)
    print('num_dna_samples=', num_samples)

    async_cb.reset()
    conf_json.reset()
    logpath = realpath("./drmlib.%d.%d.log" % (time(), randrange()))
    conf_json['settings']['log_verbosity'] = 4
    conf_json['settings']['log_file_verbosity'] = 2
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()

    image_bkp = driver.fpga_image
    dna_list = list()
    while len(dna_list) < num_samples:
        try:
            print('Reset #%d/%d...' % (len(dna_list)+1, num_samples))
            driver.program_fpga(image_bkp)
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            drm_manager.get('hw_report')
            del drm_manager
            gc.collect()
            print(f'Parsing requests from log file: {logpath}')
            with open(logpath, 'rt') as f:
                log = f.read()
            dna_list.append(search(r'-\s*dna\s*\.+\s*:\s*0x([0-9A-F]+)', log, re.I).group(1))
        except:
            print('Last reset failed: retrying!')
    request_json = {'results': dna_list}
    # Check validity
    assert len(request_json['results']) >= num_samples
    # Save to file
    with open('test_dna_duplication.%d.%d.json' % (time(), randrange()), 'wt') as f:
        f.write(dumps(request_json, indent=4, sort_keys=True))
    # Check duplicates
    dupl_score = check_duplicates(dna_list)
    if dupl_score:
        assert dupl_score < DUPLICATE_THRESHOLD
    async_cb.assert_NoError()

