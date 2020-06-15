# -*- coding: utf-8 -*-
"""
Test metering and floating behaviors of DRM Library.
"""
from time import sleep
from random import randint
from datetime import datetime, timedelta
from re import search
from json import loads, dumps
import pytest
from multiprocessing import Process
import requests

PROXY_HOST = "0.0.0.0"


@pytest.mark.no_parallel
def test_flask_view(fake_server):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    proxy_port = randint(1,65535)
    fake_server.run_bg(PROXY_HOST, proxy_port)
    try:
        proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
        print('proxy_url=', proxy_url)
        response = requests.get(proxy_url)
        print('status_code=', response.status_code)
        print('text=', response.text)
        assert response.status_code == 200
    finally:
        fake_server.stop_bg()


def test_flask_view(fake_server):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    from flask import request, redirect, Response, session
    proxy_port = randint(1,65535)
    server = Process(target=fake_server.run, args=(PROXY_HOST, proxy_port))
    server.start()
    try:
        proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
        print('proxy_url=', proxy_url)
        response = requests.get(url_path)
        print('response=', response.status_code)
        print('response=', response.text)
    finally:
        server.terminate()
        server.join()


@pytest.mark.no_parallel
def test_header_error_on_key(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    from flask import request, redirect, Response, session
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            context['cnt'] += 1
            request_json = request.get_json()
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if context['cnt'] == 1:
                dna, lic_json = list(response_json['license'].items())[0]
                key = lic_json['key']
                key = '1' + key[1:] if key[0] == '0' else '0' + key[1:]
                response_json['license'][dna]['key'] = key
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
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager.activate()
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
        assert "License header check error" in str(excinfo.value)
        async_cb.assert_NoError()
    finally:
        server.terminate()
        server.join()


@pytest.mark.skip
@pytest.mark.no_parallel
def test_mac_error_on_key(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    from flask import request, redirect, Response, session
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            context['cnt'] += 1
            request_json = request.get_json()
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if context['cnt'] == 1:
                dna, lic_json = list(response_json['license'].items())[0]
                key = lic_json['key']
                key = key[:-1] + '1' if key[-1] == '0' else key[:-1] + '0'
                response_json['license'][dna]['key'] = key
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
        with pytest.raises(accelize_drm.exceptions.DRMCtlrError) as excinfo:
            drm_manager.activate()
        assert async_handler.get_error_code(str(excinfo.value)) == accelize_drm.exceptions.DRMCtlrError.error_code
        assert "License MAC check error" in str(excinfo.value)
        async_cb.assert_NoError()
    finally:
        server.terminate()
        server.join()


@pytest.mark.no_parallel
def test_header_error_on_licenseTimer(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test a MAC error is returned if the licenseTimer value in the response has been modified
    """
    from flask import request, redirect, Response, session
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            context['cnt'] += 1
            request_json = request.get_json()
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if context['cnt'] == 2:
                dna, lic_json = list(response_json['license'].items())[0]
                timer = lic_json['licenseTimer']
                timer = '1' + timer[1:] if timer[0] == '0' else '0' + timer[1:]
                response_json['license'][dna]['licenseTimer'] = timer
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
        try:
            drm_manager.activate()
            start = datetime.now()
            lic_duration = drm_manager.get('license_duration')
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            wait_period = start + timedelta(seconds=lic_duration+2) - datetime.now()
            sleep(wait_period.total_seconds())
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert async_cb.was_called
            assert async_cb.message is not None
            assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
            assert "License header check error" in async_cb.message
    finally:
        server.terminate()
        server.join()


@pytest.mark.skip
@pytest.mark.no_parallel
def test_mac_error_on_licenseTimer(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test a MAC error is returned if the licenseTimer value in the response has been modified
    """
    from flask import request, redirect, Response, session
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            context['cnt'] += 1
            request_json = request.get_json()
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            if context['cnt'] == 2:
                dna, lic_json = list(response_json['license'].items())[0]
                timer = lic_json['licenseTimer']
                timer = timer[:-1] + '1' if timer[-1] == '0' else timer[:-1] + '0'
                response_json['license'][dna]['licenseTimer'] = timer
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
        try:
            drm_manager.activate()
            start = datetime.now()
            lic_duration = drm_manager.get('license_duration')
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            wait_period = start + timedelta(seconds=lic_duration+2) - datetime.now()
            sleep(wait_period.total_seconds())
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert async_cb.was_called
            assert async_cb.message is not None
            assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
            assert "License MAC check error" in async_cb.message
    finally:
        server.terminate()
        server.join()


@pytest.mark.no_parallel
def test_session_id_error(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test an error is returned if a wrong session id is provided
    """
    from flask import request, redirect, Response, session
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_port = randint(1,65535)
    proxy_url = "http://%s:%s" % (PROXY_HOST, proxy_port)
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        url_path = '%s/%s' % (context["url"],path)
        if path == 'o/token/':
            return redirect(url_path, code=307)
        else:
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            request_json = request.get_json()
            response = requests.post(url_path, json=request_json, headers=request.headers)
            response_json = response.json()
            response_session_id = response_json['metering']['sessionId']
            if context['session_id'] != response_session_id:
                context['session_cnt'] += 1
                context['request_cnt'] = 0
            context['request_cnt'] += 1
            context['session_id'] = response_session_id
            if context['session_cnt'] == 2:
                if context['request_cnt'] == 2:
                    response = context['replay']
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                return Response(response.content, response.status_code, headers)
            else:
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                if context['request_cnt'] == 2:
                    context['replay'] = response
                return Response(response.content, response.status_code, headers)

    context = {'url':url, 'session_id':None, 'session_cnt':0, 'request_cnt':0}
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
        try:
            # Start session #1 to record
            drm_manager.activate()
            start = datetime.now()
            lic_duration = drm_manager.get('license_duration')
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            wait_period = start + timedelta(seconds=lic_duration/2) - datetime.now()
            sleep(wait_period.total_seconds())
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            async_cb.assert_NoError()
        try:
            # Start session #2 to replay session #1
            drm_manager.activate()
            start = datetime.now()
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            lic_duration = drm_manager.get('license_duration')
            wait_period = start + timedelta(seconds=lic_duration+2) - datetime.now()
            sleep(wait_period.total_seconds())
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
        finally:
            drm_manager.deactivate()
            assert async_cb.was_called
            assert async_cb.message is not None
            assert async_cb.errcode == accelize_drm.exceptions.DRMCtlrError.error_code
            assert "License header check error" in async_cb.message
    finally:
        server.terminate()
        server.join()

