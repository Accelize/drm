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
std::string getHomeDir();
std::string getDirName( const std::string& full_path );
bool isDir( const std::string& dir_path );
std::vector<std::string> listDir( const std::string& path );
std::string readFile( const std::string& path );
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
std::string rtrim(const std::string &s);

// Encode/decode  in base32
/*
    Base32 encoding / decoding.
    Encode32 outputs at out bytes with values from 0 to 32 that can be mapped to 32 signs.
    Decode32 input is the output of Encode32. The out parameters should be unsigned char[] of
    length GetDecode32Length(inLen) and GetEncode32Length(inLen) respectively.
    To map the output of Encode32 to an alphabet of 32 characters use Map32.
    To unmap back the output of Map32 to an array understood by Decode32 use Unmap32.
    Both Map32 and Unmap32 do inplace modification of the inout32 array.
    The alpha32 array must be exactly 32 chars long.
*/
struct Base32
{
    static bool Decode32(unsigned char* in, int inLen, unsigned char* out);
    static bool Encode32(unsigned char* in, int inLen, unsigned char* out);

    static int  GetDecode32Length(int bytes);
    static int  GetEncode32Length(int bytes);

    static bool Map32(unsigned char* inout32, int inout32Len, unsigned char* alpha32);
    static bool Unmap32(unsigned char* inout32, int inout32Len, unsigned char* alpha32);
};



}
}

#endif // _H_ACCELIZE_METERING_UTILS

