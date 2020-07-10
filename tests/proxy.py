from json import dumps
from flask import Flask, request, session, redirect, Response, jsonify, url_for
from requests import get, post
from datetime import datetime
from threading import Lock
from re import search
from time import sleep

context = None
lock = Lock()

def create_app(url):
    app = Flask(__name__)
    app.secret_key = 'You Will Never Guess'
    #environ['WERKZEUG_RUN_MAIN'] = 'true'
    #environ['FLASK_ENV'] = 'development'
    url = url.rstrip('/')

    @app.route('/get/', methods=['GET'])
    def get():
        global lock
        with lock:
            return jsonify(context)

    @app.route('/set/', methods=['POST'])
    def set():
        global context
        global lock
        with lock:
            context = request.get_json()
        return 'OK'

    # Functions calling the real web services
    @app.route('/o/token/', methods=['GET', 'POST'])
    def otoken():
        new_url = url + '/o/token/'
        return redirect(new_url, code=307)

    @app.route('/auth/metering/health/', methods=['GET', 'POST'])
    def health():
        request_json = request.get_json()
        new_url = url + '/auth/metering/health/'
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense():
        request_json = request.get_json()
        new_url = url + '/auth/metering/genlicense/'
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
        global context
        global lock
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

    @app.route('/test_authentication_bad_token/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_authentication_bad_token():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_authentication_bad_token/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_authentication_bad_token_genlicense():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    # test_authentication_token_renewal
    @app.route('/test_authentication_token_renewal/o/token/', methods=['GET', 'POST'])
    def otoken__test_authentication_token_renewal():
        global context
        global lock
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

    @app.route('/test_authentication_token_renewal/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_authentication_token_renewal():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_authentication_token_renewal/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_authentication_token_renewal_genlicense():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    ##############################################################################
    # test_drm_license_error.py

    # test_header_error_on_key functions
    @app.route('/test_header_error_on_key/o/token/', methods=['GET', 'POST'])
    def otoken__test_header_error_on_key():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_header_error_on_key/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_header_error_on_key():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_header_error_on_key/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_header_error_on_key_genlicense():
        global context
        global lock
        with lock:
            context['cnt'] += 1
            new_url = request.url.replace(request.url_root+'test_header_error_on_key', url)
            request_json = request.get_json()
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            response_json = response.json()
            if context['cnt'] == 1:
                dna, lic_json = list(response_json['license'].items())[0]
                key = lic_json['key']
                key = '1' + key[1:] if key[0] == '0' else '0' + key[1:]
                response_json['license'][dna]['key'] = key
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    # test_header_error_on_licenseTimer functions
    @app.route('/test_header_error_on_licenseTimer/o/token/', methods=['GET', 'POST'])
    def otoken__test_header_error_on_licenseTimer():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_header_error_on_licenseTimer/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_header_error_on_licenseTimer():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_header_error_on_licenseTimer/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_header_error_on_licenseTimer():
        global context
        global lock
        with lock:
            context['cnt'] += 1
            new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer', url)
            request_json = request.get_json()
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            response_json = response.json()
            if context['cnt'] == 2:
                dna, lic_json = list(response_json['license'].items())[0]
                timer = lic_json['licenseTimer']
                timer = '1' + timer[1:] if timer[0] == '0' else '0' + timer[1:]
                response_json['license'][dna]['licenseTimer'] = timer
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            return Response(dumps(response_json), response.status_code, headers)

    # test_session_id_error functions
    @app.route('/test_session_id_error/o/token/', methods=['GET', 'POST'])
    def otoken__test_session_id_error():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_session_id_error/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_session_id_error():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_session_id_error/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_session_id_error():
        global context
        global lock
        with lock:
            new_url = request.url.replace(request.url_root+'test_session_id_error', url)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            request_json = request.get_json()
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
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

    ##############################################################################
    # test_async_health.py

    # test_health_period_disabled functions
    @app.route('/test_health_period_disabled/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_period_disabled():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_period_disabled/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_period_disabled():
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 300
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_period_disabled/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period_disabled():
        global context
        global lock
        with lock:
            new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
            request_json = request.get_json()
            context['cnt'] += 1
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            if context['cnt'] <= context['nb_health']:
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
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 300
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_period_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period_modification():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            new_url = request.url.replace(request.url_root+'test_health_period_modification', url)
            request_json = request.get_json()
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
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
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 300
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_disabled/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_disabled():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            new_url = request.url.replace(request.url_root+'test_health_retry_disabled', url)
            request_json = request.get_json()
            health_id = request_json['health_id']
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
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 300
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_modification():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            new_url = request.url.replace(request.url_root+'test_health_retry_modification', url)
            request_json = request.get_json()
            health_id = request_json['health_id']
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
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 300
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_retry_sleep_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_sleep_modification():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            new_url = request.url.replace(request.url_root+'test_health_retry_sleep_modification', url)
            request_json = request.get_json()
            health_id = request_json['health_id']
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
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
        assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                indent=4, sort_keys=True), response.status_code, response.text)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = 300
        return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_health_metering_data/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_metering_data():
        global context
        global lock
        with lock:
            new_url = request.url.replace(request.url_root+'test_health_metering_data', url)
            request_json = request.get_json()
            health_id = request_json['health_id']
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
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
        global context
        global lock
        with lock:
            new_url = request.url.replace(request.url_root+'test_segment_index', url)
            request_json = request.get_json()
            context['genlic_cnt'] += 1
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            response_json['metering']['timeoutSecond'] = context['timeoutSecond']
            return Response(dumps(response_json), response.status_code, headers)

    @app.route('/test_segment_index/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_segment_index():
        global context
        global lock
        with lock:
            new_url = request.url.replace(request.url_root+'test_segment_index', url)
            request_json = request.get_json()
            context['health_cnt'] += 1
            response = post(new_url, json=request_json, headers=request.headers)
            assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                    indent=4, sort_keys=True), response.status_code, response.text)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            response_json['metering']['healthPeriod'] = context['healthPeriod']
            response_json['metering']['healthRetry'] = context['healthRetry']
            return Response(dumps(response_json), response.status_code, headers)

    ##############################################################################
    # test_retry_mechanism.py

    # test_long_to_short_retry_switch functions
    @app.route('/test_long_to_short_retry_switch/o/token/', methods=['GET', 'POST'])
    def otoken__test_long_to_short_retry_switch():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_long_to_short_retry_switch/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_long_to_short_retry_switch():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_long_to_short_retry_switch/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_long_to_short_retry_switch():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            new_url = request.url.replace(request.url_root+'test_long_to_short_retry_switch', url)
            request_json = request.get_json()
            request_type = request_json['request']
            if context['cnt'] < 2 or request_json['request'] != 'running':
                response = post(new_url, json=request_json, headers=request.headers)
                assert response.status_code == 200, "Request:\n'%s'\nfailed with code %d and message: %s" % (dumps(request_json,
                        indent=4, sort_keys=True), response.status_code, response.text)
                excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                response_json = response.json()
                if context['cnt'] == 0:
                    timeoutSecond = context['timeoutSecondFirst2']
                else:
                    timeoutSecond = context['timeoutSecond']
                response_json['metering']['timeoutSecond'] = timeoutSecond
                context['post'] = (response_json, headers)
                response_status_code = response.status_code
            else:
                response_json, headers = context['post']
                response_status_code = 408
                context['data'].append( (request_type,start,str(datetime.now())) )
            context['cnt'] += 1
            return Response(dumps(response_json), response_status_code, headers)

    # test_retry_on_no_connection functions
    @app.route('/test_retry_on_no_connection/o/token/', methods=['GET', 'POST'])
    def otoken__test_retry_on_no_connection():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_retry_on_no_connection/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_retry_on_no_connection():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_retry_on_no_connection/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_retry_on_no_connection():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            request_json = request.get_json()
            request_type = request_json['request']
            if len(context['data']) < 2 or request_type == 'close':
                new_url = request.url.replace(request.url_root+'test_retry_on_no_connection', url)
                response = post(new_url, json=request_json, headers=request.headers)
                excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
                headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
                response_json = response.json()
                response_json['metering']['timeoutSecond'] = context['timeoutSecond']
                ret = Response(dumps(response_json), response.status_code, headers)
            else:
                sleep(context['timeoutSecond'] + 1)
                ret = ('', 204)
            context['data'].append( (request_type, start, str(datetime.now())) )
            return ret

    ##############################################################################
    # test_unittest_on_hw.py

    # test_http_header_api_version functions
    @app.route('/test_http_header_api_version/o/token/', methods=['GET', 'POST'])
    def otoken__test_http_header_api_version():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_http_header_api_version/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_http_header_api_version():
        return redirect(request.url_root + '/auth/metering/health/', code=307)

    @app.route('/test_http_header_api_version/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_http_header_api_version():
        global context
        global lock
        with lock:
            start = str(datetime.now())
            new_url = request.url.replace(request.url_root+'test_http_header_api_version', url)
            request_json = request.get_json()
            assert search(r'Accept:.*application/vnd\.accelize\.v1\+json', str(request.headers))
            response = post(new_url, json=request_json, headers=request.headers)
            excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
            headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
            response_json = response.json()
            return Response(dumps(response_json), response.status_code, headers)

    return app


def get_context():
    r = get(url_for('get', _external=True))
    assert r.status_code == 200
    return r.json()


def set_context(data):
    r = post(url_for('set', _external=True), json=data)
    assert r.status_code == 200

