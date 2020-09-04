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
    static void Init() {
        static CurlSingleton g_curl;
        (void) g_curl;
    }

};


// RAII for Curl easy
class CurlEasyPost {

private:
    const uint32_t cConnectionTimeoutMS = 30000;    // Timeout default value in milliseconds

    CURL *curl = NULL;
    struct curl_slist *mHeaders_p = NULL;
    struct curl_slist *mHostResolveList = NULL;
    std::list<std::string> data;                    // keep data until request performed
    std::array<char, CURL_ERROR_SIZE> mErrBuff;
    uint32_t mConnectionTimeoutMS;                  // Request timeout in milliseconds

public:

    static bool is_error_retryable(long resp_code) {
        return        resp_code == 408 // Request Timeout
                   || resp_code == 429 // Too Many Requests
                   || resp_code == 470 // Floating License: no token available
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
                   || resp_code == 527 // Railgun Error
                   || resp_code == 530 // Origin DNS Error
                   || resp_code == 560 // Accelize License generation temporary issue
                ;
    }

    CurlEasyPost();
    ~CurlEasyPost();

    long perform( std::string* resp, std::chrono::steady_clock::time_point& deadline );
    long perform( std::string* resp, std::chrono::milliseconds& timeout );
    long perform_put( std::string* resp, std::string url, const uint32_t& timeout_ms );

    template<class T>
    T perform( std::string url, const uint32_t& timeout_ms ) {
        T resp;

        // Configure and execute CURL command
        curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
        if ( mHeaders_p ) {
            curl_easy_setopt( curl, CURLOPT_HTTPHEADER, mHeaders_p );
        }
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::write_callback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &resp );
        curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, mErrBuff.data() );
        curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
        curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT_MS, mConnectionTimeoutMS );
        if ( timeout_ms <= 0 )
            Throw( DRM_WSTimedOut, "Did not perform HTTP request to Accelize webservice because deadline is reached." );
        curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, timeout_ms );
        CURLcode res = curl_easy_perform( curl );

        // Analyze HTTP answer
        long resp_code;
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &resp_code );
        Debug( "Received code {} from {} in {} ms", resp_code, url, getTotalTime() * 1000 );

        if ( res != CURLE_OK ) {
            if ( res == CURLE_COULDNT_RESOLVE_PROXY
              || res == CURLE_COULDNT_RESOLVE_HOST
              || res == CURLE_COULDNT_CONNECT
              || res == CURLE_OPERATION_TIMEDOUT ) {
                Throw( DRM_WSMayRetry, "Failed performing HTTP request to Accelize webservice ({}) : {}",
                        curl_easy_strerror( res ), mErrBuff.data() );
            } else {
                Throw( DRM_ExternFail, "Failed performing HTTP request to Accelize webservice ({}) : {}",
                        curl_easy_strerror( res ), mErrBuff.data() );
            }
        }
        return resp;
    }

    double getTotalTime();

    void setVerbosity( const uint32_t verbosity );

    void setHostResolves( const Json::Value& host_json );

    template<class T>
    void setURL(T&& url) {
        data.push_back( std::forward<T>(url) );
        curl_easy_setopt( curl, CURLOPT_URL, data.back().c_str() );
    }

    template<class T>
    void appendHeader( T&& header ) {
        data.push_back( std::forward<T>(header) );
        Debug2( "Add {} to CURL header", std::forward<T>(header) );
        mHeaders_p = curl_slist_append( mHeaders_p, data.back().c_str() );
    }

    template<class T>
    void setPostFields( T&& postfields ) {
        data.push_back( std::forward<T>(postfields) );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, data.back().size() );
        curl_easy_setopt( curl, CURLOPT_POSTFIELDS, data.back().c_str() );
    }

    void setConnectionTimeoutMS( const uint32_t timeoutMS ) { mConnectionTimeoutMS = timeoutMS; }
    uint32_t getConnectionTimeoutMS() const { return mConnectionTimeoutMS; }

protected:

    static size_t write_callback( void *contents, size_t size, size_t nmemb, std::string *userp ) {
        size_t realsize = size * nmemb;
        try {
            userp->append( (const char*)contents, realsize );
        } catch( const std::bad_alloc& e ) {
            Throw( DRM_ExternFail, "Curl write callback exception: {}", e.what() );
        }
        return realsize;
    }

};


/*DRM Web Service client : communcates with Accelize DRM web server to
 get license and send metering data*/
class DrmWSClient {

    const uint32_t cTokenExpirationMargin = 30;  // In seconds
    const uint32_t cRequestTimeout = 30;         // In seconds

protected:

    typedef std::chrono::steady_clock TClock; /// Shortcut type def to steady clock which is monotonic (so unaffected by clock adjustments)

    uint32_t mVerbosity;
    std::string mClientId;
    std::string mClientSecret;
    std::string mOAuth2Token;
    std::string mLicenseUrl;
    std::string mHealthUrl;
    Json::Value mHostResolvesJson;
    uint32_t mTokenValidityPeriod;              /// Validation period of the OAuth2 token in seconds
    uint32_t mTokenExpirationMargin;            /// OAuth2 token expiration margin in seconds
    TClock::time_point mTokenExpirationTime;    /// OAuth2 expiration time
    CurlEasyPost mOAUth2Request;
    uint32_t mRequestTimeout;

    bool isTokenValid() const;
    Json::Value requestMetering( const std::string url, const Json::Value& json_req, TClock::time_point deadline );

public:
    DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path);
    ~DrmWSClient() = default;

    uint32_t getVerbosity() const { return mVerbosity; }

    uint32_t getTokenValidity() const { return mTokenValidityPeriod; }
    int32_t getTokenTimeLeft() const;
    std::string getTokenString() const { return mOAuth2Token; }

    void requestOAuth2token(TClock::time_point deadline);

    Json::Value requestLicense( const Json::Value& json_req, TClock::time_point deadline );
    Json::Value requestHealth( const Json::Value& json_req, TClock::time_point deadline );

};

}
}

#endif // _H_ACCELIZE_DRM_WS_CLIENT
