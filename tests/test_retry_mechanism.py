# -*- coding: utf-8 -*-
"""
Test node-locked behavior of DRM Library.
"""
import pytest
import gc
from glob import glob
from os import remove, getpid
from os.path import getsize, isfile, dirname, join, realpath
from re import match, search, finditer, MULTILINE, IGNORECASE
from time import sleep, time
from json import loads
from datetime import datetime, timedelta
from tests.conftest import wait_func_true


@pytest.mark.on_2_fpga
def test_retry_disabled(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test retry mechanism on API function (not including the retry in background thread)
    The retry is tested with one FPGA activated with a floating license and a 2nd FGPA
    that's requesting the same floating license but with a limit to 1 node.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]

    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_04')

    conf_json.reset()
    retry = 0
    conf_json['settings']['ws_retry_period_short'] = retry  # Disable retry on function call
    conf_json.save()
    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSMayRetry) as excinfo:
            drm_manager1.activate()
        end = datetime.now()
        assert (end - start).total_seconds() < 1
        assert 'Metering Web Service error 470' in str(excinfo.value)
        assert 'DRM WS request failed' in str(excinfo.value)
        assert search(r'\\"Entitlement Limit Reached\\" with .+ for \S+_test_04@accelize.com', str(excinfo.value)) is not None
        assert 'You have reached the maximum quantity of 1 seat(s) for floating entitlement' in str(excinfo.value)
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSMayRetry.error_code
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        async_cb0.assert_NoError()
        async_cb1.assert_NoError()


@pytest.mark.on_2_fpga
def test_10s_retry(accelize_drm, conf_json, cred_json, async_handler):
    """
    Test retry mechanism on API function (not including the retry in background thread)
    The retry is tested with one FPGA actiavted with a floating license and a 2nd FGPA
    that's requesting the same floating license but with a limit to 1 node.
    """
    driver0 = accelize_drm.pytest_fpga_driver[0]
    driver1 = accelize_drm.pytest_fpga_driver[1]

    async_cb0 = async_handler.create()
    async_cb1 = async_handler.create()

    cred_json.set_user('accelize_accelerator_test_04')

    conf_json.reset()
    timeout = 10
    retry = 1
    conf_json['settings']['ws_request_timeout'] = timeout
    conf_json['settings']['ws_retry_period_short'] = retry
    conf_json.save()
    async_cb0.reset()
    drm_manager0 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver0.read_register_callback,
        driver0.write_register_callback,
        async_cb0.callback
    )
    async_cb1.reset()
    drm_manager1 = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver1.read_register_callback,
        driver1.write_register_callback,
        async_cb1.callback
    )
    assert not drm_manager0.get('license_status')
    assert not drm_manager1.get('license_status')
    try:
        drm_manager0.activate()
        assert drm_manager0.get('license_status')
        start = datetime.now()
        with pytest.raises(accelize_drm.exceptions.DRMWSTimedOut) as excinfo:
            drm_manager1.activate()
        end = datetime.now()
        m = search(r'Timeout on License request after (\d+) attempts', str(excinfo.value))
        assert m is not None
        assert int(m.group(1)) > 1
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMWSTimedOut.error_code
        total_seconds = int((end - start).total_seconds())
        assert total_seconds >= timeout
        assert total_seconds <= timeout + 1
    finally:
        drm_manager0.deactivate()
        assert not drm_manager0.get('license_status')
        assert not drm_manager1.get('license_status')
        async_cb0.assert_NoError()
        async_cb1.assert_NoError()


@pytest.mark.on_2_fpga
def test_long_to_short_retry_switch(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test an error is returned if a wrong session id is provided
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randrange(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    retryShortPeriod = 2
    retryLongPeriod = 10
    licenseValidity = 40

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] save the time when new request is sent to the server and
            # when the response from the server has been received
            request_json = request.get_json()
            if request_json['request'] == 'running':
                start = str(datetime.now())
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'running':
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
                response_json['metering']['healthRetrySleep'] = context['health_retry_sleep']
                if len(context['data']) >= 1:
                    response.status_code = 408
                if health_id <= 1:
                    context['data'].append( (health_id,start,str(datetime.now())) )
                else:
                    context['exit'] = True
            return Response(dumps(response_json), response.status_code, headers)

    fake_server.add_endpoint('/<path:path>', 'proxy', proxy, methods=['GET', 'POST'])

    for i in range(nb_run):
        retry_retry_sleep = tmpHealthRetrySleep + i
        context = {'url': url, 'data': list(), 'exit':False, 'health_retry_sleep':retry_retry_sleep}
        server = Process(target=fake_server.run, args=(PROXY_HOST, proxy_port))
        server.start()
        try:
            drm_manager = accelize_drm.DrmManager(
                conf_json.path,
                cred_json.path,
                driver.read_register_callback,
                driver.write_register_callback,
                async_cb.callback
            )
            drm_manager.activate()
            sleep(tmpHealthRetry)
            while not context['exit']:
                context = get(url=proxy_url+'/get/').json()
                sleep(1)
            drm_manager.deactivate()
            async_cb.assert_NoError()
            context = get(url=proxy_url+'/get/').json()
            data_list = context['data']
            data0 = data_list.pop(0)
            nb_sleep_prev = 0
            # Check the retry sleep period is correct
            for health_id, group in groupby(data_list, lambda x: x[0]):
                group = list(group)
                assert len(group) > nb_sleep_prev
                nb_sleep_prev = len(group)
                start = group.pop(0)[2]
                for _, lstart, lend in group:
                    delta = parser.parse(lstart) - parser.parse(start)
                    assert int(delta.total_seconds()) == retry_retry_sleep
                    start = lend
        finally:
            server.terminate()
            server.join()