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

#ifndef _H_ACCELIZE_COMMON_LIBEXPORT
#define _H_ACCELIZE_COMMON_LIBEXPORT

#ifdef  __cplusplus
#ifdef BUILDING_DRMLIB
    #define DRMLIB_EXPORT __attribute__((visibility("default")))
    #define DRMLIB_LOCAL __attribute__((visibility("hidden")))
#else
    #define DRMLIB_EXPORT
    #define DRMLIB_LOCAL
#endif /* BUILDING_DRMLIB */
#else
    #define DRMLIB_EXPORT
    #define DRMLIB_LOCAL
#endif /* __cplusplus */

#endif /* _H_ACCELIZE_COMMON_LIBEXPORT */
