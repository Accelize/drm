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


def test_health_disabled(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health feature can be disabled.
    """
    from random import randrange
    from os.path import realpath, isfile
    from tests.conftest import wait_func_true

    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    logpath = realpath("./test_health_disabled.%d.log" % randrange(0xFFFFFFFF))
    conf_json['settings']['log_file_verbosity'] = 1
    conf_json['settings']['log_file_type'] = 1
    conf_json['settings']['log_file_path'] = logpath
    conf_json.save()
    tmpHealthPeriod = 2
    context = {'url':url, 'data':0}
    nb_health = 4

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            request_json = request.get_json()
            if request_json['request'] == 'health':
                context['data'] += 1
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health' and context['data'] < nb_health:
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
            else:
                response_json['metering']['healthPeriod'] = 0
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
        sleep(3)
        healthPeriod = drm_manager.get('health_period')
        sleep(healthPeriod*(nb_health+2))
        drm_manager.deactivate()
        del drm_manager
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        cnt = context['data']
        assert cnt == nb_health
        assert wait_func_true(lambda: isfile(logpath), 10)
        with open(log_path, 'rt') as f:
            log_content = f.read()
        assert search(r'Health thread has been disabled', log_content, MULTILINE)


def test_health_period(accelize_drm, conf_json, cred_json, async_handler, fake_server):
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
    tmpHealthPeriod = 2
    tmpHealthRetry = 10
    context = {'url': url, 'data': list()}
    nb_health = 4

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
        while True:
            sleep(tmpHealthPeriod)
            context = get(url=proxy_url+'/get/').json()
            if len(context['data']) >= (nb_health*2 + 1):
                break
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        time_list = context['data']
        assert len(time_list) >= 2*nb_health+1
        for i in range(int(len(time_list)/2)):
            delta =  parser.parse(time_list[2*i+1]) - parser.parse(time_list[2*i])
            assert int(delta.total_seconds()) == tmpHealthPeriod


def test_health_period_modification(accelize_drm, conf_json, cred_json, async_handler, fake_server):
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
    tmpHealthPeriod = 2
    tmpHealthRetry = 10
    context = {'url': url, 'data': list()}
    nb_health = 5

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
                tmpHealthPeriod += 1
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
        while True:
            sleep(tmpHealthPeriod)
            context = get(url=proxy_url+'/get/').json()
            if len(context['data']) >= (nb_health*2 + 1):
                break
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        time_list = context['data']
        assert len(time_list) >= 2*nb_health+1
        for i in range(int(len(time_list)/2)):
            delta =  parser.parse(time_list[2*i+1]) - parser.parse(time_list[2*i])
            assert int(delta.total_seconds()) == tmpHealthPeriod + i


@pytest.mark.skip
def test_health_retry(accelize_drm, conf_json, cred_json, async_handler, fake_server):
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
    tmpHealthPeriod = 2
    tmpHealthRetry = 10
    context = {'url': url, 'data': list()}
    nb_health = 5

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
                tmpHealthPeriod += 1
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
        while True:
            sleep(tmpHealthPeriod)
            context = get(url=proxy_url+'/get/').json()
            if len(context['data']) >= (nb_health*2 + 1):
                break
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        time_list = context['data']
        assert len(time_list) >= 2*nb_health+1
        for i in range(int(len(time_list)/2)):
            delta =  parser.parse(time_list[2*i+1]) - parser.parse(time_list[2*i])
            assert int(delta.total_seconds()) == tmpHealthPeriod + i


@pytest.mark.skip
def test_health_retry_modification(accelize_drm, conf_json, cred_json, async_handler, fake_server):
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
    tmpHealthPeriod = 2
    tmpHealthRetry = 10
    context = {'url': url, 'data': list()}
    nb_health = 5

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
                tmpHealthPeriod += 1
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
        while True:
            sleep(tmpHealthPeriod)
            context = get(url=proxy_url+'/get/').json()
            if len(context['data']) >= (nb_health*2 + 1):
                break
        drm_manager.deactivate()
        async_cb.assert_NoError()
    finally:
        context = get(url=proxy_url+'/get/').json()
        server.terminate()
        server.join()
        time_list = context['data']
        assert len(time_list) >= 2*nb_health+1
        for i in range(int(len(time_list)/2)):
            delta =  parser.parse(time_list[2*i+1]) - parser.parse(time_list[2*i])
            assert int(delta.total_seconds()) == tmpHealthPeriod + i
