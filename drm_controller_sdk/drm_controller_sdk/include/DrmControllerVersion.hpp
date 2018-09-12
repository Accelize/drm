/**
*  \file      DrmControllerVersion.hpp
*  \version   3.0.0.1
*  \date      September 2018
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

#include <iostream>
#include <string>
#include <sstream>

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
    public :

      /** DrmControllerVersion
      *   \brief Class constructor.
      **/
      DrmControllerVersion();

      /** ~DrmControllerVersion
      *   \brief Class destructor.
      **/
      ~DrmControllerVersion();

      /** checkVersion
      *   \brief Check hardware version matches software version.
      *   \param[in] hardwareVersion is the version extracted from the hardware
      *              in the format "Major.Minor.Bug".
      *   \return Returns mDrmApi_NO_ERROR when hardware and SDK version match,
      *                   mDrmApi_VERSION_CHECK_ERROR otherwise.
      *   \throw DrmControllerVersionCheckException whenever an error occured. DrmControllerVersionCheckException::what()
      *          should be called to get the exception description.
      */
      static unsigned int checkVersion(const std::string &hardwareVersion);

      /** getVersionElements
      *   \brief Get each elements of the version (major, minor, bug).
      *   \param[in] version is the version to get the elements from,
      *              in the format "Major.Minor.Bug".
      *   \param[out] major is the value of the major version.
      *   \param[out] minor is the value of the minor version.
      *   \param[out] bug is the value of the bug version.
      */
      static void getVersionElements(const std::string &version,
                                     std::string &major,
                                     std::string &minor,
                                     std::string &bug);

      /** getSoftwareVersion
      *   \brief Get the software version value.
      *   \return Returns the version of this API, in the format "Major.Minor.Bug".
      */
      static std::string getSoftwareVersion();

    // protected members, functions ...
    protected :

    // private members, functions ...
    private :
      static const std::string mDrmSoftwareVersion; /**<Version value of this API, in the format "Major.Minor.Bug".**/

  }; // class DrmControllerVersion

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_VERSION_HPP__
