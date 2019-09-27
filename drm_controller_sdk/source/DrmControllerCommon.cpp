/**
*  \file      DrmControllerCommon.cpp
*  \version   4.0.0.1
*  \date      July 2019
*  \brief     This source file contains the common definitions.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#include <DrmControllerCommon.hpp>

/**
*   \var  drmLogLevelString
*   \brief Variable assignment for log level strings.
**/
const char *drmLogLevelString[] = {  DRM_CONTROLLER_LOG_DEBUG_STRING,
                                     DRM_CONTROLLER_LOG_INFO_STRING,
                                     DRM_CONTROLLER_LOG_WARNING_STRING,
                                     DRM_CONTROLLER_LOG_ERROR_STRING,
                                     DRM_CONTROLLER_LOG_NONE_STRING
                                   };

