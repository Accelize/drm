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
        Throw( DRM_ExternFail, "Curl : cannot init curl_easy" );
}

CurlEasyPost::~CurlEasyPost() {
    data.clear();
    curl_easy_cleanup( curl );
    curl_slist_free_all( headers );
    curl_slist_free_all( host_resolve_list );
    headers = NULL;
    host_resolve_list = NULL;
}

void CurlEasyPost::setHostResolves( const Json::Value& host_json ) {
    if ( host_json != Json::nullValue ) {
        if ( host_resolve_list != NULL ) {
            curl_slist_free_all( host_resolve_list );
            host_resolve_list = NULL;
        }
        for( Json::ValueConstIterator it = host_json.begin(); it != host_json.end(); it++ ) {
            std::string key = it.key().asString();
            std::string val = (*it).asString();
            std::string host_str = fmt::format( "{}:{}", key, val );
            host_resolve_list = curl_slist_append( host_resolve_list, host_str.c_str() );
        }
        if ( curl_easy_setopt(curl, CURLOPT_RESOLVE, host_resolve_list) == CURLE_UNKNOWN_OPTION )
            Warning( "Could not set the CURL Host resolve option: {}", host_json.toStyledString() );
        else
            Debug( "Set the following CURL Host resolve option: {}", host_json.toStyledString() );
    }
}

long CurlEasyPost::perform( std::string* resp, std::chrono::steady_clock::time_point deadline ) {
    CURLcode res;
    long resp_code;

    if ( headers ) {
        curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
    }
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::write_callback );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void*)resp );
    curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, errbuff.data() );
    curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
    curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT_MS, cConnectionTimeout );

    { // Compute timeout
        std::chrono::milliseconds timeout = std::chrono::duration_cast<std::chrono::milliseconds>( deadline - std::chrono::steady_clock::now() );
        if ( timeout <= std::chrono::milliseconds( 0 ) )
            Throw( DRM_WSMayRetry, "Did not perform HTTP request to Accelize webservice because deadline is already reached." );
        curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, timeout.count() );
    }
    res = curl_easy_perform( curl );
    if ( res != CURLE_OK ) {
        if ( res == CURLE_COULDNT_RESOLVE_PROXY
          || res == CURLE_COULDNT_RESOLVE_HOST
          || res == CURLE_COULDNT_CONNECT
          || res == CURLE_OPERATION_TIMEDOUT ) {
            Throw( DRM_WSMayRetry, "Failed performing HTTP request to Accelize webservice ({}) : {}",
                    curl_easy_strerror( res ), errbuff.data() );
        } else {
            Throw( DRM_ExternFail, "Failed performing HTTP request to Accelize webservice ({}) : {}",
                    curl_easy_strerror( res ), errbuff.data() );
        }
    }
    curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &resp_code );
    return resp_code;
}

double CurlEasyPost::getTotalTime() {
    double ret;
    if ( !curl_easy_getinfo( curl, CURLINFO_TOTAL_TIME, &ret ) )
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
    // Restore originla file log level
    if ( logFileLevel <= spdlog::level::debug )
        logFileHandler->set_level( logFileLevel );

    // Overwrite properties with Environment Variables if defined
    const char* url_var = std::getenv( "ONEPORTAL_URL" );
    if ( url_var != NULL )
        url = std::string( url_var );
    Debug( "Licensing URL: {}", url );
    const char* client_id_var = std::getenv( "ONEPORTAL_CLIENT_ID" );
    if ( client_id_var != NULL )
        mClientId = std::string( client_id_var );
    const char* secret_id_var = std::getenv( "ONEPORTAL_CLIENT_SECRET" );
    if ( secret_id_var != NULL )
        mClientSecret = std::string( secret_id_var );

    // Init Curl lib
    CurlSingleton::Init();

    // Set header of OAuth2 request
    mOAUth2Request.setHostResolves( mHostResolvesJson );
    mOAUth2Request.setURL( url + std::string("/o/token/") );
    std::stringstream ss;
    ss << "client_id=" << mClientId << "&client_secret=" << mClientSecret;
    ss << "&grant_type=client_credentials";
    mOAUth2Request.setPostFields( ss.str() );

    // Set URL of license request
    mMeteringUrl = url + std::string("/auth/metering/genlicense/");
}

int32_t DrmWSClient::getTokenTimeLeft() const {
    TClock::duration delta = mTokenExpirationTime - TClock::now();
    return (uint32_t)round( (double)delta.count() / 1000000000 );
}

void DrmWSClient::setOAuth2token( const std::string& token ) {
    mOAuth2Token = token;
    mTokenValidityPeriod = 10;
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( mTokenValidityPeriod );
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
    Debug( "Requesting a new authentication token" );
    std::string response;
    long resp_code = mOAUth2Request.perform( &response, deadline );

    // Parse response
    std::string error_msg;
    Json::Value json_resp;
    try {
        json_resp = parseJsonString( response );
    } catch ( const Exception& e ) {
        json_resp = Json::nullValue;
        error_msg = e.what();
    }
    Debug( "Received code {} from OAuth2 Web Service in {} ms",
            resp_code, mOAUth2Request.getTotalTime() * 1000 );

    // Analyze response
    if ( resp_code != 200 ) {
        // An error occurred
        DRM_ErrorCode drm_error;
        if ( CurlEasyPost::is_error_retryable( resp_code ) )
            drm_error = DRM_WSMayRetry;
        else if ( ( resp_code >= 400 ) && ( resp_code < 500 ) )
            drm_error = DRM_WSReqError;
        else
            drm_error = DRM_WSError;
        Throw( drm_error, "OAuth2 Web Service error {}: {}", resp_code, response );
    }

    // Verify response parsing
    if ( json_resp == Json::nullValue )
        Throw( DRM_WSRespError, "Failed to parse response from OAuth2 Web Service because {}: {}",
                error_msg, response);

    mOAuth2Token = JVgetRequired( json_resp, "access_token", Json::stringValue ).asString();
    mTokenValidityPeriod = JVgetRequired( json_resp, "expires_in", Json::intValue ).asInt();
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( mTokenValidityPeriod );
}


Json::Value DrmWSClient::requestLicense( const Json::Value& json_req, TClock::time_point deadline ) {

    // Create new request
    CurlEasyPost req;
    req.setHostResolves( mHostResolvesJson );
    req.setURL( mMeteringUrl );
    req.appendHeader( "Accept: application/json" );
    req.appendHeader( "Content-Type: application/json" );
    req.appendHeader( std::string("Authorization: Bearer ") + mOAuth2Token );
    req.setPostFields( saveJsonToString( json_req ) );

    // Send request and wait response
    Debug( "Starting license request to {} with request:\n{}", mMeteringUrl, json_req.toStyledString() );
    std::string response;
    long resp_code = req.perform( &response, deadline );

    // Parse response
    std::string error_msg;
    Json::Value json_resp;
    try {
        json_resp = parseJsonString( response );
    } catch ( const Exception& e ) {
        json_resp = Json::nullValue;
        error_msg = e.what();
    }
    Debug( "Received code {} from License Web Service in {} ms",
            resp_code, req.getTotalTime() * 1000 );

    // Analyze response
    if ( resp_code != 200 ) {
        // An error occurred
        DRM_ErrorCode drm_error;
        if ( CurlEasyPost::is_error_retryable( resp_code ) || ( resp_code == 401 ) )
            drm_error = DRM_WSMayRetry;
        else if ( ( resp_code >= 400 ) && ( resp_code < 500 ) )
            drm_error = DRM_WSReqError;
        else
            drm_error = DRM_WSError;
        Throw( drm_error, "License Web Service error {}: {}", resp_code, response );
    }
    // Verify response parsing
    if ( json_resp == Json::nullValue )
        Throw( DRM_WSRespError, "Failed to parse response from License Web Service because {}: {}",
               error_msg, response);

    // No error: return the response as JSON object
    return json_resp;
}

}
}
