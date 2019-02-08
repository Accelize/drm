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

const Json::Value& JVgetRequired(const Json::Value& jval, const char* key, const Json::ValueType& type) {
    if (!jval.isMember(key))
        Throw(DRM_BadFormat, "Missing parameter '", key, "' of type ", type);

    const Json::Value& jvalmember = jval[key];

    if (jvalmember.type()!=type && !jvalmember.isConvertibleTo(type))
        Throw(DRM_BadFormat, "Wrong parameter type for '", key, "' = ", jvalmember, ", expecting ", type, ", parsed as ", jvalmember.type());

    if (jvalmember.empty())
        Throw(DRM_BadFormat, "Value of parameter '", key, "' is empty");

    return jvalmember;
}

const Json::Value& JVgetOptional(const Json::Value& jval, const char* key, const Json::ValueType& type, const Json::Value& defaultValue) {
    bool exists = jval.isMember(key);
    const Json::Value& jvalmember = exists ? jval[key] : defaultValue;
    if (exists || !jvalmember.isNull())
        if (jvalmember.type()!=type && !jvalmember.isConvertibleTo(type))
            Throw(DRM_BadFormat, "Wrong parameter type for '", key, "' = ", jvalmember, ", expecting ", type, ", parsed as ", jvalmember.type());

    return jvalmember;
}

}
}
