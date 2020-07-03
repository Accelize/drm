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

#ifndef _H_ACCELIZE_COMMON_EXPORT
#define _H_ACCELIZE_COMMON_EXPORT

    #ifdef  __cplusplus
        #ifdef BUILDING_DRMLIB
            #define DRM_EXPORT __attribute__((visibility("default")))
            #define DRM_LOCAL __attribute__((visibility("hidden")))
        #else
            #define DRM_EXPORT
            #define DRM_LOCAL
        #endif /* BUILDING_DRMLIB */
    #else
        #define DRM_EXPORT
        #define DRM_LOCAL
    #endif /* __cplusplus */


    #define Throw( errcode, ... ) do {                                              \
        Accelize::DRM::Exception except( errcode, fmt::format( __VA_ARGS__ ) );     \
        if ( errcode == DRM_Exit )                                                  \
            Debug( __VA_ARGS__ );                                                   \
        else if ( ( errcode == DRM_WSTimedOut ) || ( errcode == DRM_WSMayRetry ) )  \
            Warning( __VA_ARGS__ );                                                 \
        else                                                                        \
            Error( __VA_ARGS__ );                                                   \
        throw except;                                                               \
    } while(0)


    #define Unreachable( ... ) \
        Throw( DRM_Assert, "Reached an unexpected part of code: " + fmt::format(__VA_ARGS__ ) + "Please contact support." );

#endif /* _H_ACCELIZE_COMMON_EXPORT */
