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
    connection_timeout = 7
    request_timeout = 11
    conf_json['settings']['ws_api_retry_duration'] = 0 # Disable retry
    conf_json['settings']['ws_connection_timeout'] = connection_timeout
    conf_json['settings']['ws_request_timeout'] = request_timeout
    conf_json['licensing']['url'] = _request.url + 'test_timeout'
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager.get('ws_connection_timeout') == connection_timeout
    assert not drm_manager.get('ws_request_timeout') == request_timeout

    context = {'sleep': connection_timeout + 1}
    set_context(context)
    assert get_context() == context

    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    del drm_manager
    assert connection_timeout - 1 <= (end - start).total_seconds() <= connection_timeout
    assert search(r'\[\s*critical\s*\]\s*\d+\s*,\s*\[errCode=\d+\]\s*Metering Web Service error 408',
            str(excinfo), IGNORECASE)
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
    connection_timeout = 11
    request_timeout = 7
    conf_json['settings']['ws_api_retry_duration'] = 0 # Disable retry
    conf_json['settings']['ws_connection_timeout'] = connection_timeout
    conf_json['settings']['ws_request_timeout'] = request_timeout
    conf_json['licensing']['url'] = _request.url + 'test_timeout'
    conf_json.save()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    assert not drm_manager.get('ws_connection_timeout') == connection_timeout
    assert not drm_manager.get('ws_request_timeout') == request_timeout

    context = {'sleep': request_timeout + 1}
    set_context(context)
    assert get_context() == context

    start = datetime.now()
    with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
        drm_manager.activate()
    end = datetime.now()
    del drm_manager
    assert request_timeout - 1 <= (end - start).total_seconds() <= request_timeout
    assert search(r'\[\s*critical\s*\]\s*\d+\s*,\s*\[errCode=\d+\]\s*Metering Web Service error 408',
            str(excinfo), IGNORECASE)
    async_cb.assert_NoError()

