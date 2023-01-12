# -*- coding: utf-8 -*-
"""
Run tests that help to improve coverage
"""
import pytest
from time import sleep
from flask import request as _request
from re import search, match, findall, IGNORECASE
from ctypes import c_uint, byref

from tests.conftest import HTTP_TIMEOUT_ERR_MSG
from tests.proxy import get_context, set_context, get_proxy_error


@pytest.mark.no_parallel
def test_improve_coverage_ws_client_on_code_600(accelize_drm, conf_json, cred_json,
                        async_handler, live_server, request, log_file_factory):
    """
    Improve coverage of the httpCode2DrmCode function when the server returns an error code 600
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager.activate()
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        assert get_proxy_error() is None
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSError.error_code, 'Accelize Web Service error 600 on HTTP request')
    async_cb.reset()
    log_content = logfile.read()
    assert search(r'Accelize Web Service error 600 on HTTP request', log_content)
    assert search(r'Generate error on purpose', log_content)
    logfile.remove()


@pytest.mark.no_parallel
def test_improve_coverage_wsclient_http_address_error(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Improve coverage of the request function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = 'http://100.100.100.100'
    conf_json['settings']['ws_api_retry_duration'] = 0
    conf_json['settings']['ws_request_timeout'] = 5
    conf_json.save()

    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    assert search(r'Failed to perform HTTP request ', str(excinfo.value), IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSMayRetry.error_code, HTTP_TIMEOUT_ERR_MSG)
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSMayRetry.error_code, 'Failed to perform HTTP request ')
    async_cb.reset()


def test_improve_coverage_getHostAndCardInfo(accelize_drm, conf_json, cred_json, async_handler,
                                             log_file_factory):
    """
    Improve coverage of the getHostAndCardInfo function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json['settings']['host_data_verbosity'] = 2
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        sleep(5)
        drm_manager.deactivate()
    assert search(r'Host and card information verbosity:\s*2', logfile.read(), IGNORECASE)
    logfile.remove()
    async_cb.assert_NoError()


def test_improve_coverage_readDrmAddress(accelize_drm, conf_json, cred_json, async_handler,
                                         log_file_factory):
    """
    Improve coverage of the readDrmAddress function
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves callbacks modification: skipped on SoM target (no callback provided for SoM)")

    def my_bad_read_register(register_offset, returned_data):
        return 123

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            my_bad_read_register,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
    assert search(r'Error in read register callback, errcode = 123: failed to read address', logfile.read(), IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMCtlrError.error_code, 'Unable to find DRM Controller registers')
    async_cb.reset()
    logfile.remove()


def test_improve_coverage_writeDrmAddress(accelize_drm, conf_json, cred_json, async_handler,
                                         log_file_factory):
    """
    Improve coverage of the writeDrmAddress function
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves callbacks modification: skipped on SoM target (no callback provided for SoM)")

    def my_bad_write_register(register_offset, data_to_write):
        return 123

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            my_bad_write_register,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
    assert search(r'Error in write register callback, errcode = 123: failed to write', logfile.read(), IGNORECASE)
    version_major = int(match(r'(\d+)\..+', accelize_drm.pytest_hdk_version).group(1))
    async_cb.assert_Error(accelize_drm.exceptions.DRMCtlrError.error_code, 'failed with error code 123')
    async_cb.reset()
    logfile.remove()


def test_improve_coverage_runBistLevel2_bad_size(accelize_drm, conf_json, cred_json, async_handler,
                                         log_file_factory):
    """
    Improve coverage of the runBistLevel2 function: generate bad mailbox size
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves callbacks modification: skipped on SoM target (no callback provided for SoM)")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    version_major = int(match(r'(\d+)\..+', accelize_drm.pytest_hdk_version).group(1))

    def my_bad_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if (version_major >= 8 and register_offset == 0xA004) or (
            version_major < 8 and ctx['page'] == 5 and register_offset == 4):
            returned_data.contents.value += 0x8000
        return ret

    def my_bad_write_register(register_offset, data_to_write, ctx):
        if register_offset == 0:
            ctx['page'] = data_to_write
        return driver.write_register_callback(register_offset, data_to_write)

    context = {'page':0}

    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            lambda x,y: my_bad_read_register(x,y, context),
            lambda x,y: my_bad_write_register(x,y, context),
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert search(r'DRM Communication Self-Test 2 failed: bad size', logfile.read(), IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'DRM Communication Self-Test 2 failed')
    async_cb.reset()
    logfile.remove()


def test_improve_coverage_runBistLevel2_bad_data(accelize_drm, conf_json, cred_json, async_handler,
                                         log_file_factory):
    """
    Improve coverage of the runBistLevel2 function: generate bad data
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves callbacks modification: skipped on SoM target (no callback provided for SoM)")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    version_major = int(match(r'(\d+)\..+', accelize_drm.pytest_hdk_version).group(1))

    def my_bad_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if (version_major >= 8 and register_offset == 0xA004) or (
            version_major < 8 and ctx['page'] == 5 and register_offset == 4):
            rw_size = returned_data.contents.value & 0xFFFF
            ro_size = returned_data.contents.value >> 16
            ctx['rwOffset'] = 4 * (ro_size + rw_size) + 4
        return ret

    def my_bad_write_register(register_offset, data_to_write, ctx):
        if register_offset == 0:
            ctx['page'] = data_to_write
        if version_major >= 8 or ctx['page'] == 5:
            if (register_offset & 0xFFF) >= ctx['rwOffset'] and data_to_write != 0 and data_to_write != 0xFFFFFFFF:
                data_to_write += 1
        return driver.write_register_callback(register_offset, data_to_write)

    context = {'page':0, 'rwOffset':0x10000}

    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            lambda x,y: my_bad_read_register(x,y, context),
            lambda x,y: my_bad_write_register(x,y, context),
            async_cb.callback
        )
    content = logfile.read()
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert len(findall(r'Mailbox\[\d+\]=0x[0-9A-F]{8} != 0x[0-9A-F]{8}', content, IGNORECASE)) == 1
    assert search(r'DRM Communication Self-Test 2 failed: random test failed.', content, IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'DRM Communication Self-Test 2 failed: random test failed')
    async_cb.reset()
    logfile.remove()


def test_improve_coverage_setLicense(accelize_drm, conf_json, cred_json, async_handler,
                            live_server, request, log_file_factory):
    """
    Improve coverage of the setLicense function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    with pytest.raises(accelize_drm.exceptions.DRMWSRespError) as excinfo:
        with accelize_drm.DrmManager(
                    conf_json.path,
                    cred_json.path,
                    driver.read_register_callback,
                    driver.write_register_callback,
                    async_cb.callback
                ) as drm_manager:
            drm_manager.activate()
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSRespError.error_code
    assert 'Malformed response from License Web Service:' in async_cb.message
    logfile.remove()


def test_improve_coverage_detectDrmFrequencyMethod1(accelize_drm, conf_json, cred_json, async_handler,
                                                    log_file_factory):
    """
    Improve coverage of the detectDrmFrequencyMethod1 function
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test involves DRM frequency: skipped on SoM target (no clock on DRM Ctrl Sw)")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['drm']['bypass_frequency_detection'] = True;
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
        drm_manager.activate()
        drm_manager.deactivate()
    log_content = logfile.read()
    assert search(r'Use dedicated counter to compute DRM frequency', log_content, IGNORECASE) is None
    assert search(r'Frequency detection counter after', log_content, IGNORECASE) is None
    async_cb.assert_NoError()
    logfile.remove()
