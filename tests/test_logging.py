# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import search, findall, finditer, MULTILINE
from time import sleep, time
from json import loads
from datetime import datetime, timedelta


LOG_FORMAT_SHORT = "[%^%=8l%$] %-6t, %v"
LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

REGEX_FORMAT_SHORT = r'\[\s*(\w+)\s*\] \s*\d+\s*, %s'
REGEX_FORMAT_LONG  = r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3} - \s*\S+:\d+\s* \[\s*(\w+)\s*\] \s*\d+\s*, %s'


## TEST LOG FILE

def test_file_path(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file verbosity"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./test_drmlib-%d.log" % getpid())
    log_type= 1

    async_cb.reset()
    if isfile(log_path):
        remove(log_path)
    assert not isfile(log_path)
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings']['log_file_path'] = log_path
    conf_json['settings']['log_file_type'] = log_type
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        del drm_manager
        gc.collect()
        assert isfile(log_path)
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)


def test_file_verbosity(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file verbosity"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.log" % getpid())
    log_type = 1
    msg = 'This is a %s message'
    level_dict = {0:'trace', 1:'debug', 2:'info', 3:'warning', 4:'error', 5:'critical'}

    for verbosity in range(len(level_dict)+1):
        async_cb.reset()
        if isfile(log_path):
            remove(log_path)
        assert not isfile(log_path)
        conf_json.reset()
        conf_json['settings']['log_verbosity'] = 6
        conf_json['settings']['log_file_format'] = LOG_FORMAT_LONG
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json.save()
        try:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            drm_manager.set(log_file_verbosity=verbosity)
            assert drm_manager.get('log_file_verbosity') == verbosity
            for i in sorted(level_dict.keys()):
                drm_manager.set(log_message_level=i)
                drm_manager.set(log_message=msg % level_dict[i])
            del drm_manager
            gc.collect()
            assert isfile(log_path)
            with open(log_path, 'rt') as f:
                log_content = f.read()
            regex = REGEX_FORMAT_LONG % (msg % '(.*)')
            trace_hit = 0
            for i, m in enumerate(finditer(regex, log_content)):
                assert m.group(1) == m.group(2)
                assert m.group(1) == level_dict[verbosity + i]
                trace_hit += 1
            assert trace_hit == 6-verbosity
            async_cb.assert_NoError()
        finally:
            if isfile(log_path):
                remove(log_path)


def test_file_format(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file formats"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.log" % getpid())
    log_type = 1
    msg = 'This is a message'
    regex_short = REGEX_FORMAT_SHORT % msg
    regex_long = REGEX_FORMAT_LONG % msg

    # Test logging with short format
    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings']['log_file_verbosity'] = 2
    conf_json['settings']['log_file_format'] = LOG_FORMAT_SHORT
    conf_json['settings']['log_file_path'] = log_path
    conf_json['settings']['log_file_type'] = log_type
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        drm_manager.set(log_message_level=2)
        drm_manager.set(log_message=msg)
        del drm_manager
        gc.collect()
        assert isfile(log_path)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        m = search(regex_short, log_content, MULTILINE)
        assert m is not None
        assert m.group(1) == 'info'
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)
    print('Test logging file short format: PASS')

    # Test logging with long format
    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings']['log_file_verbosity'] = 2
    conf_json['settings']['log_file_format'] = LOG_FORMAT_LONG
    conf_json['settings']['log_file_path'] = log_path
    conf_json['settings']['log_file_type'] = log_type
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        drm_manager.set(log_message_level=2)
        drm_manager.set(log_message=msg)
        del drm_manager
        gc.collect()
        assert isfile(log_path)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        m = search(regex_long, log_content, MULTILINE)
        assert m is not None
        assert m.group(1) == 'info'
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)
    print('Test logging file long format: PASS')


def test_file_types(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file types"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.log" % getpid())
    msg = 'This is a message'
    verbosity = 2
    rotating_size = 1024
    rotating_num = 5
    size = rotating_size * rotating_num

    for log_type in range(3):
        async_cb.reset()
        for f in glob(log_path[:-3] + '*log'):
            remove(f)
        assert not isfile(log_path)
        conf_json.reset()
        conf_json['settings']['log_verbosity'] = 6
        conf_json['settings']['log_file_verbosity'] = verbosity
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json['settings']['log_file_rotating_size'] = rotating_size
        conf_json['settings']['log_file_rotating_num'] = rotating_num
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        try:
            if log_type == 0:
                assert not isfile(log_path)
            else:
                assert isfile(log_path)
                assert rotating_size == drm_manager.get('log_file_rotating_size')
                assert rotating_num == drm_manager.get('log_file_rotating_num')
                drm_manager.set(log_message_level=verbosity)
                assert drm_manager.get('log_message_level') == verbosity
                for _ in range(2 * int(size / len(msg)) + 1):
                    drm_manager.set(log_message=msg)
                del drm_manager
                gc.collect()
                if log_type == 1:
                    # Basic file
                    assert getsize(log_path) >= 2 * size
                else:
                    # Rotating file
                    log_f_list = glob(log_path[:-3] + '*log')
                    assert len(log_f_list) == rotating_num + 1
                    for log_f in log_f_list:
                        assert isfile(log_f)
                        if log_f != log_path:
                            assert getsize(log_f) >= rotating_size / 2
                        assert getsize(log_f) < 2 * rotating_size
            async_cb.assert_NoError()
        finally:
            for f in glob(log_path[:-3] + '*log'):
                remove(f)


def test_file_rotating_parameters(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file rotating parameters"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.log" % getpid())
    log_type = 2
    msg = 'This is a message'
    verbosity = 2
    rotating_size = 1024
    rotating_num = 5

    async_cb.reset()
    for f in glob(log_path[:-3] + '*log'):
        remove(f)
    assert not isfile(log_path)
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings']['log_file_verbosity'] = verbosity
    conf_json['settings']['log_file_path'] = log_path
    conf_json['settings']['log_file_type'] = log_type
    conf_json['settings']['log_file_rotating_size'] = rotating_size
    conf_json['settings']['log_file_rotating_num'] = rotating_num
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        assert isfile(log_path)
        assert drm_manager.get('log_file_rotating_size') == rotating_size
        assert drm_manager.get('log_file_rotating_num') == rotating_num
        drm_manager.set(log_message_level=verbosity)
        assert drm_manager.get('log_message_level') == verbosity
        for _ in range(2 * rotating_num * int(rotating_size / len(msg) + 10)):
            drm_manager.set(log_message=msg)
        del drm_manager
        gc.collect()
        log_list = glob(log_path[:-3] + '*log')
        assert len(log_list) == rotating_num + 1
        index_list = list(range(rotating_num + 1))
        for log_f in log_list:
            assert isfile(log_f)
            assert getsize(log_f) < 2 * rotating_size
            m = search(r'drmlib-\d+(\.\d)?\.log', log_f)
            assert m is not None
            if m.group(1) is None:
                index = 0
            else:
                index = int(m.group(1)[1])
            assert index in index_list
            index_list.remove(index)
        assert len(index_list) == 0
        async_cb.assert_NoError()
    finally:
        for f in glob(log_path[:-3] + '*log'):
            remove(f)


def test_versions_displayed_in_log_file(accelize_drm, conf_json, cred_json, async_handler):
    """Test versions of dependent libraries are displayed in log file"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.log" % getpid())
    log_type = 1
    verbosity = 5

    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings']['log_file_verbosity'] = verbosity
    conf_json['settings']['log_file_path'] = log_path
    conf_json['settings']['log_file_type'] = log_type
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert drm_manager.get('log_file_verbosity') == verbosity
        del drm_manager
        gc.collect()
        assert isfile(log_path)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        assert search(r'drmlib\s*:\s*\d+\.\d+\.\d+', log_content, MULTILINE)
        assert search(r'libcurl\s*:(\s*\w+/\d+\.\d+(\.\d+)?)+\n', log_content, MULTILINE)
        assert search(r'jsoncpp\s*:\s*\d+\.\d+\.\d+\n', log_content, MULTILINE)
        assert search(r'spdlog\s*:\s*\d+\.\d+\.\d+\n', log_content, MULTILINE)
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)


def test_log_file_parameters_modifiability(accelize_drm, conf_json, cred_json, async_handler):
    """Once the log service has been created, test the parameters cannot be modified except verbosity and format """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_verbosity = 3
    log_path = realpath("./drmservice-%d.log" % getpid())
    log_format = LOG_FORMAT_LONG
    log_type = 2
    log_rotating_size = 10*1024
    log_rotating_num = 0

    # Test from config file

    async_cb.reset()
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings']['log_file_verbosity'] = log_verbosity
    conf_json['settings']['log_file_format'] = log_format
    conf_json['settings']['log_file_path'] = log_path
    conf_json['settings']['log_file_type'] = log_type
    conf_json['settings']['log_file_rotating_size'] = log_rotating_size
    conf_json['settings']['log_file_rotating_num'] = log_rotating_num
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert drm_manager.get('log_file_verbosity') == log_verbosity
        assert drm_manager.get('log_file_format') == log_format
        assert drm_manager.get('log_file_path') == log_path
        assert drm_manager.get('log_file_type') == log_type
        assert drm_manager.get('log_file_rotating_size') == log_rotating_size
        assert drm_manager.get('log_file_rotating_num') == log_rotating_num
        # Try to modify verbosity => authorized
        exp_value = log_verbosity - 1
        drm_manager.set(log_file_verbosity=exp_value)
        assert drm_manager.get('log_file_verbosity') == exp_value
        # Try to modify format => authorized
        exp_value = LOG_FORMAT_SHORT
        drm_manager.set(log_file_format=exp_value)
        assert drm_manager.get('log_file_format') == exp_value
        # Try to modify path => not authorized
        exp_value = realpath("./unexpected-%d.log" % getpid())
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(log_file_path=exp_value)
        assert drm_manager.get('log_file_path') == log_path
        # Try to modify rotating size => not authorized
        exp_value = int(log_rotating_size / 2)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(log_file_rotating_size=exp_value)
        assert drm_manager.get('log_file_rotating_size') == log_rotating_size
        # Try to modify rotating num => not authorized
        exp_value = int(log_rotating_num / 2)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(log_file_rotating_num=exp_value)
        assert drm_manager.get('log_file_rotating_num') == log_rotating_num
        del drm_manager
        gc.collect()
        assert isfile(log_path)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        critical_list = findall(r'[\s*critical\s*].* Parameter \S* cannot be overwritten', log_content)
        assert len(critical_list) == 3
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)
    print('Test log service parameters modifiability from config: PASS')
