# -*- coding: utf-8 -*-
"""
Test asynchronous metering behaviors of DRM Library.
"""
from time import sleep
from random import randint
from datetime import datetime, timedelta
from re import search
from json import loads, dumps
import pytest
from multiprocessing import Process
import requests

PROXY_HOST = "127.0.0.1"

context = None


def test_health_normal(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous metering feature behaves as expected.
    """
    global context
    from flask import redirect, request, Response
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    tmpHealthPeriod = 10
    tmpHealthRetry = 3

    def proxy(context, path=''):
        print('path=', path)
        print('context=', context)
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            request_json = request.get_json()
            #print('request_json=', request_json)
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if request_json['request'] == 'health':
                context['cnt'] += 1
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
                print('response_json=', response_json)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    context = {'url': url, 'cnt': 0}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
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
        #sleep(tmpHealthPeriod*2 + 1)
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        server.terminate()
        server.join()
        print('context=', context)


@pytest.mark.skip
def test_health_disabled(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous metering feature behaves as expected.
    """
    from flask import redirect, request, Response
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    tmpHealthPeriod = 10
    tmpHealthRetry = 3

    def proxy(context, path=''):
        print('path=', path)
        print('context=', context)
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            request_json = request.get_json()
            print('request_json=', request_json)
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if request_json['request'] == 'health':
                context['cnt'] += 1
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    context = {'url': url, 'cnt': 0}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
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
        #sleep(tmpHealthPeriod*2 + 1)
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        server.terminate()
        server.join()


@pytest.mark.skip
def test_async_metering_dynamic_modification(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous metering feature behaves as expected.
    """
    from flask import request, redirect, Response, session
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    # Get license duration to align health period on it
    conf_json.reset()
    cred_json.reset()
    drm_manager = accelize_drm.DrmManager(
        conf_json.path,
        cred_json.path,
        driver.read_register_callback,
        driver.write_register_callback,
        async_cb.callback
    )
    drm_manager.activate()
    lic_duration = drm_manager.get('license_duration')
    drm_manager.get('dump_all')
    drm_manager.deactivate()
    del drm_manager

    # Create Fake server to modify the health period appropriately
    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    healthPeriod1 = 2
    healthPeriod2 = 4
    healthCntSwitch = 3

    def proxy(context, path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            lic_duration = context['lic_duration']
            request_json = request.get_json()
            if request_json['request'] == 'health':
                context['cnt'] += 1
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if context['cnt'] <= healthCntSwitch:
                response_json['metering']['healthPeriod'] = healthPeriod1
            else:
                response_json['metering']['healthPeriod'] = healthPeriod2
            if context['lastRequest']:
                if context['cnt'] <= healthCntSwitch:
                    delta = datetime.now() - context['lastRequest']
                    assert delta.total_seconds() >= healthPeriod1
                    assert delta.total_seconds() <= healthPeriod1 + 1
                else:
                    assert delta.total_seconds() >= healthPeriod2
                    assert delta.total_seconds() <= healthPeriod2 + 1
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    context = {'url':url, 'cnt':0, 'lastRecord':None, 'lic_duration':lic_duration}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
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
        sleep(tmpHealthPeriod*2 + 1)
        drm_manager.deactivate()
        async_cb.assert_NoError()
        print('healthCnt=', healthCnt)
        assert healthCnt == 2
    finally:
        server.terminate()
        server.join()
        print('context=', context)

