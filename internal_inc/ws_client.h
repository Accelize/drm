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

//#include "log.h"

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
    CURL *curl;
    struct curl_slist *headers = nullptr;
    std::list<std::string> data; // keep data until request performed
    std::array<char, CURL_ERROR_SIZE> errbuff;

public:

    static bool is_error_retryable(long resp_code) {
        return        resp_code == 408 // Request Timeout
                   || resp_code == 500 // Internal Server Error
                   || resp_code == 502 // Bad Gateway
                   || resp_code == 503 // Service Unavailable
                   || resp_code == 504 // Gateway timeout
                   || resp_code == 505 // HTTP version not supported
                   || resp_code == 507 // Insufficient storage
                   || resp_code == 429 // Too Many Requests
                   || resp_code == 520 // Unknown Error
                   || resp_code == 521 // Web Server is Down
                   || resp_code == 522 // Connection Timed Out
                   || resp_code == 524 // A Timeout Occurred
                   || resp_code == 525 // SSL Handshake Failed
                   || resp_code == 527 // Railgun Error
                   || resp_code == 530 // Origin DNS Error
                   || resp_code == 470 // Floating License: no token available
                   || resp_code == 560 // Accelize License generation temporary issue
                ;
    }

    CurlEasyPost();
    ~CurlEasyPost();

    long perform(std::string* resp, std::chrono::steady_clock::time_point deadline);
    double getTotalTime();

    template<class T>
    void setURL(T&& url) {
        data.push_back(std::forward<T>(url));
        curl_easy_setopt(curl, CURLOPT_URL, data.back().c_str());
    }

    template<class T>
    void appendHeader(T&& header) {
        data.push_back(std::forward<T>(header));
        headers = curl_slist_append(headers, data.back().c_str());
    }

    template<class T>
    void setPostFields(T&& postfields) {
        data.push_back(std::forward<T>(postfields));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.back().c_str());
    }

protected:

    static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
        auto *s = (std::string*)userp;
        size_t realsize = size * nmemb;
        s->append((const char*)contents, realsize);
        return realsize;
    }
};


/*DRM Web Service client : communcates with Accelize DRM web server to
 get license and send metering data*/
class DrmWSClient {

protected:

    typedef std::chrono::steady_clock TClock; /// Shortcut type def to steady clock which is monotonic (so unaffected by clock adjustments)

    std::string mClientId;
    std::string mClientSecret;
    std::string mOAuth2Url;
    std::string mMeteringUrl;
    std::string mOAuth2Token;
    uint32_t mTokenValidityPeriod;
    TClock::time_point mTokenExpirationTime;
    CurlEasyPost mOAUth2Request;

public:
    DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path);
    ~DrmWSClient() = default;

    uint32_t getTokenValidity() const { return mTokenValidityPeriod; }
    uint32_t getTokenTimeLeft() const;
    std::string getTokenString() const { return mOAuth2Token; }

    void setOAuth2token( const std::string& token );

    void requestOAuth2token(TClock::time_point deadline);
    Json::Value requestLicense( const Json::Value& json_req, TClock::time_point deadline );

};

}
}

#endif // _H_ACCELIZE_DRM_WS_CLIENT
