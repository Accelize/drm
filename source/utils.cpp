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
#include <ctime>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#endif

#include "utils.h"
#include "log.h"


namespace Accelize {
namespace DRM {


std::string getDirName( const std::string& full_path ) {

    size_t i = full_path.rfind( PATH_SEP, full_path.length() );
    if ( i != std::string::npos ) {
        return full_path.substr( 0, i );
    }
    return std::string( "" );
}

bool isDir( const std::string& dir_path ) {
#if defined(_WIN32)
    struct _stat info;
    if ( _stat( dir_path.c_str(), &info ) != 0 )
        return false;
    return ( info.st_mode & _S_IFDIR ) != 0;
#else
    struct stat info;
    if ( stat( dir_path.c_str(), &info ) != 0 )
        return false;
    return ( info.st_mode & S_IFDIR ) != 0;
#endif
}


bool isFile( const std::string& file_path ) {
#if defined(_WIN32)
    struct _stat info;
    if ( _stat( file_path.c_str(), &info ) != 0 )
        return false;
    return ( info.st_mode & _S_IFDIR ) != 0;
#else
    struct stat info;
    if ( stat( file_path.c_str(), &info ) != 0 )
        return false;
    return ( info.st_mode & S_IFDIR ) == 0;
#endif
}


bool makeDirs( const std::string& dir_path, mode_t mode ) {
    std::string path = dir_path;
    std::string dir;
    size_t pos = 0;
    int ret;

    if ( dir_path.size() == 0 ) {
        return true;
    }

    if ( isDir( dir_path ) ) {
        Debug( "Directory '{}' already exists", dir_path );
        return true;
    }

    if ( path[ path.size() - 1 ] != PATH_SEP ) {
        // Force trailing '/' so we can handle everything in loop
        path += PATH_SEP;
    }

    while( ( pos = path.find_first_of( PATH_SEP, pos ) ) != std::string::npos ) {
        dir = path.substr( 0, pos++ );
        if ( ( dir.size() == 0 ) || ( isDir( dir ) ) )
            continue; // if leading / first time is 0 length
#if defined(_WIN32)
        ret = _mkdir( dir.c_str() );
#else
        ret = mkdir( dir.c_str(), mode );
#endif
        if ( ret ) {
            Error( "Failed to create directory {}: {}", dir, strerror( errno ) );
            return false;
        }
    }
    Debug( "Created directory '{}'", dir_path );
    return isDir( dir_path );
}


std::string typeToString( const Json::ValueType& type ) {
    std::string sType;
    switch( type ) {
        case Json::nullValue    : sType = std::string( "nullValue" ); break;
        case Json::intValue     : sType = std::string( "Integer" ); break;
        case Json::uintValue    : sType = std::string( "Unsigned Integer" ); break;
        case Json::realValue    : sType = std::string( "Real" ); break;
        case Json::stringValue  : sType = std::string( "String" ); break;
        case Json::booleanValue : sType = std::string( "Boolean" ); break;
        case Json::arrayValue   : sType = std::string( "Array" ); break;
        case Json::objectValue  : sType = std::string( "Object" ); break;
        default: Unreachable( "Unsupported Jsoncpp ValueType" ); //LCOV_EXCL_LINE
    }
    return sType;
}


std::string saveJsonToString( const Json::Value& json_value, const std::string& indent ) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = indent;
    return Json::writeString( builder, json_value );
}


void saveJsonToFile( const std::string& file_path,
        const Json::Value& json_value,
        const std::string& indent ) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = indent;
    std::unique_ptr<Json::StreamWriter> writer( builder.newStreamWriter() );
    std::ofstream ofs( file_path );

    if ( !ofs.is_open() )
        Throw( DRM_ExternFail, "Unable to access file: {}", file_path );
    if ( writer->write( json_value, &ofs ) )
        Throw( DRM_ExternFail, "Unable to write file: {}", file_path );
    if ( !ofs.good() )
        Throw( DRM_ExternFail, "Unable to write file: {}", file_path );
    ofs.close();
}


Json::Value parseJsonString( const std::string &json_string ) {
    Json::Value json_node;
    std::string parseErr;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> const reader( builder.newCharReader() );

    if ( json_string.size() == 0 )
        Throw( DRM_BadFormat, "Cannot parse an empty JSON string" );

    if ( !reader->parse( json_string.c_str(), json_string.c_str() + json_string.size(),
            &json_node, &parseErr) )
        Throw( DRM_BadFormat, "Cannot parse JSON string because {}", parseErr );

    if ( json_node.empty() || json_node.isNull() )
        Throw( DRM_BadArg, "JSON string is empty" );

    Debug2( "Extracted JSON Object: {}", json_node.toStyledString() );

    return json_node;
}


Json::Value parseJsonFile( const std::string& file_path ) {
    Json::Value json_node;
    std::string file_content;

    // Check path is a file
    if ( !isFile(file_path) ) {
        Throw( DRM_BadArg, "Path is not a valid file: {}", file_path );
    }

    // Open file
    std::ifstream fh( file_path );
    if ( !fh.good() )
        Throw( DRM_BadArg, "Cannot find JSON file: {}", file_path );
    // Read file content
    fh.seekg( 0, std::ios::end );
    file_content.reserve( fh.tellg() );
    fh.seekg( 0, std::ios::beg );
    file_content.assign( (std::istreambuf_iterator<char>(fh)), std::istreambuf_iterator<char>() );
    // Parse content as a JSON object
    try {
        json_node = parseJsonString( file_content );
    } catch( const Exception& e ) {
        Throw( e.getErrCode(), "Cannot parse JSON file {}: {}", file_path, e.what() );
    }
    Debug("Found and loaded JSON file: {}", file_path);

    return json_node;
}


const Json::Value& JVgetRequired( const Json::Value& jval, const char* key, const Json::ValueType& type ) {
    if ( !jval.isMember( key ) )
        Throw( DRM_BadFormat, "Missing parameter '{}' of type {}", key, typeToString( type ) );

    const Json::Value& jvalmember = jval[key];

    if ( ( jvalmember.type() != type ) && !jvalmember.isConvertibleTo( type ) )
        Throw( DRM_BadFormat, "Wrong parameter type for '{}' = {}, expecting {}, parsed as {}",
              key, jvalmember.toStyledString(), typeToString( type ), typeToString( jvalmember.type() ) );

    if ( jvalmember.empty() )
        Throw( DRM_BadFormat, "Value of parameter '{}' is empty", key );

    if ( ( jvalmember.type() == Json::stringValue ) && ( jvalmember.asString().empty() ) )
        Throw( DRM_BadFormat, "Value of parameter '{}' is an empty string", key );

    std::string val = jvalmember.toStyledString();
    val = val.erase( val.find_last_not_of("\t\n\v\f\r") + 1 );
    Debug( "Found parameter '{}' of type {} with value {}", key, typeToString( jvalmember.type() ), val );

    return jvalmember;
}


const Json::Value& JVgetOptional( const Json::Value& jval, const char* key, const Json::ValueType& type,
                                  const Json::Value& defaultValue ) {
    bool exists = jval.isMember( key );
    const Json::Value& jvalmember = exists ? jval[key] : defaultValue;

    if ( jvalmember.type()!=type && !jvalmember.isConvertibleTo(type) )
        Throw( DRM_BadFormat, "Wrong parameter type for '{}' = {}, expecting {}, parsed as {}",
                key, jvalmember.toStyledString(), typeToString( type ), typeToString( jvalmember.type() ) );

    std::string val = jvalmember.toStyledString();
    val = val.erase( val.find_last_not_of("\t\n\v\f\r") + 1 );

    if ( exists )
        Debug( "Found parameter '{}' of type {}: return its value {}", key, typeToString( jvalmember.type() ), val );
    else
        Debug( "Could not find parameter '{}' of type {}: return default value {}", key, typeToString( jvalmember.type() ), val );

    return jvalmember;
}


std::string time_t_to_string( const time_t &t ) {
    std::string str = std::string( asctime( std::localtime( &t ) ) );
    str.pop_back();
    return str;
}


time_t steady_clock_to_time_t( const std::chrono::steady_clock::time_point& tp ) {
    return std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() +
                std::chrono::duration_cast<std::chrono::system_clock::duration>( tp - std::chrono::steady_clock::now() ) );
}


std::chrono::steady_clock::time_point time_t_to_steady_clock( const time_t& t ) {
    return std::chrono::steady_clock::now() + (std::chrono::system_clock::from_time_t( t ) - std::chrono::system_clock::now());
}


std::string execCmd( const std::string& cmd ) {
    std::array<char, 128> buffer;
    std::string result;
    Debug( "Running command: {}", cmd );
    std::unique_ptr<FILE, decltype(&pclose)> pipe( popen( cmd.c_str(), "r" ), pclose );
    if ( !pipe ) {
        throw std::runtime_error( "popen() failed!" );
    }
    while( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr ) {
        result += buffer.data();
    }
    return result;
}


std::string toUpHex( const uint64_t& i ) {
    std::stringstream stream;
    stream << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << i;
    return stream.str();
}


std::vector<std::string> split(const std::string& str, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(str);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}


}
}
