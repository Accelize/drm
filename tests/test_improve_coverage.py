# -*- coding: utf-8 -*-
"""
Run tests that help to improve coverage
"""
import pytest
from time import sleep
from flask import request
from re import search, findall, IGNORECASE
from ctypes import c_uint, byref

from tests.proxy import get_context, set_context, get_proxy_error


@pytest.mark.no_parallel
def test_improve_coverage_httpCode2DrmCode(accelize_drm, conf_json, cred_json, async_handler, live_server):
    """
    Improve coverage of the httpCode2DrmCode function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['licensing']['url'] = request.url + 'test_improve_coverage_ws_client'
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )

    # Set initial context on the live server
    error_code = 600
    context = {'error_code':error_code}
    set_context(context)
    assert get_context() == context

    with pytest.raises(accelize_drm.exceptions.DRMWSError) as excinfo:
        drm_manager.activate()
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSError.error_code
    assert get_proxy_error() is None
    async_cb.assert_NoError()


def test_improve_coverage_getHostAndCardInfo(accelize_drm, conf_json, cred_json, async_handler,
                                             basic_log_file):
    """
    Improve coverage of the getHostAndCardInfo function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['settings'].update(basic_log_file.create(1))
    conf_json['settings']['host_data_verbosity'] = 2
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
        sleep(5)
    finally:
        drm_manager.deactivate()
    assert search(r'Host and CSP information verbosity:\s*2', basic_log_file.read(), IGNORECASE)
    basic_log_file.remove()
    async_cb.assert_NoError()


def test_improve_coverage_readDrmAddress(accelize_drm, conf_json, cred_json, async_handler,
                                         basic_log_file):
    """
    Improve coverage of the readDrmAddress function
    """
    def my_bad_read_register(register_offset, returned_data):
        return 123

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['settings'].update(basic_log_file.create(1))
    conf_json.save()

    with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            my_bad_read_register,
            driver.write_register_callback,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
    assert search(r'Error in read register callback, errcode = 123: failed to read address', basic_log_file.read(), IGNORECASE)
    async_cb.assert_NoError()
    basic_log_file.remove()


def test_improve_coverage_writeDrmAddress(accelize_drm, conf_json, cred_json, async_handler,
                                         basic_log_file):
    """
    Improve coverage of the writeDrmAddress function
    """
    def my_bad_write_register(register_offset, data_to_write):
        print('register_offset, data_to_write =', register_offset, data_to_write)
        return 123

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['settings'].update(basic_log_file.create(1))
    conf_json.save()

    with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            my_bad_write_register,
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
    assert search(r'Error in write register callback, errcode = 123: failed to write', basic_log_file.read(), IGNORECASE)
    async_cb.assert_NoError()
    basic_log_file.remove()


def test_improve_coverage_runBistLevel2_bad_size(accelize_drm, conf_json, cred_json, async_handler,
                                         basic_log_file):
    """
    Improve coverage of the runBistLevel2 function: generate bad mailbox size
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['settings'].update(basic_log_file.create(1))
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
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            lambda x,y: my_bad_read_register(x,y, context),
            lambda x,y: my_bad_write_register(x,y, context),
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert search(r'DRM Communication Self-Test 2 failed: bad size', basic_log_file.read(), IGNORECASE)
    async_cb.assert_NoError()
    basic_log_file.remove()


def test_improve_coverage_runBistLevel2_bad_data(accelize_drm, conf_json, cred_json, async_handler,
                                         basic_log_file):
    """
    Improve coverage of the runBistLevel2 function: generate bad data
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['settings'].update(basic_log_file.create(1))
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
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            lambda x,y: my_bad_read_register(x,y, context),
            lambda x,y: my_bad_write_register(x,y, context),
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMBadArg.error_code
    assert len(findall(r'Mailbox\[\d+\]=0x[0-9A-F]{8} != 0x[0-9A-F]{8}', basic_log_file.read(), IGNORECASE)) == 1
    assert search(r'DRM Communication Self-Test 2 failed: random test failed.', basic_log_file.read(), IGNORECASE)
    async_cb.assert_NoError()
    basic_log_file.remove()


def test_improve_coverage_getDesignInfo(accelize_drm, conf_json, cred_json, async_handler):
    """
    Improve coverage of the getDesignInfo function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    def my_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if ctx['page'] == 5 and register_offset == 4:
            returned_data.contents.value = 0
        return ret

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()


def test_improve_coverage_getMeteringHeader(accelize_drm, conf_json, cred_json, async_handler):
    """
    Improve coverage of the getMeteringHeader function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['design']['udid'] = '2fb8d54f-920e-4d69-aa56-197c7c72d8a3';
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()


def test_improve_coverage_detectDrmFrequencyMethod1(accelize_drm, conf_json, cred_json, async_handler):
    """
    Improve coverage of the detectDrmFrequencyMethod1 function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    json_output['settings']['bypass_frequency_detection'] = True;
    conf_json.save()

    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    try:
        drm_manager.activate()
    finally:
        drm_manager.deactivate()
    async_cb.assert_NoError()


def test_improve_coverage_detectDrmFrequencyMethod2(accelize_drm, conf_json, cred_json, async_handler):
    """
    Improve coverage of the detectDrmFrequencyMethod2 function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    fpga_image_bkp = driver.fpga_image
    async_cb = async_handler.create()
    async_cb.reset()
    refdesign = accelize_drm.pytest_ref_designs
    # First instanciate an object to get the HDK compatbility version
    try:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            driver.read_register_callback,
            driver.write_register_callback,
            async_cb.callback
        )
        HDK_Limit = float(drm_manager.get('hdk_compatibility'))
        del drm_manager
    except:
        HDK_Limit = 3.1
    # Then test all HDK versions that are compatible
    refdesignByMajor = ((float(match(r'^(\d+.\d+)', x).group(1)), x) for x in refdesign.hdk_versions)
    tested = False
    for num, versions in groupby(refdesignByMajor, lambda x: x[0]):
        if num >= HDK_Limit:
            # Program FPGA with lastest HDK per major number
            print('Testing HDK version %s is compatible ...' % num)
            hdk = sorted((e[1] for e in versions))[-1]
            image_id = refdesign.get_image_id(hdk)
            driver.program_fpga(image_id)
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            try:
                drm_manager.activate()
            finally:
                drm_manager.deactivate()
            async_cb.assert_NoError()
            break
