# -*- coding: utf-8 -*-
"""
Run tests that help to improve coverage
"""
import pytest
from time import sleep
from flask import request as _request
from re import search, findall, IGNORECASE
from ctypes import c_uint, byref

from tests.conftest import HTTP_TIMEOUT_ERR_MSG
from tests.proxy import get_context, set_context, get_proxy_error


@pytest.mark.no_parallel
def test_improve_coverage_ws_client(accelize_drm, conf_json, cred_json,
                        async_handler, live_server, request):
    """
    Improve coverage of the httpCode2DrmCode function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json.save()

    with accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        ) as drm_manager:

        # Set initial context on the live server
        error_code = 600
        context = {'error_code':error_code}
        set_context(context)
        assert get_context() == context

        with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
            drm_manager.activate()
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
        assert get_proxy_error() is None
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSError.error_code, 'Metering Web Service error 600')
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
        pytest.skip("Test skipped on SoM target")

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
        pytest.skip("Test skipped on SoM target")

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
    async_cb.assert_Error(accelize_drm.exceptions.DRMCtlrError.error_code, 'Unable to find DRM Controller registers')
    async_cb.reset()
    logfile.remove()


def test_improve_coverage_runBistLevel2_bad_size(accelize_drm, conf_json, cred_json, async_handler,
                                         log_file_factory):
    """
    Improve coverage of the runBistLevel2 function: generate bad mailbox size
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    def my_bad_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if ctx['page'] == 5 and register_offset == 4:
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
        pytest.skip("Test skipped on SoM target")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    def my_bad_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if ctx['page'] == 5 and register_offset == 4:
            rw_size = returned_data.contents.value & 0xFFFF
            ro_size = returned_data.contents.value >> 16
            ctx['rwOffset'] = 4 * (ro_size + rw_size) + 4
        return ret

    def my_bad_write_register(register_offset, data_to_write, ctx):
        if register_offset == 0:
            ctx['page'] = data_to_write
        if ctx['page'] == 5 and register_offset >= ctx['rwOffset'] and data_to_write != 0 and data_to_write != 0xFFFFFFFF:
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


def test_improve_coverage_getMeteringHeader(accelize_drm, conf_json, cred_json, async_handler,
                                            log_file_factory):
    """
    Improve coverage of the getMeteringHeader function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['design']['udid'] = '2fb8d54f-920e-4d69-aa56-197c7c72d8a3';
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
    content = logfile.read()
    assert search(r'Found parameter \'udid\' of type String: return its value "([0-9a-f]+-?)+"', content, IGNORECASE)
    assert len(findall(r'"udid"\s*:\s*"([0-9a-f]+-?)+"', content, IGNORECASE)) >= 2
    async_cb.assert_NoError()
    logfile.remove()


def test_improve_coverage_getDesignInfo(accelize_drm, conf_json, cred_json, async_handler,
                                        log_file_factory):
    """
    Improve coverage of the getDesignInfo function
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target")

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    logfile = log_file_factory.create(1)
    conf_json['settings'].update(logfile.json)
    conf_json.save()

    def my_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if ctx['page'] == 5 and register_offset == 4:
            ctx['cnt'] += 1
            if ctx['cnt'] == 11:
                returned_data.contents.value &= 0xFFFF
        return ret

    def my_write_register(register_offset, data_to_write, ctx):
        if register_offset == 0:
            ctx['page'] = data_to_write
        return driver.write_register_callback(register_offset, data_to_write)

    context = {'page':0, 'cnt':0}

    with pytest.raises(accelize_drm.exceptions.DRMBadArg) as excinfo:
        accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            lambda x,y: my_read_register(x,y, context),
            lambda x,y: my_write_register(x,y, context),
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert search(r'UDID and Product ID cannot be both missing', str(excinfo.value), IGNORECASE)
    assert search(r'Mailbox sizes: read-only=0', logfile.read(), IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMBadArg.error_code, 'UDID and Product ID cannot be both missing')
    async_cb.reset()
    logfile.remove()


def test_improve_coverage_setLicense(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, request):
    """
    Improve coverage of the setLicense function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        drm_manager.activate()
        sleep(4)
        drm_manager.deactivate()
    assert async_cb.was_called
    assert async_cb.errcode == accelize_drm.exceptions.DRMWSRespError.error_code
    assert 'Malformed response from License Web Service:' in async_cb.message


def test_improve_coverage_detectDrmFrequencyMethod1(accelize_drm, conf_json, cred_json, async_handler,
                                                    log_file_factory):
    """
    Improve coverage of the detectDrmFrequencyMethod1 function
    """
    if accelize_drm.is_ctrl_sw:
        pytest.skip("Test skipped on SoM target")

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


@pytest.mark.no_parallel
def test_improve_coverage_perform(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Improve coverage of the perform function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = 'http://100.100.100.100'
    conf_json['settings']['ws_api_retry_duration'] = 0
    conf_json['settings']['ws_request_timeout'] = 5
    conf_json.save()

    with accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            ) as drm_manager:
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager.activate()
        drm_manager.deactivate()
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    assert search(r'Failed to perform HTTP request to Accelize webservice', str(excinfo.value), IGNORECASE)
    async_cb.assert_Error(accelize_drm.exceptions.DRMWSMayRetry.error_code, HTTP_TIMEOUT_ERR_MSG)

    async_cb.assert_Error(accelize_drm.exceptions.DRMWSMayRetry.error_code, 'Failed to perform HTTP request to Accelize webservice')
    async_cb.reset()
