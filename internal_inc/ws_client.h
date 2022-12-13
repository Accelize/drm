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

#ifndef _H_ACCELIZE_DRM_WS_CLIENT
#define _H_ACCELIZE_DRM_WS_CLIENT

#include <string>
#include <list>
#include <chrono>
#include <json/json.h>
#include <curl/curl.h>

#include "log.h"
#include "utils.h"


namespace Accelize {
namespace DRM {

typedef std::chrono::steady_clock TClock; /// Shortcut type def to steady clock which is monotonic (so unaffected by clock adjustments)
typedef enum class tHttpRequestType: uint8_t { GET, POST, PUT, PATCH } tHttpRequestType;


// RAII for Curl global init/cleanup
class CurlSingleton {

private:
    CurlSingleton() { curl_global_init( CURL_GLOBAL_ALL ); }
    ~CurlSingleton() { curl_global_cleanup(); }

public:
    CurlSingleton( const CurlSingleton& ) = delete;
    CurlSingleton& operator=( const CurlSingleton& ) = delete;

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

public:
    static bool is_error_retryable( long resp_code ) {
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
        if ( ( http_resp_code >= 200 ) && ( http_resp_code < 400 ) )
            return DRM_OK;
        if ( CurlEasyPost::is_error_retryable( http_resp_code ) )
            return DRM_WSMayRetry;
        if ( ( http_resp_code >= 400 ) && ( http_resp_code < 500 ) )
            return DRM_WSReqError;
        return DRM_WSError;
    }

    CurlEasyPost( const uint32_t& connection_timeout_ms );
    ~CurlEasyPost();

    double getTotalTime();  // Get total time in seconds of the previous transfer

    void setVerbosity( const uint32_t verbosity );
    void setHostResolves( const Json::Value& host_json );

    void appendHeader( const std::string header );
    void setPostFields( const std::string postfields = "" );

    std::string escape( const std::string str ) const;

    std::string request( const std::string& suburl, const tHttpRequestType& type, const int32_t& timeout_ms );

protected:

    static size_t curl_write_callback( void *contents, size_t size, size_t nmemb, std::string *userp ) {
        size_t realsize = size * nmemb;
        try {
            userp->append( (const char*)contents, realsize );
        } catch( const std::bad_alloc& e ) {  //LCOV_EXCL_LINE
            Throw( DRM_ExternFail, "Curl write callback exception: {}", e.what() );  //LCOV_EXCL_LINE
        }
        return realsize;
    }

    static size_t curl_header_callback( void *contents, size_t size, size_t nitems, std::string *userp ) {
        size_t realsize = size * nitems;
        try {
            std::string header(reinterpret_cast<char*>(contents), realsize);
            size_t request_id_str = header.find("x-request-id");
            if ( std::string::npos != request_id_str ) {
                *userp = rtrim(header);
            }
        } catch( const std::bad_alloc& e ) {  //LCOV_EXCL_LINE
            Throw( DRM_ExternFail, "Curl header callback exception: {}", e.what() );  //LCOV_EXCL_LINE
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

    uint32_t mVerbosity;
    std::string mClientId;
    std::string mClientSecret;
    std::string mOAuth2Token;
    std::string mUrl;
    Json::Value mHostResolvesJson;
    uint32_t mTokenValidityPeriod;              /// Validation period of the OAuth2 token in seconds
    uint32_t mTokenExpirationMargin;            /// OAuth2 token expiration margin in seconds
    TClock::time_point mTokenExpirationTime;    /// OAuth2 expiration time
    std::string mTokenFilePath;                 /// Path to a cache file used to save the token
    int32_t mRequestTimeoutMS;                  /// Maximum period in milliseconds for a request to complete
    int32_t mConnectionTimeoutMS;               /// Maximum period in milliseconds for the client to connect the server

    bool isTokenValid() const;

public:

    DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path);
    ~DrmWSClient() = default;

    std::string getUrl() const { return mUrl; }
    uint32_t getVerbosity() const { return mVerbosity; }
    uint32_t getTokenValidity() const { return mTokenValidityPeriod; }
    int32_t getTokenTimeLeft() const;
    std::string getTokenString() const { return mOAuth2Token; }
    int32_t getRequestTimeoutMS() const { return mRequestTimeoutMS; }
    int32_t getConnectionTimeoutMS() const { return mConnectionTimeoutMS; }

    void setUrl( std::string& url ) { mUrl = url; }

    std::string escape( std::string str ) const;
    void getOAuth2token( int32_t timeout_msec );
    Json::Value sendSaasRequest( const std::string &url, const tHttpRequestType &type,
                                 const Json::Value& json_req, int32_t timeout_msec ) const;
};

}
}

#endif // _H_ACCELIZE_DRM_WS_CLIENT
