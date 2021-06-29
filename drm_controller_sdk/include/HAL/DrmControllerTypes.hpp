/**
*  \file      DrmControllerTypes.hpp
*  \version   6.0.0.0
*  \date      April 2021
*  \brief     Types definitions.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/


#ifndef __DRM_CONTROLLER_TYPES_HPP__
#define __DRM_CONTROLLER_TYPES_HPP__

#include <functional>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /** \typedef tDrmReadRegisterFunction
  *   \brief   Read register function prototype.
  *   \remark  The read register function shall return 0 for no error.
  **/
  typedef std::function<unsigned int(const std::string&, unsigned int&)> tDrmReadRegisterFunction;

  /** \typedef tDrmWriteRegisterFunction
  *   \brief   Write register function prototype.
  *   \remark  The write register function shall return 0 for no error.
  **/
  typedef std::function<unsigned int(const std::string&, unsigned int)>  tDrmWriteRegisterFunction;

} // DrmControllerLibrary

#endif // __DRM_CONTROLLER_TYPES_HPP__
