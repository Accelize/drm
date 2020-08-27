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

#include "csp.h"
#include "utils.h"
#include "log.h"


namespace Accelize {
namespace DRM {


/** AWS class
*/
Aws::Aws():CspBase("Aws") {}

Json::Value Aws::get_metadata() {
    Json::Value metadata = Json::nullValue;
    // Using IMDSv2 method
    // Get token
    std::string cmd = "curl -s -X PUT \"http://169.254.169.254/latest/api/token\" -H \"X-aws-ec2-metadata-token-ttl-seconds: 21600\"";
    std::string token = exec_cmd( cmd );
    // Create base command
    std::string base_cmd = fmt::format( "curl -s -H \"X-aws-ec2-metadata-token: {}\" http://169.254.169.254/latest", token);
    // Get metatdata
    metadata["instance_id"] = exec_cmd( fmt::format( "{}/meta-data/instance-id", base_cmd ) );
    metadata["instance_type"] = exec_cmd( fmt::format( "{}/meta-data/instance-type", base_cmd ) );
    metadata["ami_id"] = exec_cmd( fmt::format( "{}/meta-data/ami-id", base_cmd ) );
    metadata["region"] = exec_cmd( fmt::format( "{}/dynamic/instance-identity/document | grep -oP '\"region\"[[:space:]]*:[[:space:]]*\"\\K[^\"]+'", base_cmd ) );
    return metadata;
}


/** Alibaba class
*/
Alibaba::Alibaba():CspBase("Alibaba") {}

Json::Value Alibaba::get_metadata() {
    Json::Value metadata = Json::nullValue;
    // Create base command
    std::string base_cmd( "curl -s http://100.100.100.200/latest/meta-data" );
    // Append commands to list
    metadata["instance_id"] = exec_cmd( fmt::format( "{}/instance-id", base_cmd ) );
    metadata["instance_type"] = exec_cmd( fmt::format( "{}/instance/instance-type", base_cmd ) );
    metadata["ami_id"] = exec_cmd( fmt::format( "{}/image-id", base_cmd ) );
    metadata["region"] = exec_cmd( fmt::format( "{}/region-id", base_cmd ) );
    return metadata;
}


/** CspBase class
*/
CspBase *CspBase::make_csp() {
    //  Test AWS command
    Aws* csp_aws = new Aws();
    try {
        csp_aws->get_metadata();
        Debug( "Instance is running on Aws" );
        return csp_aws;
    } catch( std::runtime_error &e ) {
        Debug2( "Instance is not running on Aws" );
        delete csp_aws;
    }
    //  Test Alibaba command
    Alibaba* csp_alibaba = new Alibaba();
    try {
        csp_alibaba->get_metadata();
        Debug( "Instance is running on Alibaba" );
        return csp_alibaba;
    } catch( std::runtime_error &e ) {
        Debug2( "Instance is not running on Alibaba" );
        delete csp_alibaba;
    }
    //  Not a supported CSP or this is On-Prem system
    Debug( "Cloud environment could not be determined" );
    return nullptr;
}

}
}