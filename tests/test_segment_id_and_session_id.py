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
from flask import request, redirect, Response, session
import requests

PROXY_HOST = "127.0.0.1"
PROXY_PORT = 8080

@pytest.mark.skip
def test_mac_error_on_licenseTimer(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test a MAC error is returned if the licesnseTimer value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_url = f"http://{PROXY_HOST}:{PROXY_PORT}"
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        if path == 'o/token/':
            return redirect(f'{context["url"]}/{path}', code=307)
        else:
            context['cnt'] += 1
            print("\t********* context['cnt']=", context['cnt'])
            data = request.get_json()
            print('request=', data['request'], data['meteringFile'])
            response = requests.post(f'{context["url"]}/{path}', json=request.get_json(), headers=request.headers)
            response_json = response.json()
            if context['cnt'] == 2:
                for e in response_json['license'].keys():
                    timer = response_json['license'][e]['licenseTimer']
                    if timer[0] == '0':
                        timer = '1' + timer[1:]
                    else:
                        timer = '0' + timer[1:]
                    response_json['license'][e]['licenseTimer'] = timer
                    break
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            print('response=', response_json)
            return Response(dumps(response_json), response.status_code, headers)

    context = {'url': url, 'cnt': 0}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
    proxy_debug = accelize_drm.pytest_proxy_debug
    server = Process(target=fake_server.run, args=(PROXY_HOST, PROXY_PORT, proxy_debug))
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
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert async_cb.was_called
            assert async_cb.message is not None
            assert "Reached an unexpected part of code" in async_cb.message
            assert "License MAC check error" in async_cb.message
            assert async_cb.errcode == accelize_drm.exceptions.DRMAssert.error_code
    finally:
        server.terminate()
        server.join()

@pytest.mark.skip
def test_mac_error_on_key(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test a MAC error is returned if the key value in the response has been modified
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_url = f"http://{PROXY_HOST}:{PROXY_PORT}"
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        if path == 'o/token/':
            return redirect(f'{context["url"]}/{path}', code=307)
        else:
            context['cnt'] += 1
            print("\t********* context['cnt']=", context['cnt'])
            data = request.get_json()
            print('request=', data['request'], data['meteringFile'])
            response = requests.post(f'{context["url"]}/{path}', json=request.get_json(), headers=request.headers)
            response_json = response.json()
            if context['cnt'] == 2:
                for e in response_json['license'].keys():
                    key = response_json['license'][e]['key']
                    if key[0] == '0':
                        key = '1' + key[1:]
                    else:
                        key = '0' + key[1:]
                    response_json['license'][e]['licenseTimer'] = key
                    break
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            print('response=', response_json)
            return Response(dumps(response_json), response.status_code, headers)

    context = {'url': url, 'cnt': 0}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
    proxy_debug = accelize_drm.pytest_proxy_debug
    server = Process(target=fake_server.run, args=(PROXY_HOST, PROXY_PORT, proxy_debug))
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
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert async_cb.was_called
            assert async_cb.message is not None
            assert "Reached an unexpected part of code" in async_cb.message
            assert "License MAC check error" in async_cb.message
            assert async_cb.errcode == accelize_drm.exceptions.DRMAssert.error_code
    finally:
        server.terminate()
        server.join()

@pytest.mark.skip
def test_session_id_error(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test an error is returned if a wrong session id is provided
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_url = f"http://{PROXY_HOST}:{PROXY_PORT}"
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        if path == 'o/token/':
            return redirect(f'{context["url"]}/{path}', code=307)
        else:
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            request_json = request.get_json()
            response = requests.post(f'{context["url"]}/{path}', json=request_json, headers=request.headers)
            response_json = response.json()
            response_session_id = response_json['metering']['sessionId']
            if context['session_id'] and context['session_id'] != response_session_id:
                context['session_cnt'] += 1
                context['request_cnt'] = 0
            context['request_cnt'] += 1
            context['session_id'] = response_session_id
            print("\t******* session_cnt =", context['session_cnt'], ', request_cnt =', context['request_cnt'])
            print('request=', request_json['request'], request_json['meteringFile'])
            if context['session_cnt'] == 1:
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                if context['request_cnt'] == 2:
                    context['replay'] = response
                    print('save=', response.json())
                else:
                    print('response=', response.json())
                return Response(response.content, response.status_code, headers)
            elif context['session_cnt'] == 2:
                if context['request_cnt'] == 2:
                    response = context['replay']
                    print('replay=', response.json())
                else:
                    print('response=', response.json())
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                return Response(response.content, response.status_code, headers)
            else:
                raise RuntimeError('Unexpected session_cnt value: %d' % context['request_cnt'])

    context = {'url':url, 'session_id':None, 'session_cnt':1, 'request_cnt':0}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
    proxy_debug = accelize_drm.pytest_proxy_debug
    server = Process(target=fake_server.run, args=(PROXY_HOST, PROXY_PORT, proxy_debug))
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
            # Start session #1
            drm_manager.activate()
            start = datetime.now()
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            sleep(2)
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            async_cb.assert_NoError()
        try:
            # Start session #2
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
            assert "Reached an unexpected part of code" in async_cb.message
            assert search(r"Session ID mismatch: received '[^']+' from WS but expect", async_cb.message)
            assert "Reached an unexpected part of code" in async_cb.message
            assert async_cb.errcode == accelize_drm.exceptions.DRMAssert.error_code
    finally:
        server.terminate()
        server.join()

@pytest.mark.skip
def test_segment_id_error(accelize_drm, conf_json, cred_json, async_handler, fake_server):
    """
    Test an error is returned if a wrong sement id is provided
    """
    driver = accelize_drm.pytest_fpga_driver[0]
    async_cb = async_handler.create()
    async_cb.reset()

    activators = accelize_drm.pytest_fpga_activators[0]
    activators.reset_coin()
    activators.autotest()

    conf_json.reset()
    url = conf_json['licensing']['url']
    proxy_url = f"http://{PROXY_HOST}:{PROXY_PORT}"
    conf_json['licensing']['url'] = proxy_url
    conf_json.save()

    def proxy(context, path=''):
        if path == 'o/token/':
            return redirect(f'{context["url"]}/{path}', code=307)
        else:
            context['cnt'] += 1
            print("\t********* context['cnt']=", context['cnt'])
            data = request.get_json()
            print('request=', data['request'], data['meteringFile'])
            response = requests.post(f'{context["url"]}/{path}', json=request.get_json(), headers=request.headers)
            if context['cnt'] == 2:
                context['replay'] = response
                print('save=', response.json())
            elif context['cnt'] == 3:
                # Replay the 2nd response instead of the 3rd one
                response = context['replay']
                print('replay=', response.json())
            else:
                print('response=', response.json())
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(response.content, response.status_code, headers)

    context = {'url': url, 'cnt': 0}
    fake_server.add_endpoint('/<path:path>', 'proxy', lambda path: proxy(context, path), methods=['GET', 'POST'])
    proxy_debug = accelize_drm.pytest_proxy_debug
    server = Process(target=fake_server.run, args=(PROXY_HOST, PROXY_PORT, proxy_debug))
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
            wait_period = start + timedelta(seconds=2*lic_duration-2) - datetime.now()
            sleep(wait_period.total_seconds())
            assert drm_manager.get('license_status')
            activators.autotest(is_activated=True)
            wait_period = start + timedelta(seconds=4*lic_duration+2) - datetime.now()
            sleep(wait_period.total_seconds())
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
        finally:
            drm_manager.deactivate()
            assert not drm_manager.get('license_status')
            activators.autotest(is_activated=False)
            assert async_cb.was_called
            assert async_cb.message is not None
            assert "Reached an unexpected part of code" in async_cb.message
            assert search(r"Session ID mismatch: received '[^']+' from WS but expect", async_cb.message)
            assert "Reached an unexpected part of code" in async_cb.message
            assert async_cb.errcode == accelize_drm.exceptions.DRMAssert.error_code
    finally:
        server.terminate()
        server.join()
