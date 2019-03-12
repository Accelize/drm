/*
Copyright (C) 2018, Accelize

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <chrono>

#include "log.h"
#include "utils.h"
#include "ws_client.h"

namespace Accelize {
namespace DRM {


CurlEasyPost::CurlEasyPost( bool verbose ) {
    curl = curl_easy_init();
    if(!curl)
        Throw(DRM_ExternFail, "Curl : cannot init curl_easy");
    if(verbose)
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
}

CurlEasyPost::~CurlEasyPost() {
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

long CurlEasyPost::perform(std::string* resp, std::chrono::steady_clock::time_point deadline) {
    CURLcode res;
    long resp_code;
    if ( headers )
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)resp);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    {//compute timeout
        std::chrono::milliseconds timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if(timeout <= std::chrono::milliseconds(0))
            Throw(DRM_WSMayRetry, "Did not perform HTTP request to Accelize webservice because deadline is already reached.");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout.count());
    }

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        if(   res == CURLE_COULDNT_RESOLVE_PROXY
              || res == CURLE_COULDNT_RESOLVE_HOST
              || res == CURLE_COULDNT_CONNECT
              || res == CURLE_OPERATION_TIMEDOUT ) {
            Throw(DRM_WSMayRetry, "Failed performing HTTP request to Accelize webservice (", curl_easy_strerror(res), ") : ", errbuff.data());
        } else {
            Throw(DRM_ExternFail, "Failed performing HTTP request to Accelize webservice (", curl_easy_strerror(res), ") : ", errbuff.data());
        }
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
    return resp_code;
}

double CurlEasyPost::getTotalTime() {
    double ret;
    if(!curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ret))
        return ret;
    else
        return 0.0;
}



DrmWSClient::DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path) {

    mUseBadOAuth2Token = std::string("");
    mOAuth2Token = std::string("");
    mTokenExpirationTime = TClock::now();
    mRetryDeadline = cRetryDeadline;
    mRequestTimeout = cRequestTimeout;

    Json::Value conf_json = parseJsonFile(conf_file_path);
    try {
        Json::Value webservice_json = JVgetRequired(conf_json, "licensing", Json::objectValue);
        Debug2("Web service configuration: ", webservice_json.toStyledString());

        std::string url = JVgetRequired(webservice_json, "url", Json::stringValue).asString();
        mOAuth2Url = url + std::string("/o/token/");
        mMeteringUrl = url + std::string("/auth/metering/genlicense/");
        Debug("Licensing URL: ", url);

        Json::Value param_lib = JVgetOptional( conf_json, "settings", Json::objectValue );
        if ( param_lib != Json::nullValue ) {
            mRetryDeadline = JVgetOptional(param_lib, "retry_deadline", Json::uintValue,
                                  cRetryDeadline).asUInt();
            mRequestTimeout = JVgetOptional(param_lib, "ws_request_timeout", Json::uintValue,
                                            cRequestTimeout).asUInt();
        }
    } catch(Exception &e) {
        if (e.getErrCode() != DRM_BadFormat)
            throw;
        Throw(DRM_BadFormat, "Error in service configuration file '", conf_file_path, "': ", e.what());
    }

    Json::Value cred_json = parseJsonFile(cred_file_path);
    try {
        Json::Value client_id_json = JVgetRequired(cred_json, "client_id", Json::stringValue);
        mClientId = client_id_json.asString();

        Json::Value client_secret_json = JVgetRequired(cred_json, "client_secret", Json::stringValue);
        mClientSecret = client_secret_json.asString();

    } catch(Exception &e) {
        if (e.getErrCode() != DRM_BadFormat)
            throw;
        Throw(DRM_BadFormat, "Error in credential file '", cred_file_path, "': ", e.what());
    }

    CurlSingleton::Init();

    // Set headers of OAuth2 request
    mOAUth2Request.setURL( mOAuth2Url );
    std::stringstream ss;
    ss << "client_id=" << mClientId << "&client_secret=" << mClientSecret << "&grant_type=client_credentials";
    mOAUth2Request.setPostFields( ss.str() );
}

Json::Value DrmWSClient::getLicense( const Json::Value& json_req, TClock::duration timeout ) {
    TClock::time_point deadline = TClock::now() + timeout;
    Debug( "Getting starting license with timeout of ", timeout.count()/1000000000, "s" );
    return getLicense( json_req, deadline );
}

Json::Value DrmWSClient::getLicense( const Json::Value& json_req, TClock::time_point deadline ) {

    Debug( "Web Service JSON request:", json_req.toStyledString() );

    while ( TClock::now() <= deadline ) {

        // Update token: request new one if needed
        updateOAuth2token( deadline );

        // Create new request
        CurlEasyPost req;
        req.setURL(mMeteringUrl);
        req.appendHeader("Accept: application/json");
        req.appendHeader("Content-Type: application/json");
        req.appendHeader(std::string("Authorization: Bearer ") + mOAuth2Token);
        req.setPostFields(saveJsonToString(json_req));

        // Send request
        Debug("Starting license request");
        std::string response;
        long resp_code = req.perform(&response, deadline);
        Debug("Received code ", resp_code, " in ", req.getTotalTime() * 1000,
              "ms");

        if ((resp_code != 200) && (resp_code != 560) && (resp_code != 400)) {
            if (!CurlEasyPost::is_error_retryable(resp_code)) {
                Throw(DRM_WSReqError, "WS HTTP response code : ", resp_code,
                      "(", response, ")");
            }
            Debug("WS HTTP response code: ", resp_code, ", ", response);
            continue;
        }

        // Parse response
        Json::Value json_resp = parseJsonString(response);
        Debug("Web Service JSON response:\n", json_resp.toStyledString());

        // Check for error with details
        if (resp_code == 560) { /*560 : Custom License generation temporary issue*/
            Throw(DRM_WSMayRetry, "Error from WS with details : ",
                  json_resp.get("detail", "Unknown error"));
        }
        if (resp_code == 400) { /*400 : Bad request */
            Throw(DRM_WSError, "Error from WS with details : ",
                  json_resp.get("detail", "Unknown error"));
        }
        return json_resp;
    }
    Throw(DRM_WSError, "Timeout: failed to get a valid license from the Web Service" );
}

void DrmWSClient::updateOAuth2token( TClock::time_point timeout ) {

    if ( !mUseBadOAuth2Token.empty() ) {
        Debug("Temporary use following token: ", mUseBadOAuth2Token);
        mOAuth2Token = mUseBadOAuth2Token;
        mTokenExpirationTime = TClock::now();
        mUseBadOAuth2Token.clear();
        return;
    }

    // Check if a token exists
    if ( !mOAuth2Token.empty() ) {
        // Check if existing token has expired or is about to expire
        if ( mTokenExpirationTime > TClock::now() ) {
            Debug( "Existing token is still valid" );
            return;
        }
        Debug( "Existing token has expired" );
    }

    // Request a new token
    Debug( "Requesting a new token to ", mOAuth2Url );
    std::string response;
    long resp_code = mOAUth2Request.perform( &response, timeout );
    Debug( "Received code ", resp_code, " from OAuth2 service in ", mOAUth2Request.getTotalTime()*1000, "ms" );
    if ( resp_code != 200 ) {
        if ( CurlEasyPost::is_error_retryable( resp_code ) ) {
            Throw( DRM_WSMayRetry, "WSOAuth HTTP response code : ", resp_code, "(", response, ")" );
        } else {
            Throw( DRM_WSReqError, "WSOAuth HTTP response code : ", resp_code, "(", response, ")" );
        }
    }
    // Parse response
    Json::Value json_resp = parseJsonString( response );
    if ( !json_resp.isMember( "access_token" ) )
        Throw( DRM_WSRespError, "Non-valid response from WSOAuth : ", response );
    Debug( "New OAuth2 token is ", json_resp["access_token"].asString(),
            "; it will expire in ", json_resp["expires_in"].asInt(), " ms" );

    mOAuth2Token = json_resp["access_token"].asString();
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( json_resp["expires_in"].asInt() - mRetryDeadline );
}

}
}
