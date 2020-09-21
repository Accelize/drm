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
    Improve coverage of ws_client.cpp/h
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
    Improve coverage of ws_client.cpp/h
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

