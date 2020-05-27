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
from flask import redirect, request, Response
from requests import get, post
from dateutil import parser

PROXY_HOST = "127.0.0.1"


def test_health_check_period(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health period is correct.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    tmpHealthPeriod = 4
    tmpHealthRetry = 10
    context = {'url': url, 'data': list()}

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] gets the time between first health server answer and
            # the next health request which should be equal to the health period
            request_json = request.get_json()
            if request_json['request'] == 'health'
                print(len(context['data']) % 2)
            if request_json['request'] == 'health' and len(context['data']):
                context['data'].append(str(datetime.now()))
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                context['data'].append(str(datetime.now()))
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
            print(context['data'])
            return Response(dumps(response_json), response.status_code, headers)

    fake_server.add_endpoint('/<path:path>', 'proxy', proxy, methods=['GET', 'POST'])
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
        sleep(tmpHealthPeriod*2 - 1)
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        print(context['data'])
        assert len(context['data']) == 2
        delta =  parser.parse(context['data'][1]) -  parser.parse(context['data'][0])
        print('delta.total_seconds()=', delta.total_seconds())
        assert int(delta.total_seconds()) == tmpHealthPeriod

@pytest.mark.skip
def test_health_disabled(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health feature can be disabled.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    tmpHealthPeriod = 3
    tmpHealthRetry = 10
    context = {'url': url, 'data': 0}

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] gets the time between first health server answer and
            # the next health request which should be equal to the health period
            request_json = request.get_json()
            if len(context['data']) % 2:
                context['data'].append(str(datetime.now()))
            response = post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if request_json['request'] == 'health':
                if len(context['data'])  % 2 == 0:
                    context['data'].append(str(datetime.now()))
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    fake_server.add_endpoint('/<path:path>', 'proxy', proxy, methods=['GET', 'POST'])
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
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        assert context['data'] == 2

@pytest.mark.skip
def test_health_period_modified(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health feature can be modified dynamically.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    tmpHealthPeriod = 3
    tmpHealthRetry = 10
    context = {'url': url, 'data': 0}

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            request_json = request.get_json()
            response = post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if request_json['request'] == 'health':
                context['data'] += 1
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    fake_server.add_endpoint('/<path:path>', 'proxy', proxy, methods=['GET', 'POST'])
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
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        assert context['data'] == 2

@pytest.mark.skip
def test_health_normal(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health feature behaves as expected.
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()
    tmpHealthPeriod = 4
    tmpHealthRetry = 10
    context = {'url': url, 'data': list()}

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] gets the time between first health server answer and
            # the next health request which should be equal to the health period
            request_json = request.get_json()
            print(len(context['data']) % 2)
            if request_json['request'] == 'health' and len(context['data']) % 2:
                context['data'].append(str(datetime.now()))
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                if len(context['data']) % 2 == 0:
                    context['data'].append(str(datetime.now()))
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
            print(context['data'])
            return Response(dumps(response_json), response.status_code, headers)

    fake_server.add_endpoint('/<path:path>', 'proxy', proxy, methods=['GET', 'POST'])
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
        sleep(tmpHealthPeriod*2 - 1)
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        print(context['data'])
        assert len(context['data']) == 2
        delta =  parser.parse(context['data'][1]) -  parser.parse(context['data'][0])
        print('delta.total_seconds()=', delta.total_seconds())
        assert int(delta.total_seconds()) == tmpHealthPeriod

