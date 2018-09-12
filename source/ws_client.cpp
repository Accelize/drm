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

#include "log.h"
#include "utils.h"
#include "ws_client.h"

namespace Accelize {
namespace DRMLib {

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
    struct curl_slist *headers = NULL;
    std::list<std::string> data; // keep data until request performed
    std::array<char, CURL_ERROR_SIZE> errbuff;

public:
    CurlEasyPost(bool verbose = false) {
        curl = curl_easy_init();
        if(!curl)
            Throw(DRMExternFail, "Curl : cannot init curl_easy");
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

    long perform(std::string* resp) {
        CURLcode res;
        long resp_code;
        if(headers)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlEasyPost::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)resp);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff.data());
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            Throw(DRMExternFail, "Failed performing HTTP request to Accelize webservice (", curl_easy_strerror(res), ") : ", errbuff.data());
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
        return resp_code;
    }

    double getTotalTime() {
        double ret;
        if(!curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &ret))
            return ret;
        else
            return 0;
    }

protected:
    static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
        std::string *s = (std::string*)userp;
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

MeteringWSClient::MeteringWSClient(const Json::Value& config_server, const Json::Value& credentials) {
    auto getConfig = [](const Json::Value& json, const char * key, std::string* dest) {
        *dest = JVgetRequired(json, key, Json::stringValue).asString();
    };
    getConfig(config_server, "oauth2_url", &oauth2_url);
    getConfig(config_server, "metering_url", &metering_url);
    getConfig(credentials, "client_id", &client_id);
    getConfig(credentials, "client_secret", &client_secret);

    CurlSingleton::Init();
}

Json::Value MeteringWSClient::getLicense(const Json::Value& json_req) {
    std::string token = getOAuth2token();

    CurlEasyPost req;

    // Set headers of request
    req.setURL(metering_url);
    req.appendHeader("Accept: application/json");
    req.appendHeader("Content-Type: application/json");
    req.appendHeader(std::string("Authorization: Bearer ") + token);

    // Set data of request
    Json::FastWriter json_writer;
    req.setPostFields(json_writer.write(json_req));

    // Perform
    std::string resp;
    long resp_code = req.perform(&resp);
    if(resp_code != 200 && resp_code != 400)
        Throw(DRMWSReqError, "WS HTTP response code : ", resp_code, "(", resp, ")");

    Debug("License response obtained in ", req.getTotalTime()*1000, "ms");

    // Parse response
    Json::Reader reader;
    Json::Value json_resp;
    reader.parse(resp, json_resp);

    // Check for error
    if(json_resp.get("error", "true").asBool() || resp_code==400) {
        Throw(DRMWSError, "Error from WS with details : ", json_resp.get("detail", "Unknown error"));
    }

    return json_resp;
}

std::string MeteringWSClient::getOAuth2token() {
    //Request token OAuth2
    CurlEasyPost req;
    req.setURL(oauth2_url);
    std::stringstream ss;
    ss << "client_id=" << client_id << "&client_secret=" << client_secret << "&grant_type=client_credentials";
    req.setPostFields(ss.str());

    std::string resp;
    long resp_code = req.perform(&resp);
    if(resp_code != 200)
        Throw(DRMWSReqError, "WSOAuth HTTP response code : ", resp_code, "(", resp, ")");

    //Parse response
    Json::Reader reader;
    Json::Value json_resp;
    reader.parse(resp, json_resp);
    if(!json_resp.isMember("access_token"))
        Throw(DRMWSRespError, "Non-valid response from WSOAuth : ", resp);

    Debug("OAuth2 token ", json_resp["access_token"].asString(), " obtained in ", req.getTotalTime()*1000, "ms");

    return json_resp["access_token"].asString();
}

}
}
