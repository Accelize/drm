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


def test_file_path(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file path"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create()
    conf_json.reset()
    conf_json['settings']['log_verbosity'] = 6
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('log_file_path') == logfile.path
    finally:
        pass
    logfile.read()
    async_cb.assert_NoError()
    logfile.remove()


def test_file_verbosity(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file verbosity"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    msg = 'This is a %s message'
    level_dict = {0:'trace', 1:'debug', 2:'info', 3:'warning', 4:'error', 5:'critical'}

    for verbosity in range(len(level_dict)+1):
        async_cb.reset()
        logfile = log_file_factory.create(2, 1, LOG_FORMAT_LONG)
        conf_json.reset()
        conf_json['settings'].update(logfile.json)
        conf_json['settings']['log_verbosity'] = 6
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
        finally:
            pass
        log_content = logfile.read()
        regex = REGEX_FORMAT_LONG % (msg % '(.*)')
        trace_hit = 0
        for i, m in enumerate(finditer(regex, log_content)):
            assert m.group(1) == m.group(2)
            assert m.group(1) == level_dict[verbosity + i]
            trace_hit += 1
        assert trace_hit == 6-verbosity
        async_cb.assert_NoError()
        log_content.remove()


def test_file_short_format(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file short format"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    msg = 'This is a message'
    regex_short = REGEX_FORMAT_SHORT % msg
    logfile = log_file_factory.create(2, 1, LOG_FORMAT_SHORT)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
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
    finally:
        pass
    log_content = logfile.read()
    m = search(regex_short, log_content, MULTILINE)
    assert m is not None
    assert m.group(1) == 'info'
    async_cb.assert_NoError()
    logfile.remove()


def test_file_long_format(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file long format"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    msg = 'This is a message'
    regex_long = REGEX_FORMAT_LONG % msg
    logfile = log_file_factory.create(2, 1, LOG_FORMAT_LONG)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
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
    finally:
        pass
    log_content = logfile.read()
    m = search(regex_long, log_content, MULTILINE)
    assert m is not None
    assert m.group(1) == 'info'
    async_cb.assert_NoError()
    logfile.remove()


def test_file_types(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file types"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    msg = 'This is a message'
    rotating_size = 1
    rotating_num = 5
    size = rotating_size * 1024 * rotating_num
    for log_type in range(3):
        logfile = log_file_factory.create(2, log_type, LOG_FORMAT_LONG, False, rotating_size, rotating_num)
        async_cb.reset()
        conf_json.reset()
        conf_json['settings'].update(logfile.json)
        conf_json['settings']['log_verbosity'] = 6
        conf_json.save()
        try:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            drm_manager.set(log_message_level=verbosity)
            assert drm_manager.get('log_message_level') == logfile.verbosity
            for _ in range(2 * int(size / len(msg)) + 1):
                drm_manager.set(log_message=msg)
        finally:
            pass
        if log_type == 0:
            assert not isfile(logfile.path)
        elif log_type == 1:
            # Basic file
            log_content = logfile.read()
            assert len(log_content) >= 2 * size
        else:
            # Rotating file
            assert rotating_size == drm_manager.get('log_file_rotating_size')
            assert rotating_num == drm_manager.get('log_file_rotating_num')
            for i in range(rotating_num+1):
                log_content = logfile.read(i)
                if i == 0:
                    assert len(log_content) < 2 * rotating_size*1024
                else:
                    assert len(log_content) >= rotating_size*1024 / 2
        async_cb.assert_NoError()
        logfile.remove()


def test_file_append(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file append mode"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(type=1, append=True)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
    conf_json.save()
    nb_loop = 5
    for i in range(nb_loop):
        try:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        finally:
            pass
    log_content.read()
    assert len(findall(r'Installed versions', log_content)) == nb_loop
    async_cb.assert_NoError()
    log_content.remove()


def test_file_truncate(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file truncate mode"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(type=1, append=False)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
    conf_json.save()
    nb_loop = 5
    for i in range(nb_loop):
        try:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        finally:
            pass
    log_content = logfile.read()
    assert len(findall(r'Installed versions', log_content)) == 1
    async_cb.assert_NoError()
    logfile.remove()


def test_file_rotating_parameters(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test logging file rotating parameters"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(verbosity=2, type=2,
                             rotating_size_kb=1, rotating_num=5)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
    conf_json.save()
    msg = 'This is a message'
    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        assert drm_manager.get('log_file_rotating_size') == logfile.rotating_size_kb
        assert drm_manager.get('log_file_rotating_num') == logfile.rotating_num
        drm_manager.set(log_message_level=logfile.verbosity)
        assert drm_manager.get('log_message_level') == logfile.verbosity
        for _ in range(2 * logfile.rotating_num * int(logfile.rotating_size_kb*1024 / len(msg) + 10)):
            drm_manager.set(log_message=msg)
    finally:
        pass
    for i in range(logfile.rotating_num+1):
        log_content = logfile.read(i)
        assert len(log_content) < 2 * logfile.rotating_size_kb*1024
    try:
        logfile.read(logfile.rotating_num+1)
    except IOError:
        pass
    async_cb.assert_NoError()
    logfile.remove()


def test_versions_displayed_in_log_file(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Test versions of dependent libraries are displayed in log file"""
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(verbosity=5, type=1)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert drm_manager.get('log_file_verbosity') == logfile.verbosity
    finally:
        pass
    log_content = logfile.read()
    assert search(r'drmlib\s*:\s*\d+\.\d+\.\d+', log_content)
    assert search(r'libcurl\s*:(\s+[^/]+/\d+\.\d+(\.\d+)?)+', log_content)
    assert search(r'jsoncpp\s*:\s*\d+\.\d+\.\d+\n', log_content)
    assert search(r'spdlog\s*:\s*\d+\.\d+\.\d+\n', log_content)
    async_cb.assert_NoError()
    logfile.remove()


def test_log_file_parameters_modifiability(accelize_drm, conf_json, cred_json, async_handler, request,
                                        log_file_factory):
    """Once the log file has been created, test the parameters cannot be modified except verbosity and format """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(verbosity=3, type=2, format=LOG_FORMAT_LONG,
                                rotating_size_kb=10, rotating_num=0)
    # Test from config file
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['log_verbosity'] = 6
    conf_json.save()
    try:
        drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
        assert drm_manager.get('log_file_verbosity') == logfile.verbosity
        assert drm_manager.get('log_file_format') == logfile.format
        assert drm_manager.get('log_file_path') == logfile.path
        assert drm_manager.get('log_file_type') == logfile.type
        assert drm_manager.get('log_file_rotating_size') == logfile.rotating_size_kb
        assert drm_manager.get('log_file_rotating_num') == logfile.rotating_num
        # Try to modify verbosity => authorized
        exp_value = logfile.verbosity - 1
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
        assert drm_manager.get('log_file_path') == logfile.path
        # Try to modify rotating size => not authorized
        exp_value = int(logfile.rotating_size_kb / 2)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(logfile.rotating_size_kb=exp_value)
        assert drm_manager.get('log_file_rotating_size') == logfile.rotating_size_kb
        # Try to modify rotating num => not authorized
        exp_value = int(logfile.rotating_num / 2)
        with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
            drm_manager.set(logfile.rotating_num=exp_value)
        assert drm_manager.get('log_file_rotating_num') == logfile.rotating_num
    finally:
        pass
    log_content = logfile.read()
    critical_list = findall(r'\[\s*critical\s*\].* Parameter \S* cannot be overwritten', log_content)
    assert len(critical_list) == 3
    async_cb.assert_NoError()
    logfile.remove()


def test_log_file_error_on_directory_creation(accelize_drm, conf_json, cred_json, async_handler):
    """ Test an error occurred when log file directory does not exist and cannot be created """
    from subprocess import check_call
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
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
    async_cb.reset()
    log_type = 1
    log_dir = realpath(expanduser('~/tmp_log_dir.%s.%d' % (str(time()), randrange(0xFFFFFFFF))))
    if not isdir(log_dir):
        makedirs(log_dir)
    log_path = join(log_dir, "drmlib.%d.%s.log" % (getpid(), time()))
    assert isdir(log_dir)
    assert access(log_dir, W_OK)
    assert not isfile(log_path)
    conf_json.reset()
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
    finally:
        pass
    wait_func_true(lambda: isfile(log_path), 10)
    if isdir(log_dir):
        rmtree(log_dir)


def test_log_file_directory_creation(accelize_drm, conf_json, cred_json, async_handler):
    """ Test the non existing sub-directories in the log file path are created by the DRMLib """
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
    finally:
        pass
    wait_func_true(lambda: isfile(log_path), 10)
    if isdir(log_dir):
        rmtree(log_dir)


def test_log_file_without_credential_data_in_debug(accelize_drm, conf_json, cred_json, async_handler,
                                        log_file_factory, request):
    """ Test no credential information is saved into log file """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(verbosity=1, type=1)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    try:
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
    finally:
        pass
    log_content = logfile.read()
    assert not search(cred_json['client_id'], log_content)
    assert not search(cred_json['client_secret'], log_content)
    logfile.remove()


def test_log_file_without_credential_data_in_debug2(accelize_drm, conf_json, cred_json, async_handler,
                                        log_file_factory, request):
    """ Test no credential information is saved into log file """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(verbosity=0, type=1)
    conf_json.reset()
    conf_json['settings'].update(logfile.json)
    conf_json.save()
    try:
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
    finally:
        pass
    log_content = logfile.read()
    assert not search(cred_json['client_id'], log_content)
    assert not search(cred_json['client_secret'], log_content)
    logfile.remove()
