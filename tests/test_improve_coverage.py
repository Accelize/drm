# -*- coding: utf-8 -*-
"""
Run tests that help to improve coverage
"""
import pytest
from time import sleep
from flask import request
from re import search, IGNORECASE

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
    def my_bad_read_register(register_offset, returned_data, context):
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
    def my_bad_write_register(register_offset, data_to_write, context):
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


def test_improve_coverage_runBistLevel2(accelize_drm, conf_json, cred_json, async_handler,
                                         basic_log_file):
    """
    Improve coverage of the runBistLevel2 function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json['settings'].update(basic_log_file.create(1))
    conf_json.save()

    context = {'page':0}

    def my_bad_read_register(register_offset, returned_data, ctx):
        ret = driver.read_register_callback(register_offset, returned_data)
        if ctx['page'] == 5 and register_offset == 4:
            returned_data = 0x10000 + 4
        return ret

    def my_bad_write_register(register_offset, data_to_write, ctx):
        if register_offset == 0:
            ctx['page'] = data_to_write
        return driver.write_register_callback(register_offset, data_to_write)

    with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
        drm_manager = accelize_drm.DrmManager(
            conf_json.path,
            cred_json.path,
            lambda x,y: my_bad_read_register(x,y, context),
            lambda x,y: my_bad_write_register(x,y, context),
            async_cb.callback
        )
    assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
    assert search(r'Error in write register callback, errcode = 123: failed to write', basic_log_file.read(), IGNORECASE)
    async_cb.assert_NoError()
    basic_log_file.remove()


def test_improve_coverage_getMeteringHeader(accelize_drm, conf_json, cred_json, async_handler):
    """
    Improve coverage of the getMeteringHeader function
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    json_output['design']['udid'] = 'udid';
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
