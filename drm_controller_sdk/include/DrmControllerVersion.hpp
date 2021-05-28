/**
*  \file      DrmControllerVersion.hpp
*  \version   6.0.1.0
*  \date      May 2021
*  \brief     Class DrmControllerVersion defines procedures
*             for hardware versus SDK versions checks.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_VERSION_HPP__
#define __DRM_CONTROLLER_VERSION_HPP__

#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <DrmControllerVersionCheckException.hpp>
#include <DrmControllerCommon.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerVersion DrmControllerVersion.hpp "include/DrmControllerVersion.hpp"
  *   \brief    Class DrmControllerVersion defines procedures
  *             for hardware versus SDK versions checks.
  **/
  class DrmControllerVersion {

    // public members, functions ...
    public:

      /** DrmControllerVersion
      *   \brief Class constructor.
      **/
      DrmControllerVersion();

      /** ~DrmControllerVersion
      *   \brief Class destructor.
      **/
      ~DrmControllerVersion();

      /** getVersionElements
      *   \brief Get each elements of the version (major, minor, bug).
      *   \param[in] version is the version to get the elements from,
      *              in the format "Major.Minor.Bug".
      *   \param[out] major is the value of the major version.
      *   \param[out] minor is the value of the minor version.
      *   \param[out] bug is the value of the bug version.
      */
      static void getVersionElements(const std::string &version, std::string &major, std::string &minor, std::string &bug);

    // protected members, functions ...
    protected:

    // private members, functions ...
    private:

  }; // class DrmControllerVersion

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_VERSION_HPP__
