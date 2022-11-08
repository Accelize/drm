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

#include "csp.h"
#include "utils.h"
#include "log.h"


namespace Accelize {
namespace DRM {


Json::Value GetCspInfo( uint32_t verbosity ) {
    Json::Value csp_info = Json::nullValue;
    std::unique_ptr<Csp> csp_obj( new Csp(verbosity, 2000, 2000) );
    // Try EC2 metadata service
    csp_obj->get_metadata_ec2( csp_info );
    // Try OpenStack metadata service
    csp_obj->get_metadata_openstack( csp_info );
    // Determine CSP name
    if ( csp_obj->get_metadata_aws( csp_info ) ) {
        return csp_info;
    }
    if ( csp_obj->get_metadata_azure( csp_info ) ) {
        return csp_info;
    }
    if ( csp_obj->get_metadata_vmaccel( csp_info ) ) {
        return csp_info;
    }
    if ( csp_obj->get_metadata_alibaba( csp_info ) ) {
        return csp_info;
    }
    if ( csp_obj->get_metadata_azure( csp_info ) ) {
        return csp_info;
    }
    //  Not a supported CSP or this is On-Prem system
    Debug( "Could not determined CSP" );
    return csp_info;
}


/** Csp class
*/
Csp::Csp( const uint32_t verbosity, const int32_t connection_timeout_ms,
          const int32_t request_timeout_ms ) {
    mVerbosity = verbosity;
    mConnectionTimeoutMS = connection_timeout_ms;
    mRequestTimeoutMS = request_timeout_ms;
}


void Csp::get_metadata_ec2( Json::Value &csp_info ) const {
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    std::string base_url("http://169.254.169.254/latest/meta-data");
    std::string result_str;
    Json::Value result_json;
    try {
        result_str = req.perform_get<std::string>( base_url, mRequestTimeoutMS );
        Debug( "List of available EC2 instance metadata:\n{}", result_str );
        csp_info["instance_provider"] = "Unknown EC2";
        csp_info["instance_image"] = req.perform_get<std::string>( fmt::format( "{}/ami-id", base_url ), mRequestTimeoutMS );
        csp_info["instance_type"] = req.perform_get<std::string>( fmt::format( "{}/instance-type", base_url ), mRequestTimeoutMS );
        csp_info["extra"]["hostname"] = req.perform_get<std::string>( fmt::format( "{}/hostname", base_url ), mRequestTimeoutMS );
    } catch(std::runtime_error const&) {
        Debug( "Could not access instance metadata using EC2 url" );
    }
}

void Csp::get_metadata_openstack( Json::Value &csp_info ) const {
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    std::string base_url("http://169.254.169.254/openstack/latest/meta_data.json");
    std::string result_str;
    Json::Value result_json;
    try {
        result_str = req.perform_get<std::string>( base_url, mRequestTimeoutMS );
        result_json = parseJsonString( result_str );
        Debug( "List of available openstack instance metadata:\n{}", result_json.toStyledString() );
        csp_info["instance_provider"] = "Unknown OpenStack";
        //csp_info["openstack"] = Json::nullValue;
        csp_info["extra"]["hostname"] = result_json["hostname"];
        csp_info["extra"]["availability_zone"] = result_json["availability_zone"];
        csp_info["extra"]["devices"] = result_json["devices"];
        csp_info["extra"]["dedicated_cpus"] = result_json["dedicated_cpus"];
    } catch(std::runtime_error const&) {
        Debug( "Could not access instance metadata using OpenStack url" );
    }
}


// Check and collect AWS instance metadata
bool Csp::get_metadata_aws(Json::Value &csp_info) const {
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    Json::Value result_json = Json::nullValue;
    try {
        std::string result_str = req.perform_get<std::string>( "http://169.254.169.254/latest/meta-data/services/domain", mRequestTimeoutMS );
        if ( result_str.find( "aws" ) != std::string::npos) {
            csp_info["instance_provider"] = "AWS";
            csp_info["instance_region"] = result_json["region"];
            // Add AWS specific fields
            result_str = req.perform_get<std::string>( "http://169.254.169.254/latest/dynamic/instance-identity/document", mRequestTimeoutMS );
            result_json = parseJsonString( result_str );
            csp_info["extra"]["architecture"] = result_json["architecture"];
            csp_info["extra"]["version"] = result_json["version"];
            Debug( "Instance is running on AWS" );
            return true;
        }
    } catch(std::runtime_error const&) {
        Debug( "Could not extract AWS metadata" );
    }
    return false;
}


// Check and collect VMAccel instance metadata
bool Csp::get_metadata_vmaccel(Json::Value &csp_info) const {
    if ( csp_info.isMember( "hostname" ) ) {
        std::string hostname = csp_info["hostname"].asString();
        if ( hostname.find( "vmaccel" ) ) {
            csp_info["instance_provider"] = "VMAccel";
            Debug( "Instance is running on VMAccel" );
            return true;
        }
    }
    return false;
}


// Check and collect Alibaba instance metadata
bool Csp::get_metadata_alibaba(Json::Value &csp_info) const {
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    std::string base_url = std::string("http://100.100.100.200/latest/meta-data");
    try {
        csp_info["instance_provider"] = "Alibaba";
        csp_info["instance_type"] = req.perform_get<std::string>( fmt::format( "{}/instance/instance-type", base_url ), mRequestTimeoutMS );
        csp_info["instance_image"] = req.perform_get<std::string>( fmt::format( "{}/image-id", base_url ), mRequestTimeoutMS );
        csp_info["instance_region"] = req.perform_get<std::string>( fmt::format( "{}/region-id", base_url ), mRequestTimeoutMS );
        csp_info["extra"]["hostname"] = req.perform_get<std::string>( fmt::format( "{}/hostname", base_url ), mRequestTimeoutMS );
        csp_info["extra"]["product-code"] = req.perform_get<std::string>( fmt::format( "{}/image/market-place/product-code", base_url ), mRequestTimeoutMS );
        csp_info["extra"]["charge-type"] = req.perform_get<std::string>( fmt::format( "{}/image/market-place/charge-type", base_url ), mRequestTimeoutMS );
        csp_info["extra"]["zone-id"] = req.perform_get<std::string>( fmt::format( "{}/zone-id", base_url ), mRequestTimeoutMS );
        Debug( "Instance is running on Alibaba" );
        return true;
    } catch(std::runtime_error const&) {}
    return false;
}


// Check and collect Azure instance metadata
bool Csp::get_metadata_azure(Json::Value &csp_info) const {
    CurlEasyPost req( mConnectionTimeoutMS );
    req.setVerbosity( mVerbosity );
    req.appendHeader( "Metadata:true" );
    Json::Value result_json = Json::nullValue;
    try {
        std::string result_str = req.perform_get<std::string>( "http://169.254.169.254/metadata/instance?api-version=2021-02-01", mRequestTimeoutMS );
        if ( result_str.find( "AzurePublicCloud" ) != std::string::npos ) {
            csp_info["instance_provider"] = "Azure";
            result_json = parseJsonString( result_str );
            csp_info["instance_image"] = result_json["compute"]["storageProfile"]["imageReference"];
            csp_info["instance_region"] = result_json["compute"]["location"];
            csp_info["extra"]["extendedLocation"] = result_json["compute"]["extendedLocation"];
            csp_info["extra"]["osType"] = result_json["compute"]["osType"];
            csp_info["extra"]["plan"] = result_json["compute"]["plan"];
            csp_info["extra"]["vmSize"] = result_json["compute"]["vmSize"];
            csp_info["extra"]["zone"] = result_json["compute"]["zone"];
            Debug( "Instance is running on Azure" );
            return true;
        }
    } catch(std::runtime_error const&) {}
    return false;
}


}
}
