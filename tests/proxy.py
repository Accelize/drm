import os
from json import dumps
from flask import Flask, request, session, redirect, Response, jsonify, url_for
from requests import get, post, patch
from datetime import datetime
from threading import Lock
from re import search
from time import sleep
from copy import deepcopy

context = None
lock = Lock()

def create_app(url):
    app = Flask(__name__)
    app.secret_key = 'You Will Never Guess'
    #os.environ['WERKZEUG_RUN_MAIN'] = 'true'
    os.environ['FLASK_ENV'] = 'development'
    url = url.rstrip('/')
    '''
    @app.errorhandler(Exception)
    def all_exception_handler(error):
        global context, lock
        with lock:
            if context:
                context['exception'] = str(error)
        return 'Proxy caught exception: %s' % str(error), 500
    '''
    @app.route('/get/', methods=['GET'])
    def get():
        global lock
        with lock:
            return jsonify(context)

    @app.route('/set/', methods=['POST'])
    def set():
        global context, lock
        with lock:
            context = request.get_json()
        return 'OK'
    '''
    # Functions calling the real web services
    @app.route('/auth/token', methods=['GET', 'POST'])
    def otoken():
        new_url = request.url.replace(request.url_root, url)
        return redirect(new_url, code=307)

    @app.route('/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create(product_id):
        request_json = request.get_json()
        new_url = request.url.replace(request.url_root, url)
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update(entitlement_id):
        request_json = request.get_json()
        new_url = request.url.replace(request.url_root, url)
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)
    '''
    ##############################################################################
    # test_authentication.py

    # test_authentication_bad_token
    @app.route('/test_authentication_bad_token/auth/token', methods=['GET', 'POST'])
    def gettoken__test_authentication_bad_token():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_authentication_bad_token', url)
        response = post(new_url, data=request.form, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request.form,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['access_token'] = context['access_token']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_authentication_bad_token/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_authentication_bad_token(product_id):
        new_url = request.url.replace(request.url_root+'test_authentication_bad_token', url)
        return redirect(new_url, code=307)

    # test_authentication_token_renewal
    @app.route('/test_authentication_token_renewal/auth/token', methods=['GET', 'POST'])
    def gettoken__test_authentication_token_renewal():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_authentication_token_renewal', url)
        response = post(new_url, data=request.form, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request.form,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['expires_in'] = context['expires_in']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_authentication_token_renewal/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_authentication_token_renewal(product_id):
        new_url = request.url.replace(request.url_root+'test_authentication_token_renewal', url)
        return redirect(new_url, code=307)

    @app.route('/test_authentication_token_renewal/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_authentication_token_renewal(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_authentication_token_renewal', url)
        return redirect(new_url, code=307)

    ##############################################################################
    # test_drm_license_error.py

    # test_header_error_on_key functions
    @app.route('/test_header_error_on_key/auth/token', methods=['GET', 'POST'])
    def gettoken__test_header_error_on_key():
        new_url = request.url.replace(request.url_root+'test_header_error_on_key', url)
        return redirect(new_url, code=307)

    @app.route('/test_header_error_on_key/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_header_error_on_key(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_header_error_on_key', url)
        request_json = request.get_json()
        with lock:
            if context['cnt'] > 0:
                return ({'error':'Test did not run as expected'}, 408)
            context['cnt'] += 1
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        dna, lic_json = list(response_json['license'].items())[0]
        key = lic_json['key']
        key = '1' + key[1:] if key[0] == '0' else '0' + key[1:]
        response_json['license'][dna]['key'] = key
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_header_error_on_key/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_header_error_on_key(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_header_error_on_key', url)
        return redirect(new_url, code=307)

    # test_header_error_on_key2 functions
    @app.route('/test_header_error_on_key2/auth/token', methods=['GET', 'POST'])
    def gettoken__test_header_error_on_key2():
        new_url = request.url.replace(request.url_root+'test_header_error_on_key2', url)
        return redirect(new_url, code=307)

    @app.route('/test_header_error_on_key2/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_header_error_on_key2(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_header_error_on_key2', url)
        request_json = request.get_json()
        with lock:
            if context['cnt'] > 0:
                return ({'error':'Test did not run as expected'}, 408)
            context['cnt'] += 1
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        dna, lic_json = list(response_json['license'].items())[0]
        key = lic_json['key']
        key = key[0:-1] + '1' if key[-1] == '0' else key[0:-1] + '0'
        response_json['license'][dna]['key'] = key
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_header_error_on_key2/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_header_error_on_key2(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_header_error_on_key2', url)
        return redirect(new_url, code=307)

    # test_header_error_on_licenseTimer functions
    @app.route('/test_header_error_on_licenseTimer/auth/token', methods=['GET', 'POST'])
    def gettoken__test_header_error_on_licenseTimer():
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer', url)
        return redirect(new_url, code=307)

    @app.route('/test_header_error_on_licenseTimer/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_header_error_on_licenseTimer(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer', url)
        request_json = request.get_json()
        request_type = request_json['request']
        with lock:
            cnt = context['cnt']
            if request_type != 'close' and cnt > 1:
                return ({'error':'Test did not run as expected'}, 408)
            context['cnt'] += 1
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        if cnt == 1 and response_json.get('license'):
            dna, lic_json = list(response_json['license'].items())[0]
            timer = lic_json['licenseTimer']
            timer = '1' + timer[1:] if timer[0] == '0' else '0' + timer[1:]
            response_json['license'][dna]['licenseTimer'] = timer
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_header_error_on_licenseTimer/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_header_error_on_licenseTimer(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer', url)
        return redirect(new_url, code=307)

    # test_header_error_on_licenseTimer2 functions
    @app.route('/test_header_error_on_licenseTimer2/auth/token', methods=['GET', 'POST'])
    def gettoken__test_header_error_on_licenseTimer2():
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer2', url)
        return redirect(new_url, code=307)

    @app.route('/test_header_error_on_licenseTimer2/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_header_error_on_licenseTimer2(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer2', url)
        request_json = request.get_json()
        request_type = request_json['request']
        with lock:
            cnt = context['cnt']
            if request_type != 'close' and cnt > 1:
                return ({'error':'Test did not run as expected'}, 408)
            context['cnt'] += 1
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        if cnt == 1 and response_json.get('license'):
            dna, lic_json = list(response_json['license'].items())[0]
            timer = lic_json['licenseTimer']
            timer = timer[0:-1] + '1' if timer[-1] == '0' else timer[0:-1] + '0'
            response_json['license'][dna]['licenseTimer'] = timer
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_header_error_on_licenseTimer2/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_header_error_on_licenseTimer2(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer2', url)
        return redirect(new_url, code=307)

    # test_replay_request functions
    @app.route('/test_replay_request/auth/token', methods=['GET', 'POST'])
    def gettoken__test_replay_request():
        new_url = request.url.replace(request.url_root+'test_replay_request', url)
        return redirect(new_url, code=307)

    @app.route('/test_replay_request/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_replay_request(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_replay_request', url)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        response_session_id = response_json['metering']['sessionId']
        with lock:
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

    @app.route('/test_replay_request/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_replay_request(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_replay_request', url)
        return redirect(new_url, code=307)

    ##############################################################################
    # test_async_health.py

    # test_health_period_disabled functions
    @app.route('/test_health_period_disabled/auth/token', methods=['GET', 'POST'])
    def gettoken__test_health_period_disabled():
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        return redirect(new_url, code=307)

    @app.route('/test_health_period_disabled/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_health_period_disabled(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['drm_config']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_period_disabled/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_health_period_disabled(entitlement_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            context['cnt'] += 1
            if context['cnt'] < context['nb_health']:
                response_json['metering']['health_period'] = context['health_period']
            else:
                response_json['metering']['health_period'] = 0
                context['exit'] = True
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_period_modification functions
    @app.route('/test_health_period_modification/auth/token', methods=['GET', 'POST'])
    def gettoken__test_health_period_modification():
        new_url = request.url.replace(request.url_root+'test_health_period_modification', url)
        return redirect(new_url, code=307)

    @app.route('/test_health_period_modification/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_health_period_modification(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_period_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_period_modification/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_health_period_modification(entitlement_id):
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_health_period_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
            response_json['metering']['health_retry'] = context['health_retry']
            response_json['metering']['health_retry_sleep'] = context['health_retry_sleep']
            context['health_period'] += 1
            context['data'].append( (start,str(datetime.now())) )
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry_disabled functions
    @app.route('/test_health_retry_disabled/auth/token', methods=['GET', 'POST'])
    def gettoken__test_health_retry_disabled():
        new_url = request.url.replace(request.url_root+'test_health_retry_disabled', url)
        return redirect(new_url, code=307)

    @app.route('/test_health_retry_disabled/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_health_retry_disabled(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_retry_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_disabled/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_health_retry_disabled(entitlement_id):
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_health_retry_disabled', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        with lock:
            if len(context['data']) <= 1:
                response = post(new_url, json=request_json, headers=request.headers)
                assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                        indent=4, sort_keys=True), response.status_code, response.text)
                excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                response_json = response.json()
                response_json['metering']['health_period'] = context['health_period']
                response_json['metering']['health_retry'] = 0
                response_json['metering']['health_retry_sleep'] = context['health_retry_sleep']
                response_status_code = response.status_code
                context['post'] = (response_json, headers)
            else:
                response_json, headers = context['post']
                response_status_code = 408
            context['data'].append( (health_id,start,str(datetime.now())) )
        return Response(dumps(response_json), response_status_code, headers)

    # test_health_retry_modification functions
    @app.route('/test_health_retry_modification/auth/token', methods=['GET', 'POST'])
    def gettoken__test_health_retry_modification():
        new_url = request.url.replace(request.url_root+'test_health_retry_modification', url)
        return redirect(new_url, code=307)

    @app.route('/test_health_retry_modification/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_health_retry_modification(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_retry_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_modification/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_health_retry_modification(entitlement_id):
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_health_retry_modification', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        with lock:
            if len(context['data']) < 1:
                response = post(new_url, json=request_json, headers=request.headers)
                assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                        indent=4, sort_keys=True), response.status_code, response.text)
                excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                response_json = response.json()
                response_json['metering']['health_period'] = context['health_period']
                response_json['metering']['health_retry'] = context['health_retry']
                response_json['metering']['health_retry_sleep'] = context['health_retry_sleep']
                response_status_code = response.status_code
                context['post'] = (response_json, headers)
            else:
                response_json, headers = context['post']
                response_status_code = 408
            if health_id <= 1:
                context['data'].append( (health_id,start,str(datetime.now())) )
            else:
                context['exit'] = True
        return Response(dumps(response_json), response_status_code, headers)

    # test_health_retry_sleep_modification functions
    @app.route('/test_health_retry_sleep_modification/auth/token', methods=['GET', 'POST'])
    def gettoken__test_health_retry_sleep_modification():
        new_url = request.url.replace(request.url_root+'test_health_retry_sleep_modification', url)
        return redirect(new_url, code=307)

    @app.route('/test_health_retry_sleep_modification/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_health_retry_sleep_modification(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_retry_sleep_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_sleep_modification/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_health_retry_sleep_modification(entitlement_id):
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_health_retry_sleep_modification', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        with lock:
            if len(context['data']) < 1:
                response = post(new_url, json=request_json, headers=request.headers)
                assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                        indent=4, sort_keys=True), response.status_code, response.text)
                excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                response_json = response.json()
                response_json['metering']['health_period'] = context['health_period']
                response_json['metering']['health_retry'] = context['health_retry']
                response_json['metering']['health_retry_sleep'] = context['health_retry_sleep']
                response_status_code = response.status_code
                context['post'] = (response_json, headers)
            else:
                response_json, headers = context['post']
                response_status_code = 408
            if health_id <= 1:
                context['data'].append( (health_id,start,str(datetime.now())) )
            else:
                context['exit'] = True
        return Response(dumps(response_json), response_status_code, headers)

    # test_health_metering_data functions
    @app.route('/test_health_metering_data/auth/token', methods=['GET', 'POST'])
    def gettoken__test_health_metering_data():
        new_url = request.url.replace(request.url_root+'test_health_metering_data', url)
        return redirect(new_url, code=307)

    @app.route('/test_health_metering_data/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_health_metering_data(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_metering_data', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_metering_data/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_health_metering_data(entitlement_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_metering_data', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
            response_json['metering']['health_retry'] = context['health_retry']
            context['health_id']= health_id
        return Response(dumps(response_json), response.status_code, headers)

    # test_segment_index functions
    @app.route('/test_segment_index/auth/token', methods=['GET', 'POST'])
    def gettoken__test_segment_index():
        new_url = request.url.replace(request.url_root+'test_segment_index', url)
        return redirect(new_url, code=307)

    @app.route('/test_segment_index/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_segment_index(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_segment_index', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
            response_json['metering']['health_retry'] = context['health_retry']
            context['nb_genlic'] += 1
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_segment_index/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_segment_index(entitlement_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_segment_index', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
            response_json['metering']['health_retry'] = context['health_retry']
        return Response(dumps(response_json), response.status_code, headers)

    # test_async_call_on_pause_when_health_is_enabled and test_no_async_call_on_pause_when_health_is_disabled functions
    @app.route('/test_async_call_on_pause_depending_on_health_status/auth/token', methods=['GET', 'POST'])
    def gettoken__test_async_call_on_pause_depending_on_health_status():
        new_url = request.url.replace(request.url_root+'test_async_call_on_pause_depending_on_health_status', url)
        return redirect(new_url, code=307)

    @app.route('/test_async_call_on_pause_depending_on_health_status/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_async_call_on_pause_depending_on_health_status(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_async_call_on_pause_depending_on_health_status', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
            response_json['metering']['health_retry'] = context['health_retry']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_async_call_on_pause_depending_on_health_status/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_async_call_on_pause_depending_on_health_status(entitlement_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_async_call_on_pause_depending_on_health_status', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            context['health_cnt'] += 1
            response_json['metering']['health_period'] = context['health_period']
            response_json['metering']['health_retry'] = context['health_retry']
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_retry_mechanism.py

    # test_api_retry_disabled and test_api_retry_enabled functions
    @app.route('/test_api_retry/auth/token', methods=['GET', 'POST'])
    def gettoken__test_api_retry():
        new_url = request.url.replace(request.url_root+'test_api_retry', url)
        return redirect(new_url, code=307)

    @app.route('/test_api_retry/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_api_retry(product_id):
        return ({'error':'Force retry for testing'}, 408)

    @app.route('/test_api_retry/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_api_retry(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_api_retry', url)
        return redirect(new_url, code=307)

    # test_long_to_short_retry_switch_on_authentication functions
    @app.route('/test_long_to_short_retry_switch_on_authentication/auth/token', methods=['GET', 'POST'])
    def gettoken__test_long_to_short_retry_switch_on_authentication():
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_authentication', url)
        with lock:
            try:
                if context['cnt'] == 0 or context['exit']:
                    response = post(new_url, data=request.form, headers=request.headers)
                    assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request.form,
                            indent=4, sort_keys=True), response.status_code, response.text)
                    excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                    headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                    response_json = response.json()
                    response_json['expires_in'] = context['expires_in']
                    context['response_json'] = dumps(response_json)
                    context['headers'] = headers
                    return Response(dumps(response_json), response.status_code, headers)
                else:
                    return Response(context['response_json'], 408, context['headers'])
            finally:
                context['data'].append( (start,str(datetime.now())) )
                context['cnt'] += 1

    @app.route('/test_long_to_short_retry_switch_on_authentication/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_long_to_short_retry_switch_on_authentication(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_authentication', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['timeoutSecond'] = context['timeoutSecond']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_long_to_short_retry_switch_on_authentication/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_long_to_short_retry_switch_on_authentication(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_authentication', url)
        return redirect(new_url, code=307)

    # test_long_to_short_retry_switch_on_license functions
    @app.route('/test_long_to_short_retry_switch_on_license/auth/token', methods=['GET', 'POST'])
    def gettoken__test_long_to_short_retry_switch_on_license():
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_license', url)
        return redirect(new_url, code=307)

    @app.route('/test_long_to_short_retry_switch_on_license/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_long_to_short_retry_switch_on_license(product_id):
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_license', url)
        request_json = request.get_json()
        request_type = request_json['request']
        with lock:
            try:
                if context['cnt'] < 1 or request_type == 'close':
                    response = post(new_url, json=request_json, headers=request.headers)
                    assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                            indent=4, sort_keys=True), response.status_code, response.text)
                    excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                    headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                    response_json = response.json()
                    response_json['metering']['timeoutSecond'] = context['timeoutSecond']
                    return Response(dumps(response_json), response.status_code, headers)
                else:
                    return ({'error':'Test retry mechanism'}, 408)
            finally:
                context['cnt'] += 1
                context['data'].append( (request_type, start, str(datetime.now())) )

    @app.route('/test_long_to_short_retry_switch_on_license/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_long_to_short_retry_switch_on_license(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_license', url)
        return redirect(new_url, code=307)

    # test_api_retry_on_lost_connection functions
    @app.route('/test_api_retry_on_lost_connection/auth/token', methods=['GET', 'POST'])
    def gettoken__test_api_retry_on_lost_connection():
        new_url = request.url.replace(request.url_root+'test_api_retry_on_lost_connection', url)
        return redirect(new_url, code=307)

    @app.route('/test_api_retry_on_lost_connection/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_api_retry_on_lost_connection(product_id):
        global context, lock
        start = str(datetime.now())
        with lock:
            sleep_s = context['sleep']
            context['data'].append(start)
        sleep( sleep_s)
        return ('', 204)

    @app.route('/test_api_retry_on_lost_connection/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_api_retry_on_lost_connection(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_api_retry_on_lost_connection', url)
        return redirect(new_url, code=307)

    # test_thread_retry_on_lost_connection functions
    @app.route('/test_thread_retry_on_lost_connection/auth/token', methods=['GET', 'POST'])
    def gettoken__test_thread_retry_on_lost_connection():
        new_url = request.url.replace(request.url_root+'test_thread_retry_on_lost_connection', url)
        return redirect(new_url, code=307)

    @app.route('/test_thread_retry_on_lost_connection/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_thread_retry_on_lost_connection(product_id):
        global context, lock
        request_json = request.get_json()
        request_type = request_json['request']
        with lock:
            cnt = context['cnt']
            timeoutSecond = context['timeoutSecond'] + 1
            context['cnt'] += 1
        if cnt < 1 or request_type == 'close':
            new_url = request.url.replace(request.url_root+'test_thread_retry_on_lost_connection', url)
            response = post(new_url, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            response_json['metering']['timeoutSecond'] = timeoutSecond
            return Response(dumps(response_json), response.status_code, headers)
        else:
            sleep(timeoutSecond)
            return ('', 204)

    @app.route('/test_thread_retry_on_lost_connection/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_thread_retry_on_lost_connection(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_thread_retry_on_lost_connection', url)
        return redirect(new_url, code=307)

    ##############################################################################
    # test_unittest_on_hw.py

    # test_http_header_api_version functions
    @app.route('/test_http_header_api_version/auth/token', methods=['GET', 'POST'])
    def gettoken__test_http_header_api_version():
        new_url = request.url.replace(request.url_root+'test_http_header_api_version', url)
        return redirect(new_url, code=307)

    @app.route('/test_http_header_api_version/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_http_header_api_version(product_id):
        new_url = request.url.replace(request.url_root+'test_http_header_api_version', url)
        request_json = request.get_json()
        assert search(r'Accept:.*application/vnd\.accelize\.v1\+json', str(request.headers))
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_http_header_api_version/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_http_header_api_version(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_http_header_api_version', url)
        return redirect(new_url, code=307)

    ##############################################################################
    # test_lgdn_topics.py

    # test_topic0_corrupted_segment_index functions
    @app.route('/test_topic0_corrupted_segment_index/auth/token', methods=['GET', 'POST'])
    def gettoken__test_topic0_corrupted_segment_index():
        new_url = request.url.replace(request.url_root+'test_topic0_corrupted_segment_index', url)
        return redirect(new_url, code=307)

    @app.route('/test_topic0_corrupted_segment_index/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_topic0_corrupted_segment_index(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_topic0_corrupted_segment_index', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        try:
            assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        except AssertionError as e:
            if 'Metering information is not consistent' in str(e):
                with lock:
                    context['error'] += 1
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        if 'metering' in response_json.keys():
            with lock:
                response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_topic0_corrupted_segment_index/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_topic0_corrupted_segment_index(entitlement_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_topic0_corrupted_segment_index', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        try:
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        except AssertionError as e:
            if 'Metering information is not consistent' in str(e):
                with lock:
                    context['error'] += 1
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        if 'metering' in response_json.keys():
            with lock:
                response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    # test_topic1_corrupted_metering functions
    @app.route('/test_topic1_corrupted_metering/auth/token', methods=['GET', 'POST'])
    def gettoken__test_topic1_corrupted_metering():
        new_url = request.url.replace(request.url_root+'test_topic1_corrupted_metering', url)
        return redirect(new_url, code=307)

    @app.route('/test_topic1_corrupted_metering/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_topic1_corrupted_metering(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_topic1_corrupted_metering', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_topic1_corrupted_metering/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_topic1_corrupted_metering(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_topic1_corrupted_metering', url)
        return redirect(new_url, code=307)

    ##############################################################################
    # test_improve_coverage.py

    # test_improve_coverage_ws_client functions
    @app.route('/test_improve_coverage_ws_client/auth/token', methods=['GET', 'POST'])
    def gettoken__test_improve_coverage_ws_client():
        new_url = request.url.replace(request.url_root+'gettoken__test_improve_coverage_ws_client', url)
        return redirect(new_url, code=307)

    @app.route('/test_improve_coverage_ws_client/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_improve_coverage_ws_client(product_id):
        global context, lock
        with lock:
            return ({'error':'Generate error on purpose'}, context['error_code'])

    @app.route('/test_improve_coverage_ws_client/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_improve_coverage_ws_client(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_improve_coverage_ws_client', url)
        return redirect(new_url, code=307)

    # test_improve_coverage_setLicense functions
    @app.route('/test_improve_coverage_setLicense/auth/token', methods=['GET', 'POST'])
    def gettoken__test_improve_coverage_setLicense():
        new_url = request.url.replace(request.url_root+'test_improve_coverage_setLicense', url)
        return redirect(new_url, code=307)

    @app.route('/test_improve_coverage_setLicense/customer/product/<product_id>/entitlement_session', methods=['POST', 'PATCH'])
    def create__test_improve_coverage_setLicense(product_id):
        new_url = request.url.replace(request.url_root+'test_improve_coverage_setLicense', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        del response_json['drm_config']['license']
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_valgrind.py

    # test_normal_usage functions
    @app.route('/test_normal_usage/auth/token', methods=['GET', 'POST'])
    def gettoken__test_normal_usage():
        new_url = request.url.replace(request.url_root+'test_normal_usage', url)
        return redirect(new_url, code=307)

    @app.route('/test_normal_usage/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_normal_usage(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_normal_usage', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['drm_config']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_normal_usage/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_normal_usage(entitlement_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_normal_usage', url)
        request_json = request.get_json()
        is_health = request_json.get('is_health')
        is_closed = request_json.get('is_closed')
        response = patch(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 204 if is_health else 200, (
                "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text))
        if is_health or is_closed:
            return Response(status = 204)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['drm_config']['health_period'] = context['health_period']
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_derived_product.py
    """
    # test_valid_derived_product function
    @app.route('/test_valid_derived_product/auth/token', methods=['GET', 'POST'])
    def gettoken__test_valid_derived_product():
        new_url = request.url.replace(request.url_root+'test_valid_derived_product', url)
        return redirect(new_url, code=307)

    @app.route('/test_valid_derived_product/customer/product/<product_id>/entitlement_session', methods=['PATCH', 'POST'])
    def create__test_valid_derived_product(product_id):
        global context, lock
        new_url = request.url.replace(request.url_root+'test_valid_derived_product', url)
        request_json = request.get_json()
        deriv_prod = '{vendor}/{library}/{name}'.format(**request_json['product'])
        with lock:
            context['derived_product'] = deriv_prod
            request_json['product']['name'] = request_json['product']['name'].replace(context['product_suffix'], '')
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 201, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_valid_derived_product/customer/entitlement_session/<entitlement_id>', methods=['PATCH', 'POST'])
    def update__test_valid_derived_product(entitlement_id):
        new_url = request.url.replace(request.url_root+'test_valid_derived_product', url)
        return redirect(new_url, code=307)
    """
    ##############################################################################
    # test_ws_timeout.py

    # test_request_timeout functions
    @app.route('/test_request_timeout/auth/token', methods=['GET', 'POST'])
    def gettoken__test_request_timeout():
        global context, lock
        start = str(datetime.now())
        with lock:
            sleep_s = context['sleep']
        sleep( sleep_s)
        return ('This is the expected behavior', 408)

    ##############################################################################

    return app


def get_context():
    r = get(url_for('get', _external=True))
    assert r.status_code == 200
    return r.json()


def set_context(data):
    r = post(url_for('set', _external=True), json=data)
    assert r.status_code == 200


def get_proxy_error():
    r = get_context()
    try:
        return r['exception']
    except KeyError:
        return None

