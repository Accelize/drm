from json import dumps
from flask import Flask, request, session, redirect, Response, jsonify
from requests import get, post


context = None

def create_app(url):
    global context
    app = Flask(__name__)
    app.secret_key = 'You Will Never Guess'
    #environ['WERKZEUG_RUN_MAIN'] = 'true'
    #environ['FLASK_ENV'] = 'development'
    url = url.rstrip('/')

    @app.route('/get/', methods=['GET'])
    def get():
        return jsonify(context)

    @app.route('/set/', methods=['POST'])
    def set():
        global context
        context = request.get_json()
        return 'OK'

    @app.route('/o/token/', methods=['GET', 'POST'])
    def otoken():
        new_url = url + '/o/token/'
        return redirect(new_url, code=307)

    @app.route('/auth/metering/health/', methods=['GET', 'POST'])
    def health():
        new_url = url + '/auth/metering/health/'
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
        context['cnt'] += 1
        new_url = request.url.replace(request.url_root+'test_header_error_on_key', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
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
        context['cnt'] += 1
        new_url = request.url.replace(request.url_root+'test_header_error_on_licenseTimer', url)
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
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
        new_url = request.url.replace(request.url_root+'test_session_id_error', url)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        request_json = request.get_json()
        response = post(new_url, json=request_json, headers=request.headers)
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

    # test_health_period_disabled functions
    @app.route('/test_health_period_disabled/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_period_disabled():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_period_disabled/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_period_disabled():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_period_disabled/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period_disabled(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_disabled', url)
        request_json = request.get_json()
        context['cnt'] += 1
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        if context['cnt'] < context['nb_health']:
            response_json['metering']['healthPeriod'] = context['healthPeriod']
        else:
            response_json['metering']['healthPeriod'] = 0
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_period functions
    @app.route('/test_health_period/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_period():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_period/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_period():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_period/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_period', url)
        request_json = request.get_json()
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
        context['data'].append( (start,str(datetime.now())) )
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_period_modification functions
    @app.route('/test_health_period_modification/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_period_modification():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_period_modification/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_period_modification():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_period_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_period_modification(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_period_modification', url)
        request_json = request.get_json()
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
        context['health_period'] += 1
        context['data'].append( (start,str(datetime.now())) )
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry_disabled functions
    @app.route('/test_health_retry_disabled/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_disabled():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_disabled/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_disabled():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_retry_disabled/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_disabled(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_retry_disabled', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = tmpHealthPeriod
        response_json['metering']['healthRetry'] = 0
        response_json['metering']['healthRetrySleep'] = tmpHealthRetrySleep
        if len(context['data']) > 1:
            response.status_code = 408
        context['data'].append( (health_id,start,str(datetime.now())) )
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry functions
    @app.route('/test_health_retry/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_retry/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_retry', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
        if len(context['data']) >= 1:
            response.status_code = 408
        if health_id <= 1:
            context['data'].append( (health_id,start,str(datetime.now())) )
        else:
            context['exit'] = True
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry_modification functions
    @app.route('/test_health_retry_modification/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_modification():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_modification/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_modification():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_retry_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_modification(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_retry_modification', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
        if len(context['data']) >= 1:
            response.status_code = 408
        if health_id <= 1:
            context['data'].append( (health_id,start,str(datetime.now())) )
        else:
            context['exit'] = True
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry_sleep functions
    @app.route('/test_health_retry_sleep/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_sleep():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_sleep/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_sleep():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_retry_sleep/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_sleep(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_retry_sleep', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
        if len(context['data']) >= 1:
            response.status_code = 408
        if health_id <= nb_run:
            context['data'].append( (health_id,start,str(datetime.now())) )
        else:
            context['exit'] = True
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_retry_sleep_modification functions
    @app.route('/test_health_retry_sleep_modification/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_retry_sleep_modification():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_retry_sleep_modification/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_retry_sleep_modification():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_retry_sleep_modification/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_retry_sleep_modification(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_retry_sleep_modification', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        start = str(datetime.now())
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        response_json['metering']['healthRetrySleep'] = context['healthRetrySleep']
        if len(context['data']) >= 1:
            response.status_code = 408
        if health_id <= 1:
            context['data'].append( (health_id,start,str(datetime.now())) )
        else:
            context['exit'] = True
        return Response(dumps(response_json), response.status_code, headers)

    # test_health_metering_data functions
    @app.route('/test_health_metering_data/o/token/', methods=['GET', 'POST'])
    def otoken__test_health_metering_data():
        return redirect(request.url_root + '/o/token/', code=307)

    @app.route('/test_health_metering_data/auth/metering/genlicense/', methods=['GET', 'POST'])
    def genlicense__test_health_metering_data():
        return redirect(request.url_root + '/auth/metering/genlicense/', code=307)

    @app.route('/test_health_metering_data/auth/metering/health/', methods=['GET', 'POST'])
    def health__test_health_metering_data(path=''):
        global context
        new_url = request.url.replace(request.url_root+'test_health_metering_data', url)
        request_json = request.get_json()
        health_id = request_json['health_id']
        response = post(new_url, json=request_json, headers=request.headers)
        excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
        headers = [(name, value) for (name, value) in response.raw.headers.items() if name.lower() not in excluded_headers]
        response_json = response.json()
        response_json['metering']['healthPeriod'] = context['healthPeriod']
        response_json['metering']['healthRetry'] = context['healthRetry']
        context['health_id']= health_id
        return Response(dumps(response_json), response.status_code, headers)


    return app

