# -*- coding: utf-8 -*-
"""
Test logging mechanism of DRM Library.
"""
import pytest
from glob import glob
from os import remove, getpid, makedirs, access, R_OK, W_OK
from os.path import getsize, isfile, dirname, join, realpath, isdir, expanduser
from re import search, findall, finditer, MULTILINE
from time import time, sleep
from shutil import rmtree
from random import randrange
from tests.conftest import wait_func_true


LOG_FORMAT_SHORT = "[%^%=8l%$] %-6t, %v"
LOG_FORMAT_LONG = "%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v"

REGEX_FORMAT_SHORT = r'\[\s*(\w+)\s*\] \s*\d+\s*, %s'
REGEX_FORMAT_LONG  = r'\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3} - \s*\S+:\d+\s* \[\s*(\w+)\s*\] \s*\d+\s*, %s'


def test_file_path(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file path"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
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
        wait_func_true(lambda: isfile(log_path), 10)
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)


def test_file_verbosity(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file verbosity"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
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
            wait_func_true(lambda: isfile(log_path), 10)
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

    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
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
        wait_func_true(lambda: isfile(log_path), 10)
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
        wait_func_true(lambda: isfile(log_path), 10)
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

    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
    msg = 'This is a message'
    verbosity = 2
    rotating_size = 1
    rotating_num = 5
    size = rotating_size * 1024 * rotating_num

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
                wait_func_true(lambda: isfile(log_path), 10)
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
                            assert getsize(log_f) >= rotating_size*1024 / 2
                        assert getsize(log_f) < 2 * rotating_size*1024
            async_cb.assert_NoError()
        finally:
            for f in glob(log_path[:-3] + '*log'):
                remove(f)


def test_file_rotating_parameters(accelize_drm, conf_json, cred_json, async_handler):
    """Test logging file rotating parameters"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
    log_type = 2
    msg = 'This is a message'
    verbosity = 2
    rotating_size = 1
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
        for _ in range(2 * rotating_num * int(rotating_size*1024 / len(msg) + 10)):
            drm_manager.set(log_message=msg)
        del drm_manager
        wait_func_true(lambda: len(glob(log_path[:-3] + '*log')), 10)
        log_list = glob(log_path[:-3] + '*log')
        assert len(log_list) == rotating_num + 1
        index_list = list(range(rotating_num + 1))
        for log_f in log_list:
            assert isfile(log_f)
            assert getsize(log_f) < 2 * rotating_size*1024
            m = search(r'drmlib-\d+\.\d+(\.\d+)?\.log', log_f)
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

    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
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
        wait_func_true(lambda: isfile(log_path), 10)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        assert search(r'drmlib\s*:\s*\d+\.\d+\.\d+', log_content)
        assert search(r'libcurl\s*:(\s+[^/]+/\d+\.\d+(\.\d+)?)+', log_content)
        assert search(r'jsoncpp\s*:\s*\d+\.\d+\.\d+\n', log_content)
        assert search(r'spdlog\s*:\s*\d+\.\d+\.\d+\n', log_content)
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            remove(log_path)


def test_log_file_parameters_modifiability(accelize_drm, conf_json, cred_json, async_handler):
    """Once the log file has been created, test the parameters cannot be modified except verbosity and format """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_verbosity = 3
    log_path = realpath("./drmlib-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
    log_format = LOG_FORMAT_LONG
    log_type = 2
    log_rotating_size = 10  # =10KB
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
        exp_value = realpath("./unexpected-%d.%d.log" % (getpid(), randrange(0xFFFFFFFF)))
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
        wait_func_true(lambda: isfile(log_path), 10)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        critical_list = findall(r'\[\s*critical\s*\].* Parameter \S* cannot be overwritten', log_content)
        assert len(critical_list) == 3
        async_cb.assert_NoError()
    finally:
        if isfile(log_path):
            #remove(log_path)
            print(log_path)
    print('Test log file parameters modifiability from config: PASS')


def test_log_file_error_on_directory_creation(accelize_drm, conf_json, cred_json, async_handler):
    """ Test an error occurred when log file directory does not exist and cannot be created """
    from subprocess import check_call
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()

    log_type = 1
    log_dir = realpath(expanduser('~/tmp_log_dir.%s.%d' % (str(time()), randrange(0xFFFFFFFF))))
    if not isdir(log_dir):
        makedirs(log_dir)
    log_path = join(log_dir, "tmp", "drmlib.%d.%s.log" % (getpid(), str(time())))
    try:
        # Create immutable folder
        check_call('sudo chattr +i %s' % log_dir, shell=True)
        assert not access(log_dir, W_OK)
        assert not isdir(dirname(log_path))
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json.save()
        with pytest.raises(accelize_drm.exceptions.DRMExternFail) as excinfo:
            drm_manager = accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                )
        assert "Failed to create log file %s" % log_path in str(excinfo.value)
    finally:
        # Restore to mutable folder
        check_call('sudo chattr -i %s' % log_dir, shell=True)
        assert access(log_dir, W_OK)
        if isdir(log_dir):
            rmtree(log_dir)


def test_log_file_on_existing_directory(accelize_drm, conf_json, cred_json, async_handler):
    """ Test when log file directory already exists """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    log_type = 1
    log_dir = realpath(expanduser('~/tmp_log_dir.%s.%d' % (str(time()), randrange(0xFFFFFFFF))))
    if not isdir(log_dir):
        makedirs(log_dir)
    log_path = join(log_dir, "drmlib.%d.%s.log" % (getpid(), time()))
    try:
        assert isdir(log_dir)
        assert access(log_dir, W_OK)
        assert not isfile(log_path)
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        del drm_manager
        wait_func_true(lambda: isfile(log_path), 10)

    finally:
        if isdir(log_dir):
            rmtree(log_dir)


def test_log_file_directory_creation(accelize_drm, conf_json, cred_json, async_handler):
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    log_type = 1
    log_dir = realpath(expanduser('~/tmp_log_dir.%s.%d' % (str(time()), randrange(0xFFFFFFFF))))
    if not isdir(log_dir):
        makedirs(log_dir)
    log_path = join(log_dir, 'tmp', "drmlib.%d.%s.log" % (getpid(), time()))
    try:
        assert isdir(log_dir)
        assert access(log_dir, W_OK)
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        del drm_manager
        wait_func_true(lambda: isfile(log_path), 10)
    finally:
        if isdir(log_dir):
            rmtree(log_dir)


def test_log_file_without_credential_data(accelize_drm, conf_json, cred_json, async_handler):
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    log_type = 1
    log_dir = realpath(expanduser('~/tmp_log_dir.%s.%d' % (str(time()), randrange(0xFFFFFFFF))))
    if not isdir(log_dir):
        makedirs(log_dir)
    assert isdir(log_dir)
    assert access(log_dir, W_OK)
    log_path = join(log_dir, 'tmp', "drmlib.%d.%s.log" % (getpid(), time()))
    try:
        # Test with DEBUG level
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json['settings']['log_file_verbosity'] = 1
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        drm_manager.activate()
        sleep(1)
        drm_manager.deactivate()
        del drm_manager
        wait_func_true(lambda: isfile(log_path), 10)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        assert not search(cred_json['client_id'], log_content)
        assert not search(cred_json['client_secret'], log_content)

        # Test with DEBUG2 level
        async_cb.reset()
        conf_json.reset()
        conf_json['settings']['log_file_path'] = log_path
        conf_json['settings']['log_file_type'] = log_type
        conf_json['settings']['log_file_verbosity'] = 0
        conf_json.save()
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        drm_manager.activate()
        sleep(1)
        drm_manager.deactivate()
        del drm_manager
        wait_func_true(lambda: isfile(log_path), 10)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        assert not search(cred_json['client_id'], log_content)
        assert not search(cred_json['client_secret'], log_content)
    finally:
        if isdir(log_dir):
            rmtree(log_dir)
