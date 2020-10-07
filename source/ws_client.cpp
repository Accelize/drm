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
    mConnectionTimeoutMS = cConnectionTimeoutMS;
    mCurl = curl_easy_init();
    if ( !mCurl )
        Throw( DRM_ExternFail, "Curl : cannot init curl_easy" );
    curl_easy_setopt( mCurl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::write_callback );
    curl_easy_setopt( mCurl, CURLOPT_ERRORBUFFER, mErrBuff.data() );
    curl_easy_setopt( mCurl, CURLOPT_FOLLOWLOCATION, 1L );
    curl_easy_setopt( mCurl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt( mCurl, CURLOPT_TCP_KEEPALIVE, 1L);
}

CurlEasyPost::~CurlEasyPost() {
    curl_easy_reset( mCurl );
    curl_easy_cleanup( mCurl );
    data.clear();
    curl_slist_free_all( mHeaders_p );
    curl_slist_free_all( mHostResolveList );
    mHeaders_p = NULL;
    mHostResolveList = NULL;
}

void CurlEasyPost::setVerbosity( const uint32_t verbosity ) {
    curl_easy_setopt(mCurl, CURLOPT_VERBOSE, verbosity);
}

void CurlEasyPost::setHostResolves( const Json::Value& host_json ) {
    if ( host_json != Json::nullValue ) {
        if ( mHostResolveList != NULL ) {
            curl_slist_free_all( mHostResolveList );
            mHostResolveList = NULL;
        }
        for( Json::ValueConstIterator it = host_json.begin(); it != host_json.end(); it++ ) {
            std::string key = it.key().asString();
            std::string val = (*it).asString();
            std::string host_str = fmt::format( "{}:{}", key, val );
            mHostResolveList = curl_slist_append( mHostResolveList, host_str.c_str() );
        }
        if ( curl_easy_setopt(mCurl, CURLOPT_RESOLVE, mHostResolveList) == CURLE_UNKNOWN_OPTION )
            Warning( "Could not set the CURL Host resolve option: {}", host_json.toStyledString() );
        else
            Debug( "Set the following CURL Host resolve option: {}", host_json.toStyledString() );
    }
}

uint32_t CurlEasyPost::perform( const std::string url, std::string* response, int32_t timeout_ms ) {
    CURLcode res;
    uint32_t resp_code;

    if ( timeout_ms <= 0 )
        Throw( DRM_WSTimedOut, "Did not perform HTTP request to Accelize webservice because deadline is reached." );

    // Configure and execute CURL command
    curl_easy_setopt( mCurl, CURLOPT_URL, url.c_str() );
    if ( mHeaders_p ) {
        curl_easy_setopt( mCurl, CURLOPT_HTTPHEADER, mHeaders_p );
    }
    curl_easy_setopt( mCurl, CURLOPT_WRITEDATA, response );
    curl_easy_setopt( mCurl, CURLOPT_CONNECTTIMEOUT_MS, mConnectionTimeoutMS );
    curl_easy_setopt( mCurl, CURLOPT_TIMEOUT_MS, timeout_ms );
    res = curl_easy_perform( mCurl );

    // Analyze libcurl response
    if ( res != CURLE_OK ) {
        // A libcurl error occurred
        if ( res == CURLE_COULDNT_RESOLVE_PROXY
          || res == CURLE_COULDNT_RESOLVE_HOST
          || res == CURLE_COULDNT_CONNECT
          || res == CURLE_OPERATION_TIMEDOUT ) {
            Throw( DRM_WSMayRetry, "libcurl failed to perform HTTP request to Accelize webservice ({}) : {}",
                    curl_easy_strerror( res ), mErrBuff.data() );
        } else {
            Throw( DRM_ExternFail, "libcurl failed to perform HTTP request to Accelize webservice ({}) : {}",
                    curl_easy_strerror( res ), mErrBuff.data() );
        }
    }
    curl_easy_getinfo( mCurl, CURLINFO_RESPONSE_CODE, &resp_code );
    Debug( "Received code {} from {} in {} ms", resp_code, url, getTotalTime() * 1000 );
    return resp_code;
}

uint32_t CurlEasyPost::perform( const std::string url, std::string* response,
                                std::chrono::steady_clock::time_point& deadline ) {
    std::chrono::milliseconds timeout_chrono = std::chrono::duration_cast<std::chrono::milliseconds>(
                        deadline - std::chrono::steady_clock::now() );
    int32_t timeout_ms = timeout_chrono.count();
    if ( timeout_ms >= (int32_t)mConnectionTimeoutMS )
        timeout_ms = mConnectionTimeoutMS;
    return perform( url, response, timeout_ms );
}

std::string CurlEasyPost::perform_put( const std::string url, const uint32_t& timeout_ms ) {
    std::string response;
    uint32_t resp_code;

    if ( timeout_ms <= 0 )
        Throw( DRM_WSTimedOut, "Did not perform HTTP request to Accelize webservice because deadline is reached." );

    // Configure and execute CURL command
    curl_easy_setopt( mCurl, CURLOPT_URL, url.c_str() );
    if ( mHeaders_p ) {
        curl_easy_setopt( mCurl, CURLOPT_HTTPHEADER, mHeaders_p );
    }
    curl_easy_setopt( mCurl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt( mCurl, CURLOPT_WRITEDATA, &response );
    curl_easy_setopt( mCurl, CURLOPT_CONNECTTIMEOUT_MS, mConnectionTimeoutMS );
    curl_easy_setopt( mCurl, CURLOPT_TIMEOUT_MS, timeout_ms );
    CURLcode res = curl_easy_perform( mCurl );

    // Analyze HTTP answer
    if ( res != CURLE_OK ) {
        if ( res == CURLE_COULDNT_RESOLVE_PROXY
          || res == CURLE_COULDNT_RESOLVE_HOST
          || res == CURLE_COULDNT_CONNECT
          || res == CURLE_OPERATION_TIMEDOUT ) {
            Throw( DRM_WSMayRetry, "libcurl failed to perform HTTP request to Accelize webservice ({}) : {}",
                    curl_easy_strerror( res ), mErrBuff.data() );
        } else {
            Throw( DRM_ExternFail, "libcurl failed to perform HTTP request to Accelize webservice ({}) : {}",
                    curl_easy_strerror( res ), mErrBuff.data() );
        }
    }
    curl_easy_getinfo( mCurl, CURLINFO_RESPONSE_CODE, &resp_code );
    Debug( "Received code {} from {} in {} ms", resp_code, url, getTotalTime() * 1000 );
    return response;
}

double CurlEasyPost::getTotalTime() {
    double ret;
    if ( !curl_easy_getinfo( mCurl, CURLINFO_TOTAL_TIME, &ret ) )
        return ret;
    Unreachable( "Failed to get the CURLINFO_TOTAL_TIME information" ); //LCOV_EXCL_LINE
}



DrmWSClient::DrmWSClient( const std::string &conf_file_path, const std::string &cred_file_path ) {

    std::string url;

    mOAuth2Token = std::string("");
    mTokenValidityPeriod = 0;
    mTokenExpirationMargin = cTokenExpirationMargin;
    mTokenExpirationTime = TClock::now();

    // Set properties based on file
    try {
        Json::Value conf_json = parseJsonFile( conf_file_path );
        Json::Value webservice_json = JVgetRequired( conf_json, "licensing", Json::objectValue );
        Debug2( "Web service configuration: {}", webservice_json.toStyledString() );

        mHostResolvesJson = JVgetOptional( webservice_json, "host_resolves", Json::objectValue );

        url = JVgetRequired( webservice_json, "url", Json::stringValue ).asString();

        Json::Value settings = JVgetOptional( conf_json, "settings", Json::objectValue );
        mRequestTimeout = JVgetOptional( settings, "ws_request_timeout",
                        Json::uintValue, cRequestTimeout).asUInt();
        if ( mRequestTimeout == 0 )
            Throw( DRM_BadArg, "ws_request_timeout must not be 0");
        mVerbosity = JVgetOptional( settings, "ws_verbosity",
                        Json::uintValue, 0).asUInt();

    } catch( Exception &e ) {
        Throw( e.getErrCode(), "Error with service configuration file '{}': {}",
                conf_file_path, e.what() );
    }

    // Temporarily change file log level to be sure not to capture the Client and Secret IDs
    auto logFileHandler = sLogger->sinks()[1];
    spdlog::level::level_enum logFileLevel = logFileHandler->level();
    if ( logFileLevel <= spdlog::level::debug )
        logFileHandler->set_level( spdlog::level::info );
    try {
        Json::Value cred_json = parseJsonFile( cred_file_path );
        Json::Value client_id_json = JVgetRequired( cred_json, "client_id", Json::stringValue );
        mClientId = client_id_json.asString();
        Json::Value client_secret_json = JVgetRequired( cred_json, "client_secret", Json::stringValue );
        mClientSecret = client_secret_json.asString();
    } catch( Exception &e ) {
        Throw( e.getErrCode(), "Error with credential file '{}': {}", cred_file_path, e.what() );
    }
    // Restore original file log level
    if ( logFileLevel <= spdlog::level::debug )
        logFileHandler->set_level( logFileLevel );

    // Overwrite properties with Environment Variables if defined
    const char* url_var = std::getenv( "ONEPORTAL_URL" );
    if ( url_var != NULL )
        url = std::string( url_var );
    const char* client_id_var = std::getenv( "ONEPORTAL_CLIENT_ID" );
    if ( client_id_var != NULL )
        mClientId = std::string( client_id_var );
    const char* secret_id_var = std::getenv( "ONEPORTAL_CLIENT_SECRET" );
    if ( secret_id_var != NULL )
        mClientSecret = std::string( secret_id_var );

    // Init Curl lib
    CurlSingleton::Init();

    // Set URL of license and metering requests
    mOAuth2Url = url + std::string("/o/token/");
    mLicenseUrl = url + std::string("/auth/metering/genlicense/");
    mHealthUrl = url + std::string("/auth/metering/health/");

    Debug( "OAuth URL: {}", mOAuth2Url );
    Debug( "Licensing URL: {}", mLicenseUrl );
    Debug( "Health URL: {}", mHealthUrl );
}

int32_t DrmWSClient::getTokenTimeLeft() const {
    TClock::duration delta = mTokenExpirationTime - TClock::now();
    return (uint32_t)round( (double)delta.count() / 1000000000 );
}

bool DrmWSClient::isTokenValid() const {
    uint32_t margin = ( mTokenExpirationMargin >= mTokenValidityPeriod ) ?
        ( mTokenValidityPeriod >> 1) : mTokenExpirationMargin;
    if ( ( mTokenExpirationTime - std::chrono::seconds( margin ) ) > TClock::now() ) {
        Debug( "Current authentication token is still valid" );
        return true;
    } else {
        if ( mTokenExpirationTime > TClock::now() )
            Debug( "Current authentication token is about to expire in {} seconds maximum", margin );
        else
            Debug( "Current authentication token has expired" );
        return false;
    }
}

void DrmWSClient::requestOAuth2token( TClock::time_point deadline ) {

    // Check if a token exists
    if ( !mOAuth2Token.empty() ) {
        // Yes a token already exists, check if it has expired or is about to expire
        if ( isTokenValid() ) {
            return;
        }
    }

    // Request a new token and wait response
    CurlEasyPost req;
    req.setVerbosity( mVerbosity );
    req.setConnectionTimeoutMS( mRequestTimeout * 1000 );
    req.setHostResolves( mHostResolvesJson );
    std::stringstream ss;
    ss << "client_id=" << mClientId << "&client_secret=" << mClientSecret;
    ss << "&grant_type=client_credentials";
    req.setPostFields( ss.str() );

    Debug( "Starting OAuthentication request to {}", mOAuth2Url );
    std::string response;
    long resp_code = req.perform( mOAuth2Url, &response, deadline );

    // Parse response
    std::string error_msg;
    Json::Value json_resp;
    try {
        json_resp = parseJsonString( response );
    } catch ( const Exception& e ) {
        json_resp = Json::nullValue;
        error_msg = e.what();
    }
    // Analyze response
    DRM_ErrorCode drm_error = CurlEasyPost::httpCode2DrmCode( resp_code );
    if ( drm_error != DRM_OK )
        Throw( drm_error, "OAuth2 Web Service error {}: {}", resp_code, response );

    // Verify response parsing
    if ( json_resp == Json::nullValue )
        Throw( DRM_WSRespError, "Failed to parse response from OAuth2 Web Service because {}: {}",
                error_msg, response);

    mOAuth2Token = JVgetRequired( json_resp, "access_token", Json::stringValue ).asString();
    mTokenValidityPeriod = JVgetRequired( json_resp, "expires_in", Json::intValue ).asInt();
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( mTokenValidityPeriod );
}

Json::Value DrmWSClient::requestMetering( const std::string url, const Json::Value& json_req, TClock::time_point deadline ) {

    // Create new request
    CurlEasyPost req;
    req.setVerbosity( mVerbosity );
    req.setConnectionTimeoutMS( mRequestTimeout * 1000 );
    req.setHostResolves( mHostResolvesJson );
    req.appendHeader( "Accept: application/vnd.accelize.v1+json" );
    req.appendHeader( "Content-Type: application/json" );
    std::string token_header("Authorization: Bearer ");
    token_header += mOAuth2Token;
    req.appendHeader( token_header );
    req.setPostFields( saveJsonToString( json_req ) );

    // Send request and wait response
    std::string response;
    long resp_code = req.perform( url, &response, deadline );

    // Parse response
    std::string error_msg;
    Json::Value json_resp;
    try {
        json_resp = parseJsonString( response );
    } catch ( const Exception& e ) {
        json_resp = Json::nullValue;
        error_msg = e.what();
    }

    // Analyze response
    DRM_ErrorCode drm_error = CurlEasyPost::httpCode2DrmCode( resp_code );
    if ( resp_code == 401 )
        drm_error = DRM_WSMayRetry;
    // An error occurred
    if ( drm_error != DRM_OK )
        Throw( drm_error, "Metering Web Service error {}: {}", resp_code, response );

    // Verify response parsing
    if ( json_resp == Json::nullValue )
        Throw( DRM_WSRespError, "Failed to parse response from Metering Web Service because {}: {}",
               error_msg, response);

    // No error: return the response as JSON object
    return json_resp;
}

Json::Value DrmWSClient::requestLicense( const Json::Value& json_req, TClock::time_point deadline ) {
    Debug( "Starting License request to {} with data:\n{}", mLicenseUrl, json_req.toStyledString() );
    return requestMetering( mLicenseUrl, json_req, deadline );
}

Json::Value DrmWSClient::requestHealth( const Json::Value& json_req, TClock::time_point deadline ) {
    Debug( "Starting Health request to {} with data:\n{}", mHealthUrl, json_req.toStyledString() );
    return requestMetering( mHealthUrl, json_req, deadline );
}

}
}
