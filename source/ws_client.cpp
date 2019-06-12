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
#include <unistd.h>
#include <math.h>

#include "log.h"
#include "utils.h"
#include "ws_client.h"

namespace Accelize {
namespace DRM {


CurlEasyPost::CurlEasyPost() {
    curl = curl_easy_init();
    if ( !curl )
        Throw(DRM_ExternFail, "Curl : cannot init curl_easy");
}

CurlEasyPost::~CurlEasyPost() {
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

long CurlEasyPost::perform(std::string* resp, std::chrono::steady_clock::time_point deadline) {
    CURLcode res;
    long resp_code;

    if ( headers ) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        std::string sHeader;
        for( const std::string& h: data )
            sHeader += std::string("\t") + h + std::string("\n");
        Debug2( "CURL header:\n", sHeader );
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)resp);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    { // Compute timeout
        std::chrono::milliseconds timeout = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if ( timeout <= std::chrono::milliseconds(0) )
            Throw(DRM_WSMayRetry, "Did not perform HTTP request to Accelize webservice because deadline is already reached.");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout.count());
    }

    res = curl_easy_perform(curl);
    if ( res != CURLE_OK ) {
        if ( res == CURLE_COULDNT_RESOLVE_PROXY
          || res == CURLE_COULDNT_RESOLVE_HOST
          || res == CURLE_COULDNT_CONNECT
          || res == CURLE_OPERATION_TIMEDOUT ) {
            Throw(DRM_WSMayRetry, "Failed performing HTTP request to Accelize webservice (",
                    curl_easy_strerror(res), ") : ", errbuff.data());
        } else {
            Throw(DRM_ExternFail, "Failed performing HTTP request to Accelize webservice (",
                    curl_easy_strerror(res), ") : ", errbuff.data());
        }
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
    return resp_code;
}

double CurlEasyPost::getTotalTime() {
    double ret;
    if ( !curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ret) )
        return ret;
    Unreachable( "Failed to get the CURLINFO_TOTAL_TIME information" ); //LCOV_EXCL_LINE
}



DrmWSClient::DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path) {

    mOAuth2Token = std::string("");
    mTokenValidityPeriod = 0;
    mTokenExpirationTime = TClock::now();

    try {
        Json::Value conf_json = parseJsonFile(conf_file_path);
        Json::Value webservice_json = JVgetRequired(conf_json, "licensing", Json::objectValue);
        Debug2( "Web service configuration: ", webservice_json.toStyledString() );

        std::string url = JVgetRequired(webservice_json, "url", Json::stringValue).asString();
        mOAuth2Url = url + std::string("/o/token/");
        mMeteringUrl = url + std::string("/auth/metering/genlicense/");
        Debug( "Licensing URL: ", url );

    } catch(Exception &e) {
        Throw( e.getErrCode(), "Error with service configuration file '",
                conf_file_path, "': ", e.what() );
    }

    try {
        Json::Value cred_json = parseJsonFile(cred_file_path);
        Json::Value client_id_json = JVgetRequired(cred_json, "client_id", Json::stringValue);
        mClientId = client_id_json.asString();
        Json::Value client_secret_json = JVgetRequired(cred_json, "client_secret", Json::stringValue);
        mClientSecret = client_secret_json.asString();
    } catch(Exception &e) {
        Throw( e.getErrCode(), "Error with credential file '", cred_file_path, "': ", e.what() );
    }

    CurlSingleton::Init();

    // Set headers of OAuth2 request
    mOAUth2Request.setURL( mOAuth2Url );
    std::stringstream ss;
    ss << "client_id=" << mClientId << "&client_secret=" << mClientSecret;
    ss << "&grant_type=client_credentials";
    mOAUth2Request.setPostFields( ss.str() );
}

uint32_t DrmWSClient::getTokenTimeLeft() const {
    TClock::duration delta = mTokenExpirationTime - TClock::now();
    return (uint32_t)round( (double)delta.count() / 1000000000 );
}

void DrmWSClient::setOAuth2token( const std::string& token ) {
    mOAuth2Token = token;
    mTokenValidityPeriod = 10;
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( mTokenValidityPeriod );
}

void DrmWSClient::requestOAuth2token( TClock::time_point deadline ) {

    // Check if a token exists
    if (!mOAuth2Token.empty()) {
        // Check if existing token has expired or is about to expire
        if (mTokenExpirationTime > TClock::now()) {
            Debug( "Current authentication token is still valid" );
            return;
        }
        Debug( "Current authentication token has expired" );
    }

    // Request a new token and wait response
    Debug( "Requesting a new authentication token from ", mOAuth2Url );
    std::string response;
    long resp_code = mOAUth2Request.perform( &response, deadline );

    // Parse response
    std::string formatted_response, error_msg;
    Json::Value json_resp;
    try {
        json_resp = parseJsonString(response);
        formatted_response = json_resp.toStyledString();
    } catch ( const Exception& e ) {
        json_resp = Json::nullValue;
        formatted_response = response;
        error_msg = e.what();
    }
    Debug( "Received code ", resp_code, " from OAuth2 Web Service in ",
           mOAUth2Request.getTotalTime() * 1000, "ms with following response:\n", formatted_response );

    // Analyze response
    if ( resp_code != 200 ) {
        // An error occurred
        // Extract the error message
        std::stringstream msg;
        msg << "HTTP response code from OAuth2 Web Service: " << resp_code;
        std::string error_details = json_resp.get("error", "").asString();
        if ( error_details.size() )
            msg << " (" << error_details << ")";
        else
            msg << " (" << response << ")";

        if (CurlEasyPost::is_error_retryable(resp_code)) {
            Throw(DRM_WSMayRetry, msg.str());
        } else if ( (resp_code >= 400) && (resp_code < 500) ) {
            Throw(DRM_WSReqError, msg.str());
        } else {
            Throw(DRM_WSError, msg.str());
        }
    }

    // Verify response parsing
    if ( json_resp == Json::nullValue )
        Throw( DRM_WSRespError, "Failed to parse response from OAuth2 Web Service: ", error_msg );

    mOAuth2Token = JVgetRequired( json_resp, "access_token", Json::stringValue ).asString();
    mTokenValidityPeriod = JVgetRequired( json_resp, "expires_in", Json::intValue ).asInt();
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( mTokenValidityPeriod );
}


Json::Value DrmWSClient::requestLicense( const Json::Value& json_req, TClock::time_point deadline ) {

    // Create new request
    CurlEasyPost req;
    req.setURL(mMeteringUrl);
    req.appendHeader( "Accept: application/json" );
    req.appendHeader( "Content-Type: application/json" );
    req.appendHeader( std::string("Authorization: Bearer ") + mOAuth2Token );
    req.setPostFields( saveJsonToString(json_req) );

    // Send request and wait response
    Debug( "Starting license request to ", mMeteringUrl, " with request:\n", json_req.toStyledString() );
    std::string response;
    long resp_code = req.perform( &response, deadline );

    // Parse response
    std::string formatted_response, error_msg;
    Json::Value json_resp;
    try {
        json_resp = parseJsonString(response);
        formatted_response = json_resp.toStyledString();
    } catch ( const Exception& e ) {
        json_resp = Json::nullValue;
        formatted_response = response;
        error_msg = e.what();
    }
    Debug( "Received code ", resp_code, " from License Web Service in ",
           req.getTotalTime() * 1000, "ms with following response\n", formatted_response );

    // Analyze response
    if ( resp_code != 200 ) {
        // An error occurred
        // Extract the error message
        std::stringstream msg;
        msg << "HTTP response code from License Web Service: " << resp_code;
        std::string error_details = json_resp.get("detail", "").asString();
        if ( error_details.size() )
            msg << " (" << error_details << ")";

        if (CurlEasyPost::is_error_retryable(resp_code)) {
            Throw(DRM_WSMayRetry, msg.str());
        } else if ((resp_code >= 400) && (resp_code < 500)) {
            Throw(DRM_WSReqError, msg.str());
        } else {
            Throw(DRM_WSError, msg.str());
        }
    }
    // Verify response parsing
    if ( json_resp == Json::nullValue )
        Throw( DRM_WSRespError, "Failed to parse response from License Web Service: ", error_msg );

    // No error: return the response as JSON object
    return json_resp;
}

}
}
