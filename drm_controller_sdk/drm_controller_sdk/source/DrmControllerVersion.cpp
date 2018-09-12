/**
*  \file      DrmControllerVersion.cpp
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

#include <DrmControllerVersion.hpp>

// Static members initialization
const std::string DrmControllerLibrary::DrmControllerVersion::mDrmSoftwareVersion = DRM_CONTROLLER_VERSION; /**<Version value of this API, in the format "Major.Minor.Bug".**/

/** DrmControllerVersion
*   \brief Class constructor.
**/
DrmControllerLibrary::DrmControllerVersion::DrmControllerVersion() { }

/** ~DrmControllerVersion
*   \brief Class destructor.
**/
DrmControllerLibrary::DrmControllerVersion::~DrmControllerVersion() { }

/** checkVersion
*   \brief Check hardware version matches software version.
*   \param[in] hardwareVersion is the version extracted from the hardware
*              in the format "Major.Minor.Bug".
*   \return Returns mDrmApi_NO_ERROR when hardware and SDK version match,
*                   mDrmApi_VERSION_CHECK_ERROR otherwise.
*   \throw DrmControllerVersionCheckException whenever an error occured. DrmControllerVersionCheckException::what()
*          should be called to get the exception description.
*/
unsigned int DrmControllerLibrary::DrmControllerVersion::checkVersion(const std::string &hardwareVersion) {
#ifndef VERSION_CHECK_DISABLED
  std::string hardwareMajor, hardwareMinor, hardwareBug;
  std::string softwareMajor, softwareMinor, softwareBug;
  DrmControllerVersion::getVersionElements(hardwareVersion, hardwareMajor, hardwareMinor, hardwareBug);
  DrmControllerVersion::getVersionElements(DrmControllerVersion::mDrmSoftwareVersion, softwareMajor, softwareMinor, softwareBug);
  // check version
  if (hardwareMajor != softwareMajor || hardwareMinor != softwareMinor) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_VERSION_CHECK_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_VERSION_CHECK_ERROR_HARDWARE_VERSION << hardwareVersion << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_VERSION_CHECK_ERROR_SOFTWARE_VERSION << DrmControllerVersion::mDrmSoftwareVersion << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_VERSION_CHECK_ERROR_FOOTER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerLibrary::DrmControllerVersionCheckException(stringStream.str());
    // return version error
    return mDrmApi_VERSION_CHECK_ERROR;
  }
#endif
  // return no error
  return mDrmApi_NO_ERROR;
}

/** getVersionElements
*   \brief Get each elements of the version (major, minor, bug).
*   \param[in] version is the version to get the elements from,
*              in the format "Major.Minor.Bug".
*   \param[out] major is the value of the major version.
*   \param[out] minor is the value of the minor version.
*   \param[out] bug is the value of the bug version.
*/
void DrmControllerLibrary::DrmControllerVersion::getVersionElements(const std::string &version,
                                                               std::string &major,
                                                               std::string &minor,
                                                               std::string &bug) {
  size_t start = 0;
  size_t end = 0;
  for (unsigned int ii = 0; ii < DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT; ii++) {
    // find the '.'
    end = version.find('.', start+1);
    switch (ii) {
      // get major version
      case 0 :
        major = version.substr(start, end-start);
        break;
      // get minor version
      case 1 :
        minor = version.substr(start+1, end-start-1);
        break;
      // get bug version
      case 2 :
        bug = version.substr(start+1);
        break;
      // do nothing
      default :
        break;
    }
    // find the '.'
    start = version.find('.', end);
  }
}

/** getSoftwareVersion
*   \brief Get the software version value.
*   \return Returns the version of this API, in the format "Major.Minor.Bug".
*/
std::string DrmControllerLibrary::DrmControllerVersion::getSoftwareVersion() {
  return DrmControllerVersion::mDrmSoftwareVersion;
}
