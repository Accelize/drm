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

/** \brief Accelize CSP Metadata Management
*/

#ifndef _H_ACCELIZE_CSP_MANAGER
#define _H_ACCELIZE_CSP_MANAGER

#include <json/json.h>

#include "ws_client.h"


namespace Accelize {
namespace DRM {


void GetCspInfo( Json::Value &json_info, uint32_t verbosity );


/** \brief Interface to collect CSP metadata
*/
class Csp {

private:
    int32_t mConnectionTimeoutMS; // Connection timeout in milliseconds
    int32_t mRequestTimeoutMS; // Request timeout in milliseconds
    uint32_t mVerbosity; // HTTP verbosity: 0: default, 1:verbose

public:

    Csp() = delete; //!< No default constructor
    Csp( const uint32_t verbosity = 0, const int32_t connection_timeout_ms = 50,
         const int32_t request_timeout_ms = 50 );
    ~Csp() {};

    uint32_t getVerbosity() const { return mVerbosity; }
    void setVerbosity( const uint32_t& verbosity ) { mVerbosity = verbosity; }

    void get_metadata_ec2( Json::Value &csp_info ) const;
    void get_metadata_openstack( Json::Value &csp_info ) const;
    bool get_metadata_aws(Json::Value &csp_info) const;
    bool get_metadata_vmaccel(Json::Value &csp_info) const;
    bool get_metadata_alibaba(Json::Value &csp_info) const;
    bool get_metadata_azure(Json::Value &csp_info) const;
};


}
}

#endif // _H_ACCELIZE_CSP_MANAGER
