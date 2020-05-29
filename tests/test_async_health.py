# -*- coding: utf-8 -*-
"""
Test asynchronous metering behaviors of DRM Library.
"""
from time import sleep
from random import randint
from datetime import datetime, timedelta
from re import search, MULTILINE
from json import loads, dumps
import pytest
from multiprocessing import Process
from flask import redirect, request, Response
from requests import get, post
from dateutil import parser
from copy import deepcopy

PROXY_HOST = "127.0.0.1"


def test_health_period_disabled(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health feature can be disabled.
    """
    from random import randrange
    from os import remove
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
        context = get(url=proxy_url+'/get/').json()
        cnt = context['data']
        assert cnt == nb_health
        assert wait_func_true(lambda: isfile(logpath), 10)
        with open(logpath, 'rt') as f:
            log_content = f.read()
        assert search(r'Health thread has been disabled', log_content, MULTILINE)
    finally:
        server.terminate()
        server.join()
        if isfile(logpath):
            remove(logpath)


@pytest.mark.minimum
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
            # context['data'] save the time when new request is sent to the server and
            # when the response from the served has been received
            request_json = request.get_json()
            if request_json['request'] == 'health':
                start = str(datetime.now())
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
                context['data'].append( (start,str(datetime.now())) )
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
            if len(context['data']) >= (nb_health + 1):
                break
        drm_manager.deactivate()
        async_cb.assert_NoError()
        context = get(url=proxy_url+'/get/').json()
        data_list = context['data']
        assert len(data_list) >= nb_health+1
        wait_start = data_list.pop(0)[1]
        for start, end in data_list:
            delta = parser.parse(start) - parser.parse(wait_start)
            assert int(delta.total_seconds()) == tmpHealthPeriod
            wait_start = end
    finally:
        server.terminate()
        server.join()


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
    context = {'url': url, 'data': list(), 'health_period':tmpHealthPeriod}
    nb_health = 5

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] save the time when new request is sent to the server and
            # when the response from the served has been received
            request_json = request.get_json()
            if request_json['request'] == 'health':
                start = str(datetime.now())
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                response_json['metering']['healthPeriod'] = context['health_period']
                response_json['metering']['healthRetry'] = tmpHealthRetry
                context['health_period'] += 1
                context['data'].append( (start,str(datetime.now())) )
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
        context = get(url=proxy_url+'/get/').json()
        data_list = context['data']
        assert len(data_list) >= nb_health+1
        wait_start = data_list.pop(0)[1]
        for i, (start, end) in enumerate(data_list):
            delta = parser.parse(start) - parser.parse(wait_start)
            assert int(delta.total_seconds()) == tmpHealthPeriod + i
            wait_start = end
    finally:
        server.terminate()
        server.join()


def test_health_retry_disabled(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health retry feature can be disabled.
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
    context = {'url': url, 'data': list()}
    nb_health = 4

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] save the time when new request is sent to the server and
            # when the response from the served has been received
            request_json = request.get_json()
            if request_json['request'] == 'health':
                health_id = request_json['health_id']
                start = str(datetime.now())
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = 0
                if len(context['data']) > 1:
                    response.status_code = 408
                context['data'].append( (health_id,start,str(datetime.now())) )
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
            if len(context['data']) >= nb_health + 1:
                break
        drm_manager.deactivate()
        async_cb.assert_NoError()
        context = get(url=proxy_url+'/get/').json()
        data_list = context['data']
        assert len(data_list) >= nb_health+1
        # Check there is no duplicated health_id
        id_list = tuple(map(lambda x: x[0], data_list))
        assert id_list == tuple(set(id_list))
        # Check the time between 2 period
        wait_start = data_list.pop(0)[2]
        for hid, start, end in data_list:
            delta = parser.parse(start) - parser.parse(wait_start)
            assert int(delta.total_seconds()) == tmpHealthPeriod
            wait_start = end
    finally:
        server.terminate()
        server.join()


@pytest.mark.minimum
def test_health_retry(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health retry.
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
    tmpHealthRetry = 20
    context = {'url': url, 'data': list(), 'exit':False}

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] save the time when new request is sent to the server and
            # when the response from the served has been received
            request_json = request.get_json()
            if request_json['request'] == 'health':
                health_id = request_json['health_id']
                start = str(datetime.now())
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = tmpHealthRetry
                if len(context['data']) >= 1:
                    response.status_code = 408
                if health_id <= 1:
                    context['data'].append( (health_id,start,str(datetime.now())) )
                else:
                    context['exit'] = True
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
        error_gap = drm_manager.get('ws_retry_period_short')
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
        assert len(data_list) > 1
        # Check health_id is unchanged during the retry period
        assert data0[0] == 0
        id_list = list(set(map(lambda x: x[0], data_list)))
        assert len(id_list) == 1
        assert id_list[0] == 1
        # Check the retry period is correct
        start = data_list[0][1]
        end = data_list[-1][2]
        delta = parser.parse(end) - parser.parse(start)
        assert tmpHealthRetry - error_gap <= int(delta.total_seconds()) <= tmpHealthRetry + error_gap
    finally:
        server.terminate()
        server.join()


def test_health_retry_modification(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test the asynchronous health retry can be modified dynamically.
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
    nb_run = 4

    def proxy(path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        elif path == 'get/':
            return context
        else:
            # context['data'] save the time when new request is sent to the server and
            # when the response from the served has been received
            request_json = request.get_json()
            if request_json['request'] == 'health':
                health_id = request_json['health_id']
                start = str(datetime.now())
            response = post(url_path, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if request_json['request'] == 'health':
                response_json['metering']['healthPeriod'] = tmpHealthPeriod
                response_json['metering']['healthRetry'] = context['health_retry']
                if len(context['data']) >= 1:
                    response.status_code = 408
                if health_id <= 1:
                    context['data'].append( (health_id,start,str(datetime.now())) )
                else:
                    context['exit'] = True
            return Response(dumps(response_json), response.status_code, headers)

    fake_server.add_endpoint('/<path:path>', 'proxy', proxy, methods=['GET', 'POST'])

    for i in range(nb_run):

        retry_period = tmpHealthRetry+5*i
        context = {'url': url, 'data': list(), 'exit':False, 'health_retry':retry_period}
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
            error_gap = drm_manager.get('ws_retry_period_short')
            drm_manager.activate()
            sleep(retry_period)
            while not context['exit']:
                context = get(url=proxy_url+'/get/').json()
                sleep(1)
            drm_manager.deactivate()
            async_cb.assert_NoError()
            context = get(url=proxy_url+'/get/').json()
            data_list = context['data']
            data0 = data_list.pop(0)
            assert len(data_list) > 1
            # Check health_id is unchanged during the retry period
            assert data0[0] == 0
            id_list = list(set(map(lambda x: x[0], data_list)))
            assert len(id_list) == 1
            assert id_list[0] == 1
            # Check the retry period is correct
            start = data_list[0][1]
            end = data_list[-1][2]
            delta = parser.parse(end) - parser.parse(start)
            assert retry_period - error_gap <= int(delta.total_seconds()) <= retry_period + error_gap
        finally:
            server.terminate()
            server.join()
