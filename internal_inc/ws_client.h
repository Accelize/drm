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

#ifndef _H_ACCELIZE_DRM_WS_CLIENT
#define _H_ACCELIZE_DRM_WS_CLIENT

#include <string>
#include <list>
#include <chrono>
#include <json/json.h>
#include <curl/curl.h>

#include "log.h"


namespace Accelize {
namespace DRM {


// RAII for Curl global init/cleanup
class CurlSingleton {

private:
    CurlSingleton() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlSingleton() { curl_global_cleanup(); }

public:
    CurlSingleton(const CurlSingleton&) = delete;
    CurlSingleton& operator=(const CurlSingleton&) = delete;

    static void Init() {
        static CurlSingleton g_curl;
        (void) g_curl;
    }

};


// RAII for Curl easy
class CurlEasyPost {

private:
    CURL *mCurl = NULL;
    struct curl_slist *mHeaders_p = NULL;
    struct curl_slist *mHostResolveList = NULL;
    std::array<char, CURL_ERROR_SIZE> mErrBuff;
    int32_t mConnectionTimeout;   // Default timeout (in seconds) to establish connection

public:
    static bool is_error_retryable(long resp_code) {
        return        resp_code == 408 // Request Timeout
                   || resp_code == 429 // Too Many Requests
                   || resp_code == 470 // Floating License: no token available
                   || resp_code == 495 // SSL Certificate Error
                   || resp_code == 500 // Internal Server Error
                   || resp_code == 502 // Bad Gateway
                   || resp_code == 503 // Service Unavailable
                   || resp_code == 504 // Gateway timeout
                   || resp_code == 505 // HTTP version not supported
                   || resp_code == 507 // Insufficient storage
                   || resp_code == 520 // Unknown Error
                   || resp_code == 521 // Web Server is Down
                   || resp_code == 522 // Connection Timed Out
                   || resp_code == 524 // A Timeout Occurred
                   || resp_code == 525 // SSL Handshake Failed
                   || resp_code == 526 // Invalid SSL Certificate
                   || resp_code == 527 // Railgun Error
                   || resp_code == 530 // Origin DNS Error
                   || resp_code == 560 // Accelize License generation temporary issue
                ;
    }
    static DRM_ErrorCode httpCode2DrmCode( const uint32_t http_resp_code ) {
        if ( http_resp_code == 200 )
            return DRM_OK;
        if ( CurlEasyPost::is_error_retryable( http_resp_code ) )
            return DRM_WSMayRetry;
        if ( ( http_resp_code >= 400 ) && ( http_resp_code < 500 ) )
            return DRM_WSReqError;
        return DRM_WSError;
    }

    CurlEasyPost( const uint32_t& connection_timeout );
    ~CurlEasyPost();

    double getTotalTime();

    void setVerbosity( const uint32_t verbosity );
    void setHostResolves( const Json::Value& host_json );

    void appendHeader( const std::string header );
    void setPostFields( const std::string& postfields );

    uint32_t perform( const std::string url, std::string* resp, const int32_t timeout_ms );

    template<class T>
    T perform_get( const std::string url, const uint32_t& timeout_ms ) {
        T response;
        uint32_t resp_code;

        if ( timeout_ms <= 0 )
            Throw( DRM_WSTimedOut, "Did not perform HTTP request to Accelize webservice because deadline is reached." );

        // Configure and execute CURL command
        curl_easy_setopt( mCurl, CURLOPT_URL, url.c_str() );
        if ( mHeaders_p ) {
            curl_easy_setopt( mCurl, CURLOPT_HTTPHEADER, mHeaders_p );
        }
        curl_easy_setopt( mCurl, CURLOPT_WRITEDATA, (void*)&response );
        curl_easy_setopt( mCurl, CURLOPT_TIMEOUT_MS, timeout_ms );
        CURLcode res = curl_easy_perform( mCurl );

        // Analyze libcurl response
        if ( res != CURLE_OK ) {
            if ( res == CURLE_COULDNT_RESOLVE_PROXY
              || res == CURLE_COULDNT_RESOLVE_HOST
              || res == CURLE_COULDNT_CONNECT
              || res == CURLE_OPERATION_TIMEDOUT ) {
                Throw( DRM_WSMayRetry, "Failed to perform HTTP request to Accelize webservice ({}) : {}", curl_easy_strerror( res ), mErrBuff.data() );  //LCOV_EXCL_LINE
            } else {
                Throw( DRM_ExternFail, "Failed to perform HTTP request to Accelize webservice ({}) : {}", curl_easy_strerror( res ), mErrBuff.data() );  //LCOV_EXCL_LINE
            }
        }
        curl_easy_getinfo( mCurl, CURLINFO_RESPONSE_CODE, &resp_code );
        Debug( "Received code {} from {} in {} ms", resp_code, url, getTotalTime() * 1000 );

        // Analyze HTTP response
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
        return response;
    }

    template<class T>
    T perform_put( const std::string url, const uint32_t& timeout_ms ) {
        T response;
        uint32_t resp_code;

        if ( timeout_ms <= 0 )
            Throw( DRM_WSTimedOut, "Did not perform HTTP request to Accelize webservice because deadline is reached." );

        // Configure and execute CURL command
        curl_easy_setopt( mCurl, CURLOPT_URL, url.c_str() );
        if ( mHeaders_p ) {
            curl_easy_setopt( mCurl, CURLOPT_HTTPHEADER, mHeaders_p );
        }
        curl_easy_setopt( mCurl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt( mCurl, CURLOPT_WRITEDATA, (void*)&response );
        curl_easy_setopt( mCurl, CURLOPT_TIMEOUT_MS, timeout_ms );
        CURLcode res = curl_easy_perform( mCurl );

        // Analyze HTTP answer
        if ( res != CURLE_OK ) {
            if ( res == CURLE_COULDNT_RESOLVE_PROXY
              || res == CURLE_COULDNT_RESOLVE_HOST
              || res == CURLE_COULDNT_CONNECT
              || res == CURLE_OPERATION_TIMEDOUT ) {
                Throw( DRM_WSMayRetry, "Failed to perform HTTP request to Accelize webservice ({}) : {}",
                        curl_easy_strerror( res ), mErrBuff.data() );
            } else {
                Throw( DRM_ExternFail, "Failed to perform HTTP request to Accelize webservice ({}) : {}",
                        curl_easy_strerror( res ), mErrBuff.data() );
            }
        }
        curl_easy_getinfo( mCurl, CURLINFO_RESPONSE_CODE, &resp_code );
        Debug( "Received code {} from {} in {} ms", resp_code, url, getTotalTime() * 1000 );
        return response;
    }

protected:

    static size_t write_callback( void *contents, size_t size, size_t nmemb, std::string *userp ) {
        size_t realsize = size * nmemb;
        try {
            userp->append( (const char*)contents, realsize );
        } catch( const std::bad_alloc& e ) {  //LCOV_EXCL_LINE
            Throw( DRM_ExternFail, "Curl write callback exception: {}", e.what() );  //LCOV_EXCL_LINE
        }
        return realsize;
    }

};


/*DRM Web Service client : communicate with Accelize DRM web server to
 get license and send metering data*/
class DrmWSClient {

    const uint32_t cTokenExpirationMargin = 60;  // In seconds
    const int32_t cRequestTimeout = 30;          // In seconds
    const int32_t cConnectionTimeout = 15;       // In seconds

protected:

    typedef std::chrono::steady_clock TClock; /// Shortcut type def to steady clock which is monotonic (so unaffected by clock adjustments)

    uint32_t mVerbosity;
    std::string mClientId;
    std::string mClientSecret;
    std::string mOAuth2Token;
    std::string mOAuth2Url;
    std::string mLicenseUrl;
    std::string mHealthUrl;
    Json::Value mHostResolvesJson;
    uint32_t mTokenValidityPeriod;              /// Validation period of the OAuth2 token in seconds
    uint32_t mTokenExpirationMargin;            /// OAuth2 token expiration margin in seconds
    TClock::time_point mTokenExpirationTime;    /// OAuth2 expiration time
    int32_t mRequestTimeout;                    /// Maximum period in seconds for a request to complete
    int32_t mConnectionTimeout;                 /// Maximum period in seconds for the client to connect the server

    bool isTokenValid() const;
    Json::Value requestMetering( const std::string url, const Json::Value& json_req, int32_t timeout_msec );

public:
    DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path);
    ~DrmWSClient() = default;

    uint32_t getVerbosity() const { return mVerbosity; }

    uint32_t getTokenValidity() const { return mTokenValidityPeriod; }
    int32_t getTokenTimeLeft() const;
    std::string getTokenString() const { return mOAuth2Token; }
    int32_t getRequestTimeout() const { return mRequestTimeout; }
    int32_t getConnectionTimeout() const { return mConnectionTimeout; }

    void requestOAuth2token( int32_t timeout_msec );

    Json::Value requestLicense( const Json::Value& json_req, int32_t timeout_msec );
    Json::Value requestHealth( const Json::Value& json_req, int32_t timeout_msec );

};

}
}

#endif // _H_ACCELIZE_DRM_WS_CLIENT
