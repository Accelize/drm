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

Json::Value GetCspInfo( uint32_t verbosity ) {
    Json::Value info_node = Json::nullValue;

    //  Test AWS command
    Aws* csp_aws = new Aws();
    try {
        csp_aws->setVerbosity( verbosity );
        info_node = csp_aws->get_metadata();
        Debug( "Instance is running on Aws" );
        delete csp_aws;
        return info_node;
    } catch( std::runtime_error &e ) {
        Debug( "Instance is not running on Aws" );
        delete csp_aws;
    }

    //  Test Alibaba command
    Alibaba* csp_alibaba = new Alibaba();
    try {
        csp_alibaba->setVerbosity( verbosity );
        info_node = csp_alibaba->get_metadata();
        Debug( "Instance is running on Alibaba" );
        delete csp_alibaba;
        return info_node;
    } catch( std::runtime_error &e ) {
        Debug( "Instance is not running on Alibaba" );
        delete csp_alibaba;
    }

    //  Not a supported CSP or this is On-Prem system
    Debug( "Cloud environment could not be determined" );
    return info_node;
}


/** AWS class
*/
Aws::Aws():CspBase( "Aws", 50 ) {}

Json::Value Aws::get_metadata() {
    Json::Value metadata = Json::nullValue;
    std::string token;
    uint32_t timeout_ms = mHTTPRequest.getConnectionTimeoutMS();

    // Using IMDSv2 method

    // Get token
    CurlEasyPost tokenReq;
    tokenReq.setConnectionTimeoutMS( timeout_ms );
    tokenReq.setVerbosity( mVerbosity );
    tokenReq.appendHeader( "X-aws-ec2-metadata-token-ttl-seconds: 21600" );
    token = tokenReq.perform_put( "http://169.254.169.254/latest/api/token", timeout_ms );

    // Collect AWS information
    std::string header = fmt::format("X-aws-ec2-metadata-token: {}", token);
    mHTTPRequest.appendHeader( header );
    std::string base_url("http://169.254.169.254/latest");
    metadata["instance_id"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/meta-data/instance-id", base_url ), timeout_ms );
    metadata["instance_type"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/meta-data/instance-type", base_url ), timeout_ms );
    metadata["ami_id"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/meta-data/ami-id", base_url ), timeout_ms );
    std::string doc_string = mHTTPRequest.perform<std::string>( fmt::format( "{}/dynamic/instance-identity/document", base_url ), timeout_ms );
    Json::Value doc_json = parseJsonString( doc_string );
    metadata["region"] = doc_json["region"];
    return metadata;
}


/** Alibaba class
*/
Alibaba::Alibaba():CspBase( "Alibaba", 50 ) {}

Json::Value Alibaba::get_metadata() {
    Json::Value metadata = Json::nullValue;
    uint32_t timeout_ms = mHTTPRequest.getConnectionTimeoutMS();
    // Collect Alibaba information
    std::string base_url("http://100.100.100.200/latest/meta-data");
    metadata["instance_id"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/instance-id", base_url ), timeout_ms );
    metadata["instance_type"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/instance/instance-type", base_url ), timeout_ms );
    metadata["ami_id"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/image-id", base_url ), timeout_ms );
    metadata["region"] = mHTTPRequest.perform<std::string>( fmt::format( "{}/region-id", base_url ), timeout_ms );
    return metadata;
}


/** CspBase class
*/
CspBase::CspBase( const std::string &name, const uint32_t timeout_ms ) {
    mName = name;
    mHTTPRequest.setConnectionTimeoutMS( timeout_ms );
    mVerbosity = 0;
}


}
}
