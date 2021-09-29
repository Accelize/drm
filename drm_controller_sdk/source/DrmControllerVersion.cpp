/**
*  \file      DrmControllerVersion.cpp
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

#include <DrmControllerVersion.hpp>

// namespace usage
using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** DrmControllerVersion
*   \brief Class constructor.
**/
DrmControllerVersion::DrmControllerVersion() { }

/** ~DrmControllerVersion
*   \brief Class destructor.
**/
DrmControllerVersion::~DrmControllerVersion() { }

/** getVersionElements
*   \brief Get each elements of the version (major, minor, bug).
*   \param[in] version is the version to get the elements from,
*              in the format "Major.Minor.Bug".
*   \param[out] major is the value of the major version.
*   \param[out] minor is the value of the minor version.
*   \param[out] bug is the value of the bug version.
*/
void DrmControllerVersion::getVersionElements(const std::string &version, std::string &major, std::string &minor, std::string &bug) {
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
