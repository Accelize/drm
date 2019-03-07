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
#include <list>
#include <curl/curl.h>
#include <chrono>

#include "log.h"
#include "utils.h"
#include "ws_client.h"

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
    CurlEasyPost( bool verbose = false ) {
        curl = curl_easy_init();
        if(!curl)
            Throw(DRM_ExternFail, "Curl : cannot init curl_easy");
        if(verbose)
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
    }

    ~CurlEasyPost() {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

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

    long perform(std::string* resp, std::chrono::steady_clock::time_point deadline) {
        CURLcode res;
        long resp_code;
        if(headers)
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

    double getTotalTime() {
        double ret;
        if(!curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ret))
            return ret;
        else
            return 0.0;
    }

    static bool is_error_retryable(long resp_code) {
        return     resp_code == 408 /* Request Timeout */
                || resp_code == 500 /* Internal Server Error */
                || resp_code == 502 /* Bad Gateway */
                || resp_code == 503 /* Service Unavailable */
                || resp_code == 504 /* Gateway timeout */
                || resp_code == 505 /* HTTP version not supported */
                || resp_code == 507 /* Insufficient strorage */
                || resp_code == 429 /* Too Many Requests  */
                || resp_code == 520 /* Unknown Error */
                || resp_code == 521 /* Web Server is Down */
                || resp_code == 522 /* Connection Timed Out */
                || resp_code == 524 /* A Timeout Occured */
                || resp_code == 525 /* SSL Handshake Failed */
                || resp_code == 527 /* Railgun Error */
                || resp_code == 530 /* Origin DNS Error */
                || resp_code == 404 /* Not Found Error */
                || resp_code == 470 /* Floating License: no token available */
                ;
    }

protected:
    static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
        auto *s = (std::string*)userp;
        size_t realsize = size * nmemb;
        s->append((const char*)contents, realsize);
        return realsize;
    }

    static int debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
        (void)handle; (void)userptr;

        std::string msg(data, size);
        std::string msg_head;

        switch (type) {
            case CURLINFO_TEXT:
                msg_head = "Curl info : ";
                break;
            case CURLINFO_HEADER_OUT:
                msg_head = "=> Send header";
                break;
            case CURLINFO_DATA_OUT:
                msg_head = "=> Send data";
                break;
            case CURLINFO_SSL_DATA_OUT:
                msg_head = "=> Send SSL data";
                break;
            case CURLINFO_HEADER_IN:
                msg_head = "<= Recv header";
                break;
            case CURLINFO_DATA_IN:
                msg_head = "<= Recv data";
                break;
            case CURLINFO_SSL_DATA_IN:
                msg_head = "<= Recv SSL data";
                break;
            default: /* in case a new one is introduced to shock us */
                return 0;
        }
         Debug(msg_head, msg);
         return 0;
    }
};

DrmWSClient::DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path) {

    Json::Value conf_json = parseJsonFile(conf_file_path);
    Json::Value cred_json = parseJsonFile(cred_file_path);

    mUseBadOAuth2Token = std::string("");

    try {
        Json::Value webservice_json = JVgetRequired(conf_json, "licensing", Json::objectValue);
        Debug2("Web service configuration: ", webservice_json.toStyledString());

        std::string url = JVgetRequired(webservice_json, "url", Json::stringValue).asString();
        mOAuth2Url = url + std::string("/o/token/");
        mMeteringUrl = url + std::string("/auth/metering/genlicense/");
        Debug("Licensing URL: ", url);

        mDefaultRequestTimeout = std::chrono::seconds( JVgetOptional(webservice_json, "default_ws_request_timeout", Json::uintValue, 20).asUInt() );
        Debug2("mDefaultRequestTimeout=", mDefaultRequestTimeout.count());

    } catch(Exception &e) {
        if (e.getErrCode() != DRM_BadFormat)
            throw;
        Throw(DRM_BadFormat, "Error in service configuration file '", conf_file_path, "': ", e.what());
    }

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
}

Json::Value DrmWSClient::getLicense(const Json::Value& json_req) {
    return getLicense(json_req, std::chrono::steady_clock::now() + mDefaultRequestTimeout);
}

Json::Value DrmWSClient::getLicense(const Json::Value& json_req, std::chrono::steady_clock::time_point deadline) {

    std::string token = mUseBadOAuth2Token;
    if ( mUseBadOAuth2Token.empty() ) {
        Debug("Requesting new token");
        token = getOAuth2token(deadline);
    } else {
        Debug("Overwriting token with: ", mUseBadOAuth2Token);
        mUseBadOAuth2Token.clear();
        Debug("NEw token with: ", mUseBadOAuth2Token);
    }

    Debug("Web Service JSON request:\n", json_req.toStyledString());

    CurlEasyPost req;

    // Set headers of request
    req.setURL(mMeteringUrl);
    req.appendHeader("Accept: application/json");
    req.appendHeader("Content-Type: application/json");
    req.appendHeader(std::string("Authorization: Bearer ") + token);

    // Set data of request
    req.setPostFields( saveJsonToString( json_req ) );

    // Perform
    Debug("Starting license request");
    std::string resp;
    long resp_code = req.perform(&resp, deadline);
    Debug("Received code = ", resp_code);

    if ( (resp_code != 200) && (resp_code != 560) && (resp_code != 400) ) {
         if(CurlEasyPost::is_error_retryable(resp_code)) {
            Throw(DRM_WSMayRetry, "WS HTTP response code : ", resp_code, "(", resp, ")");
        } else {
            Throw(DRM_WSReqError, "WS HTTP response code : ", resp_code, "(", resp, ")");
        }
    }

    Debug("License response obtained in ", req.getTotalTime()*1000, "ms");

    // Parse response
    Json::Value json_resp = parseJsonString(resp);
    Debug( "Web Service JSON response:\n", json_resp.toStyledString() );

    // Check for error with details
    if(resp_code == 560) { /*560 : Custom License generation temporary issue*/
       Assert(json_resp.get("error", "true").asBool());
       Throw(DRM_WSMayRetry, "Error from WS with details : ", json_resp.get("detail", "Unknown error"));
    } else if(resp_code == 400) { /*400 : Bad request */
       Assert(json_resp.get("error", "true").asBool());
       Throw(DRM_WSError, "Error from WS with details : ", json_resp.get("detail", "Unknown error"));
    }

    return json_resp;
}

std::string DrmWSClient::getOAuth2token(std::chrono::steady_clock::time_point deadline) {

    CurlEasyPost req;

    // Set headers of request
    req.setURL( mOAuth2Url );
    std::stringstream ss;
    ss << "client_id=" << mClientId << "&client_secret=" << mClientSecret << "&grant_type=client_credentials";
    req.setPostFields(ss.str());

    Debug( "Starting OAuth2 token request to url: ", mOAuth2Url );

    std::string resp;
    long resp_code = req.perform(&resp, deadline);
    if(resp_code != 200) {
        if(CurlEasyPost::is_error_retryable(resp_code)) {
            Throw(DRM_WSMayRetry, "WSOAuth HTTP response code : ", resp_code, "(", resp, ")");
        } else {
            Throw(DRM_WSReqError, "WSOAuth HTTP response code : ", resp_code, "(", resp, ")");
        }
    }

    //Parse response
    Json::Value json_resp = parseJsonString(resp);
    if ( !json_resp.isMember("access_token") )
        Throw(DRM_WSRespError, "Non-valid response from WSOAuth : ", resp);

    Debug("OAuth2 token ", json_resp["access_token"].asString(), " obtained in ", req.getTotalTime()*1000, "ms");

    return json_resp["access_token"].asString();
}

}
}
