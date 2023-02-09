/*
Copyright (C) 2022, Accelize

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
#include <chrono>
#include <unistd.h>
#include <math.h>

#include "accelize/drm/version.h"
#include "log.h"
#include "utils.h"
#include "ws_client.h"

#define DRMLIB_ENVVAR_URL           "DRMSAAS_URL"
#define DRMLIB_ENVVAR_CLIENT_ID     "DRMSAAS_CLIENT_ID"
#define DRMLIB_ENVVAR_CLIENT_SECRET "DRMSAAS_CLIENT_SECRET"

using namespace fmt::literals;

namespace Accelize {
namespace DRM {


CurlEasyPost::CurlEasyPost( const uint32_t& connection_timeout_ms ) {
    mCurl = curl_easy_init();
    if ( !mCurl )
        Throw( DRM_ExternFail, "Curl : cannot init curl_easy" );
    curl_easy_setopt( mCurl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::curl_write_callback );
    curl_easy_setopt( mCurl, CURLOPT_HEADERFUNCTION, &CurlEasyPost::curl_header_callback );
    curl_easy_setopt( mCurl, CURLOPT_ERRORBUFFER, mErrBuff.data() );
    curl_easy_setopt( mCurl, CURLOPT_FOLLOWLOCATION, 1L );
    curl_easy_setopt( mCurl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt( mCurl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt( mCurl, CURLOPT_CONNECTTIMEOUT_MS, connection_timeout_ms );
}

CurlEasyPost::~CurlEasyPost() {
    curl_easy_reset( mCurl );
    curl_slist_free_all( mHeaders_p );
    curl_slist_free_all( mHostResolveList );
    curl_easy_cleanup( mCurl );
    mHeaders_p = NULL;
    mHostResolveList = NULL;
}

void CurlEasyPost::setVerbosity( const uint32_t verbosity ) {
    curl_easy_setopt(mCurl, CURLOPT_VERBOSE, verbosity);
}

std::string CurlEasyPost::escape( const std::string str ) const {
    char *escaped_char = curl_easy_escape(mCurl, str.c_str(), str.size());
    if ( escaped_char == NULL ) {
        Throw( DRM_BadArg, "Could not URL encode the string: {}", str );
    }
    std::string escaped_str = std::string(escaped_char);
    curl_free(escaped_char);
    return escaped_str;
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

void CurlEasyPost::appendHeader( const std::string header ) {
    Debug2( "Add '{}' to CURL header", header );
    mHeaders_p = curl_slist_append( mHeaders_p, header.c_str() );
}

void CurlEasyPost::setPostFields( const std::string postfields ) {
    curl_easy_setopt( mCurl, CURLOPT_POSTFIELDSIZE, postfields.size() );
    curl_easy_setopt( mCurl, CURLOPT_COPYPOSTFIELDS, postfields.c_str() );
}

std::string CurlEasyPost::request( const std::string& url, const tHttpRequestType& httpType, const int32_t& timeout_ms ) {
    std::string resp_body;
    std::string resp_header;

    if ( timeout_ms <= 0 )
        ThrowSetLog( SPDLOG_LEVEL_DEBUG, DRM_WSTimedOut,
            "Did not perform HTTP request because deadline is reached." );

    // Configure and execute CURL command
    curl_easy_setopt( mCurl, CURLOPT_URL, url.c_str() );
    if ( mHeaders_p ) {
        curl_easy_setopt( mCurl, CURLOPT_HTTPHEADER, mHeaders_p );
    }
    if ( httpType == tHttpRequestType::GET )
        (void)0;    // No-op
    else if ( httpType == tHttpRequestType::POST )
        (void)0;    // No-op
    else if ( httpType == tHttpRequestType::PUT )
        curl_easy_setopt( mCurl, CURLOPT_CUSTOMREQUEST, "PUT");
    else if ( httpType == tHttpRequestType::PATCH )
        curl_easy_setopt( mCurl, CURLOPT_CUSTOMREQUEST, "PATCH");
    else
        Unreachable( "Invalid HTTP type. " );  //LCOV_EXCL_LINE

    curl_easy_setopt( mCurl, CURLOPT_WRITEDATA, (void*)&resp_body );
    curl_easy_setopt( mCurl, CURLOPT_HEADERDATA, (void*)&resp_header );
    curl_easy_setopt( mCurl, CURLOPT_TIMEOUT_MS, timeout_ms );
    CURLcode res = curl_easy_perform( mCurl );

    // Analyze HTTP answer
    if ( res != CURLE_OK ) {
        std::string err_msg = fmt::format("Error {} on HTTP request", (uint32_t)res);
        if ( !resp_header.empty() )
            err_msg += fmt::format(", {}", resp_header);
        err_msg += fmt::format(", {}: {}", curl_easy_strerror( res ), mErrBuff.data());
        if ( res == CURLE_COULDNT_RESOLVE_PROXY || res == CURLE_COULDNT_RESOLVE_HOST
          || res == CURLE_COULDNT_CONNECT || res == CURLE_OPERATION_TIMEDOUT ) {
            ThrowSetLog( SPDLOG_LEVEL_DEBUG, DRM_WSMayRetry, "{}", err_msg );
        } else {
            ThrowSetLog( SPDLOG_LEVEL_DEBUG, DRM_ExternFail, "{}", err_msg );
        }
    }
    long resp_code = 0;
    curl_easy_getinfo( mCurl, CURLINFO_RESPONSE_CODE, &resp_code );
    Debug( "Received HTTP response for request '{}' with code {} in {} ms",
            resp_header, resp_code, getTotalTime() * 1000 );

    // Analyze HTTP response
    DRM_ErrorCode drm_error = httpCode2DrmCode(resp_code);
    if ( drm_error != DRM_OK ) {
        // An error occurred
        ThrowSetLog( SPDLOG_LEVEL_DEBUG, drm_error, "Accelize Web Service error {} on HTTP request '{}': {}",
                    resp_code, resp_header, resp_body );
    }
    return resp_body;
}

double CurlEasyPost::getTotalTime() {
    double time_in_sec;
    if ( curl_easy_getinfo( mCurl, CURLINFO_TOTAL_TIME, &time_in_sec ) )
        Unreachable( "Failed to get the CURLINFO_TOTAL_TIME information. " ); //LCOV_EXCL_LINE
    return time_in_sec;
}



DrmWSClient::DrmWSClient( const std::string &conf_file_path, const std::string &cred_file_path ) {
    std::string url;
    mOAuth2Token = std::string("");
    mTokenValidityPeriod = 0;
    mTokenExpirationMargin = cTokenExpirationMargin;
    mTokenExpirationTime = TClock::now();

    const char* home = getenv("HOME");
    if ( !home ) {
        Throw( DRM_ExternFail, "No 'HOME' environment variable could be found: please create it." );
    }

    // Set properties based on file
    try {
        Json::Value conf_json = parseJsonFile( conf_file_path );
        Json::Value webservice_json = JVgetRequired( conf_json, "licensing", Json::objectValue );
        Debug2( "Web service configuration: {}", webservice_json.toStyledString() );

        mHostResolvesJson = JVgetOptional( webservice_json, "host_resolves", Json::objectValue );

        url = JVgetRequired( webservice_json, "url", Json::stringValue ).asString();

        Json::Value settings = JVgetOptional( conf_json, "settings", Json::objectValue );
        mRequestTimeoutMS = JVgetOptional( settings, "ws_request_timeout",
                        Json::intValue, cRequestTimeout).asInt() * 1000;
        if ( mRequestTimeoutMS == 0 )
            Throw( DRM_BadArg, "ws_request_timeout must not be 0. ");

        mConnectionTimeoutMS = JVgetOptional( settings, "ws_connection_timeout",
                        Json::intValue, cConnectionTimeout).asInt() * 1000;
        if ( mConnectionTimeoutMS == 0 )
            Throw( DRM_BadArg, "ws_connection_timeout must not be 0. ");

        mVerbosity = JVgetOptional( settings, "ws_verbosity",
                        Json::uintValue, 0).asUInt();
    } catch( Exception &e ) {
        Throw( e.getErrCode(), "Error parsing configuration file '{}: {}. ", conf_file_path, e.what() );
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
        Throw( e.getErrCode(), "Error parsing credential file '{}: {}. ", cred_file_path, e.what() );
    }
    // Restore original file log level
    if ( logFileLevel <= spdlog::level::debug )
        logFileHandler->set_level( logFileLevel );

    // Overwrite properties with Environment Variables if defined
    const char* url_var = std::getenv( DRMLIB_ENVVAR_URL );
    if ( url_var != NULL ) {
        url = std::string( url_var );
        Debug( "Use environment variable {}: {}", DRMLIB_ENVVAR_URL, url );
    }
    const char* client_id_var = std::getenv( DRMLIB_ENVVAR_CLIENT_ID );
    if ( client_id_var != NULL ) {
        mClientId = std::string( client_id_var );
        Debug( "Use environment variable {}: {}", DRMLIB_ENVVAR_CLIENT_ID, mClientId );
    }
    const char* secret_id_var = std::getenv( DRMLIB_ENVVAR_CLIENT_SECRET );
    if ( secret_id_var != NULL ) {
        mClientSecret = std::string( secret_id_var );
        Debug( "Use environment variable {}", DRMLIB_ENVVAR_CLIENT_SECRET );
    }

    // Init Curl lib
    CurlSingleton::Init();

    // Set URL of license and metering requests
    mUrl = url;
    Debug( "DRM Server URL: {}", mUrl );

    // Set path to the cache file used to save the token
    std::string tokenDir = fmt::format( "{home}{sep}.cache{sep}accelize{sep}drm",
                "sep"_a=PATH_SEP, "home"_a=home, "clientid"_a=mClientId );
    mTokenFilePath = fmt::format( "{}{}{}.json", tokenDir, PATH_SEP, mClientId );

    // Check if a cached token exists and load it
    if ( isFile( mTokenFilePath ) ) {
        Json::Value token_json = parseJsonFile( mTokenFilePath );
        mOAuth2Token = JVgetRequired( token_json, "access_token", Json::stringValue ).asString();
        mTokenValidityPeriod = JVgetRequired( token_json, "expires_in", Json::intValue ).asInt();
        mTokenExpirationTime = time_t_to_steady_clock( JVgetRequired( token_json, "expires_at", Json::uintValue ).asUInt() );
        Debug( "Loaded token from file {}: {}", mTokenFilePath, token_json.toStyledString() );
    } else {
        // Create the path if not existing
        if ( !makeDirs( tokenDir ) ) {
            Throw( DRM_ExternFail, "Failed to create cached token folder {}. ", tokenDir );
        }

    }
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

std::string DrmWSClient::escape( std::string str ) const {
    CurlEasyPost req( mConnectionTimeoutMS );
    return req.escape( str );
}

void DrmWSClient::getOAuth2token( int32_t timeout_msec ) {
    // Check if a token exists
    if ( !mOAuth2Token.empty() ) {
        // Yes a token already exists, check if it has expired or is about to expire
        if ( isTokenValid() ) {
            return;
        }
    }

    // Setup a request to get a new token
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    req.setHostResolves( mHostResolvesJson );
    req.setPostFields();

    // Send request and wait response
    if ( (timeout_msec == -1 ) || ( timeout_msec >= mRequestTimeoutMS ) )
        timeout_msec = mRequestTimeoutMS;
    std::string oauth_url = fmt::format( "{}/auth/token", mUrl );
    Debug( "Starting Authentication request to {}", oauth_url );
    std::stringstream qs;
    qs << "?grant_type=client_credentials";
    qs << "&client_id=" << mClientId;
    qs << "&client_secret=" << mClientSecret;
    std::string response_str = req.request( oauth_url + qs.str(), tHttpRequestType::GET, timeout_msec );

    // Parse response string
    Json::Value response_json;
    try {
         response_json = parseJsonString( response_str );
         Debug( "Received JSON response: {}", response_json.toStyledString() );
    } catch ( const Exception& e ) {
        Throw( DRM_WSRespError, "Failed to parse response from Authentication Web Service because {}: {}. ",
               e.what(), response_str);
    }
    mOAuth2Token = JVgetRequired( response_json, "access_token", Json::stringValue ).asString();
    mTokenValidityPeriod = JVgetRequired( response_json, "expires_in", Json::intValue ).asInt();
    mTokenExpirationTime = TClock::now() + std::chrono::seconds( mTokenValidityPeriod );
    response_json["expires_at"] = steady_clock_to_time_t( mTokenExpirationTime );

    // Cache the token
    saveJsonToFile(mTokenFilePath, response_json);
    Debug( "Saved token to file {}", mTokenFilePath );
}

Json::Value DrmWSClient::sendSaasRequest( const std::string &suburl, const tHttpRequestType &type,
                const Json::Value& json_req, int32_t timeout_msec = -1 ) const {
    // Create new request
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    req.setHostResolves( mHostResolvesJson );
    req.appendHeader( "Content-Type: application/json" );
    req.appendHeader( fmt::format("User-Agent: libaccelize_drm/{}", DRMLIB_VERSION ) );
    req.appendHeader( fmt::format( "Authorization: Bearer {}", mOAuth2Token ) );
    req.setPostFields( saveJsonToString( json_req ) );

    // Evaluate timeout with regard to the security limit
    if ( ( timeout_msec == -1 ) || ( timeout_msec >= mRequestTimeoutMS ) )
        timeout_msec = mRequestTimeoutMS;
    // Send request and wait response
    std::string url = mUrl + suburl;
    Debug( "Starting Saas request to {} with data:\n{}", url, json_req.toStyledString() );
    std::string response = req.request( url, type, timeout_msec );

    // Parse response string
    if ( !response.empty() ) {
        try {
             Json::Value json_resp = parseJsonString( response );
             Debug( "Received JSON response: {}", json_resp.toStyledString() );
             return json_resp;
        } catch ( const Exception& e ) {
            Throw( DRM_WSRespError, "Failed to parse response from Accelize Web Service because {}: {}. ",
                   e.what(), response);
        }
    }
    return Json::nullValue;
}


}
}
