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

#ifndef _H_ACCELIZE_METERING_UTILS
#define _H_ACCELIZE_METERING_UTILS

#include <iostream>
#include <json/json.h>

namespace Accelize {
namespace DRM {

const char path_separator =
#ifdef _WIN32
                     '\\';
#else
                     '/';
#endif

std::string getDirName( const std::string& full_path );
bool isDir( const std::string& dir_path );
bool isFile( const std::string& file_path );
bool makeDirs( const std::string& dir_path, mode_t mode = 744 );

std::string saveJsonToString( const Json::Value& json_value, const std::string& indent = "" );
void saveJsonToFile( const std::string& file_path, const Json::Value& json_value, const std::string& indent = "\t" );
Json::Value parseJsonString(const std::string &json_string);
Json::Value parseJsonFile(const std::string &file_path);
const Json::Value& JVgetRequired( const Json::Value& json_value, const char* key, const Json::ValueType& type );
const Json::Value& JVgetOptional( const Json::Value& json_value, const char* key, const Json::ValueType& type, const Json::Value& defaultValue = Json::nullValue );

std::string exec_cmd( const char* cmd);
}
}

#endif // _H_ACCELIZE_METERING_UTILS

