import os
from json import dumps
from flask import Flask, request, session, redirect, Response, jsonify, url_for
from requests import get, post
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

    # Functions calling the real web services
    @app.route('/o/token/', methods=['GET', 'POST'])
    def otoken():
        new_url = url + '/o/token/'
        return redirect(new_url, code=307)

    @app.route('/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense():
        request_json = request.get_json()
        new_url = url + '/auth/metering/genlicense/'
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/auth/metering/health/', methods=['GET', 'POST'])
    def health():
        request_json = request.get_json()
        new_url = url + '/auth/metering/health/'
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_authentication.py

    # test_authentication_bad_token
    @app.route('/test_authentication_bad_token/o/token/', methods=['GET', 'POST'])
    def otoken__test_authentication_bad_token():
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

    @app.route('/test_authentication_bad_token/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_authentication_bad_token_genlicense():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_authentication_bad_token/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_authentication_bad_token():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_authentication_token_renewal
    @app.route('/test_authentication_token_renewal/o/token/', methods=['GET', 'POST'])
    def otoken__test_authentication_token_renewal():
        global context, lock
        new_url = url + '/o/token/'
        response = post(new_url, data=request.form, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request.form,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['expires_in'] = context['expires_in']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_authentication_token_renewal/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_authentication_token_renewal_genlicense():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_authentication_token_renewal/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_authentication_token_renewal():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    ##############################################################################
    # test_drm_license_error.py

    # test_header_error_on_key functions
    @app.route('/test_header_error_on_key/o/token/', methods=['GET', 'POST'])
    def otoken__test_header_error_on_key():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_header_error_on_key/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_header_error_on_key_genlicense():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_header_error_on_key', url)
        request_json = request.get_json()
        with lock:
            if context['cnt'] > 0:
                return ({'error':'Test did not run as expected'}, 408)
            context['cnt'] += 1
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        dna, lic_json = list(response_json['license'].items())[0]
        key = lic_json['key']
        key = '1' + key[1:] if key[0] == '0' else '0' + key[1:]
        response_json['license'][dna]['key'] = key
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_header_error_on_key/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_header_error_on_key():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_header_error_on_licenseTimer functions
    @app.route('/test_header_error_on_licenseTimer/o/token/', methods=['GET', 'POST'])
    def otoken__test_header_error_on_licenseTimer():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_header_error_on_licenseTimer/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_header_error_on_licenseTimer():
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
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        response_json = response.json()
        if cnt == 1:
            dna, lic_json = list(response_json['license'].items())[0]
            timer = lic_json['licenseTimer']
            timer = '1' + timer[1:] if timer[0] == '0' else '0' + timer[1:]
            response_json['license'][dna]['licenseTimer'] = timer
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_header_error_on_licenseTimer/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_header_error_on_licenseTimer():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_session_id_error functions
    @app.route('/test_session_id_error/o/token/', methods=['GET', 'POST'])
    def otoken__test_session_id_error():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_session_id_error/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_session_id_error():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_session_id_error', url)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
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

    @app.route('/test_session_id_error/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_session_id_error():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    ##############################################################################
    # test_async_health.py

    # test_health_period_disabled functions
    @app.route('/test_health_period_disabled/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_period_disabled():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_period_disabled/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_period_disabled():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_period_disabled/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period_disabled():
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
                response_json['metering']['healthPeriod'] = context['healthPeriod']
            else:
                response_json['metering']['healthPeriod'] = 0
                context['exit'] = True
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_period_modification functions
    @app.route('/test_health_period_modification/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_period_modification():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_period_modification/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_period_modification():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_period_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_period_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period_modification():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
            response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
            context['healthPeriod'] += 1
            context['data'].append( (start,str(datetime.now())) )
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry_disabled functions
    @app.route('/test_health_retry_disabled/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_disabled():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_disabled/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_disabled():
        new_url = request.url.replace(request.url_root+'test_health_retry_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_disabled/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_disabled():
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
                response_json['metering']['healthPeriod'] = context['healthPeriod']
                response_json['metering']['healthRetry'] = 0
                response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
                response_status_code = response.status_code
                context['post'] = (response_json, headers)
            else:
                response_json, headers = context['post']
                response_status_code = 408
            context['data'].append( (health_id,start,str(datetime.now())) )
        return Response(dumps(response_json), response_status_code, headers)

    # test_health_retry_modification functions
    @app.route('/test_health_retry_modification/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_modification():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_modification/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_modification():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_retry_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_modification():
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
                response_json['metering']['healthPeriod'] = context['healthPeriod']
                response_json['metering']['healthRetry'] = context['healthRetry']
                response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
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
    @app.route('/test_health_retry_sleep_modification/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_sleep_modification():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_sleep_modification/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_sleep_modification():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_retry_sleep_modification', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_sleep_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_sleep_modification():
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
                response_json['metering']['healthPeriod'] = context['healthPeriod']
                response_json['metering']['healthRetry'] = context['healthRetry']
                response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
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
    @app.route('/test_health_metering_data/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_metering_data():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_metering_data/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_metering_data():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_health_metering_data', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_metering_data/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_metering_data():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
            context['health_id']= health_id
        return Response(dumps(response_json), response.status_code, headers)

    # test_segment_index functions
    @app.route('/test_segment_index/o/token/', methods=['GET', 'POST'])
    def otoken__test_segment_index():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_segment_index/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_segment_index():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
            context['nb_genlic'] += 1
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_segment_index/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_segment_index():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
        return Response(dumps(response_json), response.status_code, headers)

    # test_async_call_on_pause_when_health_is_enabled and test_no_async_call_on_pause_when_health_is_disabled functions
    @app.route('/test_async_call_on_pause_depending_on_health_status/o/token/', methods=['GET', 'POST'])
    def otoken__test_async_call_on_pause_depending_on_health_status():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_async_call_on_pause_depending_on_health_status/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_async_call_on_pause_depending_on_health_status():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_async_call_on_pause_depending_on_health_status/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_async_call_on_pause_depending_on_health_status():
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
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_retry_mechanism.py

    # test_api_retry_disabled and test_api_retry_enabled functions
    @app.route('/test_api_retry/o/token/', methods=['GET', 'POST'])
    def otoken__test_api_retry():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_api_retry/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_api_retry():
        return ({'error':'Force retry for testing'}, 408)

    @app.route('/test_api_retry/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_api_retry():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_long_to_short_retry_switch_on_authentication functions
    @app.route('/test_long_to_short_retry_switch_on_authentication/o/token/', methods=['GET', 'POST'])
    def otoken__test_long_to_short_retry_switch_on_authentication():
        global context, lock
        start = str(datetime.now())
        new_url = url + '/o/token/'
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

    @app.route('/test_long_to_short_retry_switch_on_authentication/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_long_to_short_retry_switch_on_authentication():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_authentication', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['timeoutSecond'] = context['timeoutSecond']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_long_to_short_retry_switch_on_authentication/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_long_to_short_retry_switch_on_authentication():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_long_to_short_retry_switch_on_license functions
    @app.route('/test_long_to_short_retry_switch_on_license/o/token/', methods=['GET', 'POST'])
    def otoken__test_long_to_short_retry_switch_on_license():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_long_to_short_retry_switch_on_license/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_long_to_short_retry_switch_on_license():
        global context, lock
        start = str(datetime.now())
        new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch_on_license', url)
        request_json = request.get_json()
        request_type = request_json['request']
        with lock:
            try:
                if context['cnt'] < 1 or request_type == 'close':
                    response = post(new_url, json=request_json, headers=request.headers)
                    assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
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

    @app.route('/test_long_to_short_retry_switch_on_license/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_long_to_short_retry_switch_on_license():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_api_retry_on_lost_connection functions
    @app.route('/test_api_retry_on_lost_connection/o/token/', methods=['GET', 'POST'])
    def otoken__test_api_retry_on_lost_connection():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_api_retry_on_lost_connection/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_api_retry_on_lost_connection():
        global context, lock
        start = str(datetime.now())
        with lock:
            sleep_s = context['sleep']
            context['data'].append(start)
        sleep( sleep_s)
        return ('', 204)

    @app.route('/test_api_retry_on_lost_connection/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_api_retry_on_lost_connection():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_thread_retry_on_lost_connection functions
    @app.route('/test_thread_retry_on_lost_connection/o/token/', methods=['GET', 'POST'])
    def otoken__test_thread_retry_on_lost_connection():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_thread_retry_on_lost_connection/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_thread_retry_on_lost_connection():
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

    @app.route('/test_thread_retry_on_lost_connection/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_thread_retry_on_lost_connection():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    ##############################################################################
    # test_unittest_on_hw.py

    # test_http_header_api_version functions
    @app.route('/test_http_header_api_version/o/token/', methods=['GET', 'POST'])
    def otoken__test_http_header_api_version():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_http_header_api_version/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_http_header_api_version():
        new_url = request.url.replace(request.url_root+'test_http_header_api_version', url)
        request_json = request.get_json()
        assert search(r'Accept:.*application/vnd\.accelize\.v1\+json', str(request.headers))
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_http_header_api_version/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_http_header_api_version():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    ##############################################################################
    # test_lgdn_topics.py

    # test_topic0_corrupted_segment_index functions
    @app.route('/test_topic0_corrupted_segment_index/o/token/', methods=['GET', 'POST'])
    def otoken__test_topic0_corrupted_segment_index():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_topic0_corrupted_segment_index/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_topic0_corrupted_segment_index():
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
                response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_topic0_corrupted_segment_index/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_topic0_corrupted_segment_index():
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
                response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    # test_topic1_corrupted_metering functions
    @app.route('/test_topic1_corrupted_metering/o/token/', methods=['GET', 'POST'])
    def otoken__test_topic1_corrupted_metering():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_topic1_corrupted_metering/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_topic1_corrupted_metering():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_topic1_corrupted_metering', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_topic1_corrupted_metering/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_topic1_corrupted_metering():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    ##############################################################################
    # test_improve_coverage.py

    # test_improve_coverage_ws_client functions
    @app.route('/test_improve_coverage_ws_client/o/token/', methods=['GET', 'POST'])
    def otoken__test_improve_coverage_ws_client():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_improve_coverage_ws_client/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_improve_coverage_ws_client():
        global context, lock
        with lock:
            return ({'error':'Generate error on purpose'}, context['error_code'])

    @app.route('/test_improve_coverage_ws_client/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_improve_coverage_ws_client():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    # test_improve_coverage_setLicense functions
    @app.route('/test_improve_coverage_setLicense/o/token/', methods=['GET', 'POST'])
    def otoken__test_improve_coverage_setLicense():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_improve_coverage_setLicense/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_improve_coverage_setLicense():
        new_url = request.url.replace(request.url_root+'test_improve_coverage_setLicense', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 0
        if request_json['request'] == 'running':
            response_json['metering'] = 'test'
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_valgrind.py

    # test_normal_usage functions
    @app.route('/test_normal_usage/o/token/', methods=['GET', 'POST'])
    def otoken__test_normal_usage():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_normal_usage/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_normal_usage():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_normal_usage', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_normal_usage/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_normal_usage():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_normal_usage', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        with lock:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_derived_product.py

    # test_valid_derived_product function
    @app.route('/test_valid_derived_product/o/token/', methods=['GET', 'POST'])
    def otoken__test_valid_derived_product():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_valid_derived_product/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_valid_derived_product():
        global context, lock
        new_url = request.url.replace(request.url_root+'test_valid_derived_product', url)
        request_json = request.get_json()
        deriv_prod = '{vendor}/{library}/{name}'.format(**request_json['product'])
        with lock:
            context['derived_product'] = deriv_prod
            request_json['product']['name'] = request_json['product']['name'].replace(context['product_suffix'], '')
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
            indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_valid_derived_product/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_valid_derived_product():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    ##############################################################################
    # test_ws_timeout.py

    # test_request_timeout functions
    @app.route('/test_request_timeout/o/token/', methods=['GET', 'POST'])
    def otoken__test_request_timeout():
        global context, lock
        start = str(datetime.now())
        with lock:
            sleep_s = context['sleep']
        sleep( sleep_s)
        return ('This is the expected behavior', 408)
        return redirect(request.url_root + '/o/token/', code=307)

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

