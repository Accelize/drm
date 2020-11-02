# -*- coding: utf-8 -*-
"""
Test web service connection and request timeout.
"""
import pytest
from re import match, search, finditer, findall, MULTILINE, IGNORECASE
from datetime import datetime, timedelta
from flask import request as _request

from tests.proxy import get_context, set_context


def test_connection_timeout(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, request):
    """
    Test the connection timeout is respected
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    connection_timeout = 3
    request_timeout = 10
    conf_json['settings']['ws_api_retry_duration'] = 0 # Disable retry
    conf_json['settings']['ws_connection_timeout'] = connection_timeout
    conf_json['settings']['ws_request_timeout'] = request_timeout
    conf_json['licensing']['url'] = 'http://100.100.100.100'
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('ws_connection_timeout') == connection_timeout
    assert drm_manager.get('ws_request_timeout') == request_timeout

    context = {'sleep': connection_timeout + 10}
    set_context(context)
    assert get_context() == context

    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    del drm_manager
    assert connection_timeout - 1 <= int((end - start).total_seconds()) <= connection_timeout
    assert search(r'\[errCode=\d+\]\s*Failed to perform HTTP request to Accelize webservice \(Timeout was reached\) : Connection timed out after [%d%d]\d{3} milliseconds' % (connection_timeout-1,connection_timeout),
            str(excinfo.value), IGNORECASE)
    async_cb.assert_NoError()


def test_request_timeout(accelize_drm, conf_json, cred_json, async_handler,
                        live_server, request):
    """
    Test the request timeout is respected
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()
    conf_json.reset()
    connection_timeout = 10
    request_timeout = 3
    conf_json['settings']['ws_api_retry_duration'] = 0 # Disable retry
    conf_json['settings']['ws_connection_timeout'] = connection_timeout
    conf_json['settings']['ws_request_timeout'] = request_timeout
    conf_json['licensing']['url'] = _request.url + request.function.__name__
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert drm_manager.get('ws_connection_timeout') == connection_timeout
    assert drm_manager.get('ws_request_timeout') == request_timeout

    context = {'sleep': request_timeout + 1}
    set_context(context)
    assert get_context() == context

    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    del drm_manager
    assert request_timeout - 1 <= int((end - start).total_seconds()) <= request_timeout
    assert search(r'\[errCode=\d+\]\s*Failed to perform HTTP request to Accelize webservice \(Timeout was reached\) : Operation timed out after [%d%d]\d+ milliseconds' % (connection_timeout-1,connection_timeout),
            str(excinfo.value), IGNORECASE)
    async_cb.assert_NoError()

