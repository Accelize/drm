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

#ifndef _H_ACCELIZE_METERING_UTILS
#define _H_ACCELIZE_METERING_UTILS

#include <iostream>
#include <chrono>
#include <json/json.h>

namespace Accelize {
namespace DRM {

const char PATH_SEP =
#ifdef _WIN32
                     '\\';
#else
                     '/';
#endif

// Path related functions
std::string getDirName( const std::string& full_path );
bool isDir( const std::string& dir_path );
bool isFile( const std::string& file_path );
bool removeFile( const std::string& file_path );
bool makeDirs( const std::string& dir_path, mode_t mode = 744 );

// JSON related functions
std::string saveJsonToString( const Json::Value& json_value, const std::string& indent = "" );
void saveJsonToFile( const std::string& file_path, const Json::Value& json_value, const std::string& indent = "\t" );
Json::Value parseJsonString(const std::string &json_string);
Json::Value parseJsonFile(const std::string &file_path);
const Json::Value& JVgetRequired( const Json::Value& json_value, const char* key, const Json::ValueType& type );
const Json::Value& JVgetOptional( const Json::Value& json_value, const char* key, const Json::ValueType& type, const Json::Value& defaultValue = Json::nullValue );

// Time related functions
std::string time_t_to_string( const time_t &t );
time_t steady_clock_to_time_t( const std::chrono::steady_clock::time_point& tp );
std::chrono::steady_clock::time_point time_t_to_steady_clock( const time_t& t );

// Miscellaneous functions
std::string execCmd( const std::string& cmd );
std::string toUpHex( const uint64_t& i );
std::vector<std::string> splitByDelimiter( const std::string& str, char delimiter );
std::vector<std::string> splitByLength( const std::string& str, uint32_t splitLength );
uint64_t str2int64( std::string num_str );

}
}

#endif // _H_ACCELIZE_METERING_UTILS

