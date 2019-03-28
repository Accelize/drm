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

#include <fstream>

#include "utils.h"
#include "log.h"

namespace Accelize {
namespace DRM {


std::ostream& operator<<(std::ostream& os, const Json::ValueType& type) {
    switch(type) {
        case Json::nullValue    : os << "nullValue"; break;
        case Json::intValue     : os << "Integer"; break;
        case Json::uintValue    : os << "Unsigned Integer"; break;
        case Json::realValue    : os << "Real"; break;
        case Json::stringValue  : os << "String"; break;
        case Json::booleanValue : os << "Boolean"; break;
        case Json::arrayValue   : os << "Array"; break;
        case Json::objectValue  : os << "Object"; break;
        default: Unreachable( "Unsupported Jsoncpp ValueType" );
    }
    return os;
}


std::string saveJsonToString( const Json::Value& json_value, const std::string& indent ) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = indent;
    return Json::writeString( builder, json_value );
}


void saveJsonToFile( const std::string& file_path, const Json::Value& json_value, const std::string& indent ) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = indent;
    std::unique_ptr<Json::StreamWriter> writer( builder.newStreamWriter() );
    std::ofstream ofs( file_path );

    if ( !ofs.is_open() )
        Throw( DRM_ExternFail, "Unable to access file: ", file_path );
    if ( writer->write( json_value, &ofs ) )
        Throw(DRM_ExternFail, "Unable to write file: ", file_path);
    if ( !ofs.good())
        Throw(DRM_ExternFail, "Unable to write file: ", file_path);
    ofs.close();
}


Json::Value parseJsonString(const std::string &json_string) {
    Json::Value json_node;
    std::string parseErr;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> const reader(builder.newCharReader() );
    
    if ( json_string.size() == 0 )
        Throw( DRM_BadArg, "Cannot parse an empty JSON string" );
    if ( !reader->parse( json_string.c_str(), json_string.c_str() + json_string.size(), &json_node, &parseErr) )
        Throw( DRM_BadFormat, "Cannot parse following JSON string because ", parseErr, "\n", json_string );
    return json_node;
}


Json::Value parseJsonFile( const std::string& file_path ) {
    Json::Value json_node;
    std::string parseErr;
    Json::CharReaderBuilder builder;
    std::ifstream fh( file_path );

    if ( !fh.good() )
        Throw( DRM_BadArg, "Cannot find JSON file: ", file_path );
    bool ret = Json::parseFromStream( builder, fh, &json_node, &parseErr);
    fh.close();
    if ( !ret  )
        Throw( DRM_BadFormat, "Cannot parse ", file_path, " : ", parseErr );
    if ( json_node.empty() )
        Throw( DRM_BadArg, "JSON file '", file_path, "' is empty" );

    return json_node;
}


const Json::Value& JVgetRequired(const Json::Value& jval, const char* key, const Json::ValueType& type) {
    if (!jval.isMember(key))
        Throw(DRM_BadFormat, "Missing parameter '", key, "' of type ", type);
    const Json::Value& jvalmember = jval[key];
    Debug( "Found parameter '", key, "' of type ", jvalmember.type());
    if (jvalmember.type()!=type && !jvalmember.isConvertibleTo(type))
        Throw(DRM_BadFormat, "Wrong parameter type for '", key, "' = ", jvalmember, ", expecting ", type, ", parsed as ", jvalmember.type());
    if (jvalmember.empty())
        Throw(DRM_BadFormat, "Value of parameter '", key, "' is empty");
    if ( ( jvalmember.type() == Json::stringValue ) && ( jvalmember.asString().empty() ) )
        Throw(DRM_BadFormat, "Value of parameter '", key, "' is an empty string");
    return jvalmember;
}


const Json::Value& JVgetOptional(const Json::Value& jval, const char* key, const Json::ValueType& type, const Json::Value& defaultValue) {
    bool exists = jval.isMember(key);
    const Json::Value& jvalmember = exists ? jval[key] : defaultValue;
    if (exists || !jvalmember.isNull())
        Debug( "Found parameter '", key, "' of type ", jvalmember.type());
        if (jvalmember.type()!=type && !jvalmember.isConvertibleTo(type))
            Throw(DRM_BadFormat, "Wrong parameter type for '", key, "' = ", jvalmember, ", expecting ", type, ", parsed as ", jvalmember.type());
    return jvalmember;
}

}
}
