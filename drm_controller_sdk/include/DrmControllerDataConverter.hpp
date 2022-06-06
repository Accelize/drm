/**
*  \file      DrmControllerDataConverter.hpp
*  \version   8.0.0.0
*  \date      March 2022
*  \brief     Class DrmControllerDataConverter provides base functions for data conversion.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_DATA_CONVERTER_HPP__
#define __DRM_CONTROLLER_DATA_CONVERTER_HPP__

#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include <algorithm>

#include <DrmControllerCommon.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerDataConverter DrmControllerDataConverter.hpp "include/DrmControllerDataConverter.hpp"
  *   \brief    Class DrmControllerDataConverter provides base functions for data conversion.
  **/
  class DrmControllerDataConverter {

    // public members, functions ...
    public:

      /** hexStringToBinary
      *   \brief Convert a hexadecimal representation into a list of binary values.
      *   \param[in] hexString is the hexadecimal representation.
      *   \return Returns the list of binary values.
      **/
      static const std::vector<unsigned int> hexStringToBinary(const std::string &hexString);

      /** binaryToHexString
      *   \brief Convert a list of binary values into a hexadecimal representation.
      *   \param[in] binary is the list of binary values.
      *   \return Returns the hexadecimal representation.
      **/
      static const std::string binaryToHexString(const std::vector<unsigned int> &binary);

      /** hexStringListToBinary
      *   \brief Convert a list of list of hexadecimal string representation into a binary values.
      *   \param[in] hexString is the list of hexadecimal string representation.
      *   \return Returns the list of binary values.
      **/
      static const std::vector<unsigned int> hexStringListToBinary(const std::vector<std::string> &hexString);

      /** binaryToHexString
      *   \brief Convert a binary value into a hexadecimal representation.
      *   \param[in] binary is the binary value.
      *   \return Returns the hexadecimal representation.
      **/
      static const std::string binaryToHexString(const unsigned int &binary);

      /** binaryToHexStringList
      *   \brief Convert a list of binary values into a list of hexadecimal string representation.
      *   \param[in] binary is the binary value.
      *   \param[in] wordsNumber is the number of words to push per string.
      *   \return Returns the list of hexadecimal string representation.
      **/
      static const std::vector<std::string> binaryToHexStringList(const std::vector<unsigned int> &binary, const unsigned int &wordsNumber);

      /** binaryToVersionString
      *   \brief Convert a binary value into a formated string for the version where each digit
      *   are separated by a dot. The string is in the form major.minor.correction.
      *   \param[in] binary is the binary value.
      *   \return Returns the formated version.
      **/
      static const std::string binaryToVersionString(const unsigned int &binary);

      /** binStringToBinary
      *   \brief Convert a binary representation into a list of binary values.
      *   \param[in] binString is the binary representation.
      *   \return Returns the list of binary values.
      **/
      static const std::vector<unsigned int> binStringToBinary(const std::string &binString);

      /** binaryToBinString
      *   \brief Convert a list of binary values into a binary representation.
      *   \param[in] binary is the list of binary values.
      *   \return Returns the binary representation.
      **/
      static const std::string binaryToBinString(const std::vector<unsigned int> &binary);

      /** binaryToBinString
      *   \brief Convert a binary value into a binary representation.
      *   \param[in] binary is binary value.
      *   \return Returns the binary representation.
      **/
      static const std::string binaryToBinString(const unsigned int &binary);

      /** hexStringToBinString
      *   \brief Convert a hexadecimal representation into a binary representation.
      *   \param[in] hexString is the hexadecimal representation.
      *   \return Returns the binary representation.
      **/
      static const std::string hexStringToBinString(const std::string &hexString);

      /** binStringToHexString
      *   \brief Convert a binary representation into a hexadecimal representation.
      *   \param[in] binString is the binary representation.
      *   \return Returns the hexadecimal representation.
      **/
      static const std::string binStringToHexString(const std::string &binString);

      /** asciiStringToBinary
      *   \brief Convert an ASCII string into a list of binary values.
      *   \param[in] asciiString is the ASCII string.
      *   \param[in] asciiStringLen is the length of the ascii string.
      *   \return Returns a list of binary values.
      **/
      static const std::vector<unsigned int> asciiStringToBinary(const unsigned char *asciiString, const unsigned int &asciiStringLen);

      /** binaryToAsciiString
      *   \brief Convert a list of binary values into an ASCII string.
      *   \param[in] binary is the list of binary values.
      *   \param[out] asciiStringLen is the length of the ascii string.
      *   \return Returns the ASCII string.
      **/
      static unsigned char* binaryToAsciiString(const std::vector<unsigned int> &binary, unsigned int &asciiStringLen);

      /** base64ToBinary
      *   \brief Convert a base64 encoded string into a list of binary values.
      *   \param[in] base64 is the base64 encoded string.
      *   \return Returns the list of binary values.
      **/
      static const std::vector<unsigned int> base64ToBinary(const std::string &base64);

      /** binaryToBase64
      *   \brief Convert a list of binary values into a base64 encoded string.
      *   \param[in] binary is the list of binary values.
      *   \return Returns the base64 encoded string.
      **/
      static const std::string binaryToBase64(const std::vector<unsigned int> &binary);

      /** base64ToHexString
      *   \brief Convert a base64 encoded string into its hexadecimal representation.
      *   \param[in] base64 is the base64 encoded string.
      *   \return Returns the hexadecimal representation.
      **/
      static const std::string base64ToHexString(const std::string &base64);

      /** hexStringToBase64
      *   \brief Convert a hexadecimal representation into its base64 encoded string.
      *   \param[in] hexString is the hexadecimal representation.
      *   \return Returns the base64 encoded string.
      **/
      static const std::string hexStringToBase64(const std::string &hexString);

      /** base64ToBinString
      *   \brief Convert a base64 encoded string into its binary representation.
      *   \param[in] base64 is the base64 encoded string.
      *   \return Returns the binary representation.
      **/
      static const std::string base64ToBinString(const std::string &base64);

      /** binStringToBase64
      *   \brief Convert a binary representation into its base64 encoded string.
      *   \param[in] binString is the binary representation.
      *   \return Returns the base64 encoded string.
      **/
      static const std::string binStringToBase64(const std::string &binString);

    // protected members, functions ...
    protected:

    // private members, functions ...
    private:

      /** DrmControllerDataConverter
      *   \brief Class constructor.
      **/
      DrmControllerDataConverter();

      /** ~DrmControllerDataConverter
      *   \brief Class destructor.
      **/
      ~DrmControllerDataConverter();

      /** isBase64
      *   \brief Check the input byte is from a base64 encoded string.
      *   \param[in] byte is the byte to check.
      *   \return Returns true when the byte is from a base64 encoded string, false otherwise.
      **/
      static bool isBase64(unsigned char byte);

  }; // class DrmControllerDataConverter

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_DATA_CONVERTER_HPP__
