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

/** \brief Accelize CSP Metadata Management
*/

#ifndef _H_ACCELIZE_CSP_MANAGER
#define _H_ACCELIZE_CSP_MANAGER

#include <json/json.h>

#include "ws_client.h"


namespace Accelize {
namespace DRM {


/** \brief Interface to collect CSP metadata
*/
class CspBase {

private:
    std::string mName;  // CSP name
    CurlEasyPost mHTTPRequest;

public:

    CspBase() = delete; //!< No default constructor
    CspBase( const std::string &name, const uint32_t timeout_ms );
    virtual ~CspBase() {};

    std::string getName() const { return mName; }

    /** \brief Get Metadata information

        \param[in] resume_session_request : If true, the pending session is
        reused. If no pending session is found, create a new one. If
        false and a pending session is found, close it and create a new
        one. Default to False.
    */
    virtual Json::Value get_metadata() = 0;

    static CspBase *make_csp();
};


/** \brief Interface to collect AWS metadata
*/
class Aws : public CspBase {
public:
    Aws();
    //~Aws() {};
    Json::Value get_metadata();
};


/** \brief Interface to collect Alibaba metadata
*/
class Alibaba : public CspBase {
public:
    Alibaba();
    //~Alibaba() {};
    Json::Value get_metadata();
};


}
}

#endif // _H_ACCELIZE_CSP_MANAGER
