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
#include <jsoncpp/json/json.h>

namespace Accelize {
namespace DRM {

#define RETROCOMPATIBLITY_LIMIT_MAJOR     3
#define RETROCOMPATIBLITY_LIMIT_MINOR     1

static const Json::Value nullJSValue = Json::Value{};

const Json::Value& JVgetRequired(const Json::Value& jval, const char* key, const Json::ValueType& type);
const Json::Value& JVgetOptional(const Json::Value& jval, const char* key, const Json::ValueType& type, const Json::Value& defaultValue = nullJSValue);

}
}

#endif // _H_ACCELIZE_METERING_UTILS
