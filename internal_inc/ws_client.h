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
#include <json/json.h>
#include <chrono>

namespace Accelize {
namespace DRM {

/*DRM Web Service client : communcates with Accelize DRM web server to
 get license and send metering data*/
class DrmWSClient {
protected:
    std::string mClientId;
    std::string mClientSecret;
    std::string mOAuth2Url;
    std::string mMeteringUrl;
    std::chrono::seconds mDefaultRequestTimeout;

    // Test only parameters
    std::string mUseBadOAuth2Token;

public:
    DrmWSClient(const std::string &conf_file_path, const std::string &cred_file_path);
    ~DrmWSClient() = default;

    Json::Value getLicense(const Json::Value& json_req);
    Json::Value getLicense(const Json::Value& json_req, std::chrono::steady_clock::time_point deadline);

    inline void useBadOAuth2Token( const std::string& new_token ) { mUseBadOAuth2Token = new_token; };

protected:
    std::string getOAuth2token(std::chrono::steady_clock::time_point deadline);
    void parse_configuration(const std::string &conf_file_path, Json::Reader &reader, Json::Value &conf_json);

};

}
}

#endif // _H_ACCELIZE_DRM_WS_CLIENT
