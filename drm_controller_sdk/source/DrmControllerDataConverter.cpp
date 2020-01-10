/**
*  \file      DrmControllerDataConverter.cpp
*  \version   4.0.1.0
*  \date      January 2020
*  \brief     Class DrmController is used as a top level component in the user application.
*             It provides top level procedures related to the  DRM Controller component.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#include <DrmControllerDataConverter.hpp>

// namespace usage
using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** hexStringToBinary
*   \brief Convert a hexadecimal representation into a list of binary values.
*   \param[in] hexString is the hexadecimal representation.
*   \return Returns the list of binary values.
**/
const std::vector<unsigned int> DrmControllerDataConverter::hexStringToBinary(const std::string &hexString) {
  // create the result vector
  std::vector<unsigned int> result;
  // loop for each element from the hex string
  for(unsigned int ii = 0; ii < hexString.size(); ii+=(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE/DRM_CONTROLLER_NIBBLE_SIZE)) {
    // get a substring
    std::string subString = hexString.substr(ii, DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE/DRM_CONTROLLER_NIBBLE_SIZE);
    // set the current word
    unsigned int currentWord = (unsigned int)strtoul(subString.c_str(), NULL, 16);
    // push the current word
    result.push_back(currentWord);
  }
  // return the result
  return result;
}

/** binaryToHexString
*   \brief Convert a list of binary values into a hexadecimal representation.
*   \param[in] binary is the list of binary values.
*   \return Returns the hexadecimal representation.
**/
const std::string DrmControllerDataConverter::binaryToHexString(const std::vector<unsigned int> &binary) {
  // create the result string
  std::string result("");
  // get each element from the input list
  for (std::vector<unsigned int>::const_iterator it = binary.begin(); it != binary.end(); it++) {
    result += binaryToHexString(*it);
  }
  // return the result
  return result;
}

/** hexStringListToBinary
*   \brief Convert a list of list of hexadecimal string representation into a binary values.
*   \param[in] hexString is the list of hexadecimal string representation.
*   \return Returns the list of binary values.
**/
const std::vector<unsigned int> DrmControllerDataConverter::hexStringListToBinary(const std::vector<std::string> &hexString) {
  std::vector<unsigned int> result;
  result.clear();
  for (std::vector<std::string>::const_iterator it = hexString.begin(); it != hexString.end(); it++) {
    std::vector<unsigned int> convertedData = DrmControllerDataConverter::hexStringToBinary(*it);
    result.insert(result.end(), convertedData.begin(), convertedData.end());
  }
  return result;
}

/** binaryToHexString
*   \brief Convert a binary value into a hexadecimal representation.
*   \param[in] binary is the binary value.
*   \return Returns the hexadecimal representation.
**/
const std::string DrmControllerDataConverter::binaryToHexString(const unsigned int &binary) {
  // create a string stream
  std::stringstream stringStream;
  // push into the string stream the binary value
  // but we need to have to fills with 0 if the size
  // of the value is not system bus data size
  stringStream << std::setfill('0') << std::setw(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE/DRM_CONTROLLER_NIBBLE_SIZE) << std::hex << binary;
  // get the result string
  std::string result = stringStream.str();
  // transform the string to have only upper case chars
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  // return the result string
  return result;
}

/** binaryToHexStringList
*   \brief Convert a list of binary values into a list of hexadecimal string representation.
*   \param[in] binary is the binary value.
*   \param[in] wordsNumber is the number of words to push per string.
*   \return Returns the list of hexadecimal string representation.
**/
const std::vector<std::string> DrmControllerDataConverter::binaryToHexStringList(const std::vector<unsigned int> &binary, const unsigned int &wordsNumber) {
  std::vector<std::string> stringList;
  stringList.clear();
  for (unsigned int ii = 0; ii < binary.size(); ii+=wordsNumber) {
    // fill the vector
    if (binary.begin()+ii < binary.end() && binary.begin()+ii+wordsNumber <= binary.end())
      stringList.push_back(DrmControllerDataConverter::binaryToHexString(std::vector<unsigned int>(binary.begin()+ii, binary.begin()+ii+wordsNumber)));
    else
      stringList.push_back(DrmControllerDataConverter::binaryToHexString(std::vector<unsigned int>(binary.begin(), binary.end())));
  }
  return stringList;
}

/** binaryToVersionString
*   \brief Convert a binary value into a formated string for the version where each digit
*   are separated by a dot. The string is in the form major.minor.correction.
*   \param[in] binary is the binary value.
*   \return Returns the formated version.
**/
const std::string DrmControllerDataConverter::binaryToVersionString(const unsigned int &binary) {
  // create a string stream
  std::stringstream stringStream;
  // create 3 bytes for each digit
  unsigned char digits[DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT];
  // create mask and shifter
  unsigned int mask = 0x00FF0000;
  unsigned int shift = DRM_CONTROLLER_VERSION_DIGIT_SIZE_BITS*(DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT-1);
  // fill each digit
  for (unsigned int ii = 0; ii < DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT; ii++) {
    digits[ii] = (unsigned char)((binary & mask) >> shift);
    // shift the mask and decrement the shift
    mask = mask >> DRM_CONTROLLER_VERSION_DIGIT_SIZE_BITS;
    shift -= DRM_CONTROLLER_VERSION_DIGIT_SIZE_BITS;
    // push into the string stream each the generated digit
    stringStream << std::dec << (unsigned int)digits[ii];
    // append a dot if needed
    if (ii < (DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT-1)) {
      stringStream << ".";
    }
  }
  // get the result string
  std::string result = stringStream.str();
  // transform the string to have only upper case chars
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  // return the result string
  return result;
}

/** binStringToBinary
*   \brief Convert a binary representation into a list of binary values.
*   \param[in] binString is the binary representation.
*   \return Returns the list of binary values.
**/
const std::vector<unsigned int> DrmControllerDataConverter::binStringToBinary(const std::string &binString) {
  // create the result vector
  std::vector<unsigned int> result;
  // loop for each element from the bin string
  for(unsigned int ii = 0; ii < binString.size(); ii+=DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE) {
    // create the current word
    std::bitset<DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE> currentWord(binString.substr(ii, DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE));
    // push the current word
    result.push_back((unsigned int)currentWord.to_ulong());
  }
  // return the result
  return result;

}

/** binaryToBinString
*   \brief Convert a list of binary values into a binary representation.
*   \param[in] binary is the list of binary values.
*   \return Returns the binary representation.
**/
const std::string DrmControllerDataConverter::binaryToBinString(const std::vector<unsigned int> &binary) {
  // create the result string
  std::string result("");
  // get each element from the input list
  for (std::vector<unsigned int>::const_iterator it = binary.begin(); it != binary.end(); it++) {
    result += binaryToBinString(*it);
  }
  // return the result
  return result;
}

/** binaryToBinString
*   \brief Convert a binary value into a binary representation.
*   \param[in] binary is binary value.
*   \return Returns the binary representation.
**/
const std::string DrmControllerDataConverter::binaryToBinString(const unsigned int &binary) {
  // convert the binary value into a binary string
  std::string result = std::bitset<DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE>(binary).to_string();
  // transform the string to have only upper case chars
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  // return the result string
  return result;
}

/** hexStringToBinString
*   \brief Convert a hexadecimal representation into a binary representation.
*   \param[in] hexString is the hexadecimal representation.
*   \return Returns the binary representation.
**/
const std::string DrmControllerDataConverter::hexStringToBinString(const std::string &hexString) {
  // return the converted value
  return binaryToBinString(hexStringToBinary(hexString));
}

/** binStringToHexString
*   \brief Convert a binary representation into a hexadecimal representation.
*   \param[in] binString is the binary representation.
*   \return Returns the hexadecimal representation.
**/
const std::string DrmControllerDataConverter::binStringToHexString(const std::string &binString) {
  // return the converted value
  return binaryToHexString(binStringToBinary(binString));
}

/** asciiStringToBinary
*   \brief Convert an ASCII string into a list of binary values.
*   \param[in] asciiString is the ASCII string.
*   \param[in] asciiStringLen is the length of the ascii string.
*   \return Returns a list of binary values.
**/
const std::vector<unsigned int> DrmControllerDataConverter::asciiStringToBinary(const unsigned char *asciiString, const unsigned int &asciiStringLen) {
  // create the result
  std::vector<unsigned int> result;
  // the current binary value, index and mask
  unsigned int currentVal = 0;
  int currentIndex = 24;
  unsigned int currentMask = 0xFF000000;
  // loop for each element of the ascii string
  for (unsigned int ii = 0; ii < asciiStringLen; ii++) {
    // set the current value
    currentVal += ((asciiString[ii] << currentIndex) & currentMask);
    // update the index and the mask
    currentIndex -= 8;
    currentMask = currentMask >> 8;
    // check the index
    if (currentIndex < 0) {
      // push the current value
      result.push_back(currentVal);
      // reset the index, the current value and the mask
      currentVal = 0;
      currentIndex = 24;
      currentMask = 0xFF000000;
    }
  }
  // return the result
  return result;
}

/** binaryToAsciiString
*   \brief Convert a list of binary values into an ASCII string.
*   \param[in] binary is the list of binary values.
*   \param[out] asciiStringLen is the length of the ascii string.
*   \return Returns the ASCII string.
**/
unsigned char* DrmControllerDataConverter::binaryToAsciiString(const std::vector<unsigned int> &binary, unsigned int &asciiStringLen) {
  // define the size of the ascii string as
  // the number of element from binary multiplied by the size of an unsigned int
  // and divided by the size of an unsigned char
  const size_t len = (binary.size()*sizeof(unsigned int))/sizeof(unsigned char);
  // create the result string and allocate enough size
  unsigned char *result = new unsigned char[len];
  // initialize the length
  asciiStringLen = 0;
  // loop on each element of the binary list
  for (std::vector<unsigned int>::const_iterator it = binary.cbegin(); it != binary.cend(); it++) {
    // the current index and mask
    int currentIndex = 24;
    unsigned int currentMask = 0xFF000000;
    // loop while index is positive
    while (currentIndex >= 0) {
      // update the result
      result[asciiStringLen++] = (unsigned char)(((*it) & currentMask) >> currentIndex);
      // update the index and the mask
      currentIndex -= 8;
      currentMask = currentMask >> 8;
    }
  }
  // return the result
  return result;
}

/** base64ToBinary
*   \brief Convert a base64 encoded string into a list of binary values.
*   \param[in] base64 is the base64 encoded string.
*   \return Returns the list of binary values.
**/
const std::vector<unsigned int> DrmControllerDataConverter::base64ToBinary(const std::string &base64) {
  // converter string
  const std::string converterString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  // input char array
  unsigned char input[4];
  // the buffer
  std::string buffer;
  // input element index
  unsigned int inputIndex = 0;
  // iterate on each element from the input parameter
  for (std::string::const_iterator it = base64.begin(); it != base64.end(); it++) {
    // break whenether the element is the char '=' or the element is not a base64 char
    if (*it == '=' || isBase64(*it) == false) {
      break;
    }
    // set the input element
    input[inputIndex++] = (unsigned char)converterString.find(*it);
    // when the input array is ready
    if (inputIndex == 4) {
      // fill the buffer
      buffer += (input[0] << 2) + ((input[1] & 0x30) >> 4);
      buffer += ((input[1] & 0xf) << 4) + ((input[2] & 0x3c) >> 2);
      buffer += ((input[2] & 0x3) << 6) + input[3];
      // reset input index
      inputIndex = 0;
    }
  }
  // check input index not empty
  if (inputIndex > 0) {
    for (unsigned int jj = inputIndex; jj < 4; jj++)
      input[jj] = (unsigned char) converterString.find((char)0);

    for (unsigned int jj = 0; (jj < inputIndex - 1); jj++) {
      if (jj == 0) {
        buffer += (input[0] << 2) + ((input[1] & 0x30) >> 4);
      }
      else if (jj == 1) {
        buffer += ((input[1] & 0xf) << 4) + ((input[2] & 0x3c) >> 2);
      }
      else if (jj == 2) {
        buffer += ((input[2] & 0x3) << 6) + input[3];
      }
    }
  }
  // convert from ascii to binary
  return asciiStringToBinary((const unsigned char*)buffer.c_str(), (unsigned int) buffer.size());
}

/** binaryToBase64
*   \brief Convert a list of binary values into a base64 encoded string.
*   \param[in] binary is the list of binary values.
*   \return Returns the base64 encoded string.
**/
const std::string DrmControllerDataConverter::binaryToBase64(const std::vector<unsigned int> &binary) {
  // convert the binary list into an ascii string
  unsigned int len = 0;
  unsigned char *buffer = binaryToAsciiString(binary, len);
  // converter string
  const std::string converterString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  // input char array
  unsigned char input[3];
  // input index
  unsigned int inputIndex = 0;
  // the result string
  std::string result;
  // iterate on each element of the buffer
  for (unsigned int ii = 0; ii < len; ii++) {
    // set the input array
    input[inputIndex++] = buffer[ii];
    // the input array is filled
    if (inputIndex == 3) {
      result += converterString[(input[0] & 0xfc) >> 2];
      result += converterString[((input[0] & 0x03) << 4) + ((input[1] & 0xf0) >> 4)];
      result += converterString[((input[1] & 0x0f) << 2) + ((input[2] & 0xc0) >> 6)];
      result += converterString[input[2] & 0x3f];
      // reset the input index
      inputIndex = 0;
    }
  }
  // delete the buffer
  delete[] buffer;
  // check input index
  if (inputIndex > 0) {
    // reset unused chars
    for (unsigned int jj = inputIndex; jj < 3; jj++) {
      input[jj] = 0;
    }
    // update the result
    for (unsigned int jj = 0; (jj < inputIndex + 1); jj++) {
      if (jj == 0) {
        result += converterString[(input[0] & 0xfc) >> 2];
      }
      else if (jj == 1) {
        result += converterString[((input[0] & 0x03) << 4) + ((input[1] & 0xf0) >> 4)];
      }
      else if (jj == 2) {
        result += converterString[((input[1] & 0x0f) << 2) + ((input[2] & 0xc0) >> 6)];
      }
      else if (jj == 3) {
        result += converterString[input[2] & 0x3f];
      }
    }
    // add '=' char if needed
    while (inputIndex++ < 3) {
      result += '=';
    }
  }
  // return the result
  return result;
}

/** base64ToHexString
*   \brief Convert a base64 encoded string into its hexadecimal representation.
*   \param[in] base64 is the base64 encoded string.
*   \return Returns the hexadecimal representation.
**/
const std::string DrmControllerDataConverter::base64ToHexString(const std::string &base64) {
  // return the converted value
  return binaryToHexString(base64ToBinary(base64));
}

/** hexStringToBase64
*   \brief Convert a hexadecimal representation into its base64 encoded string.
*   \param[in] hexString is the hexadecimal representation.
*   \return Returns the base64 encoded string.
**/
const std::string DrmControllerDataConverter::hexStringToBase64(const std::string &hexString) {
  // return the converted value
  return binaryToBase64(hexStringToBinary(hexString));
}

/** base64ToBinString
*   \brief Convert a base64 encoded string into its binary representation.
*   \param[in] base64 is the base64 encoded string.
*   \return Returns the binary representation.
**/
const std::string DrmControllerDataConverter::base64ToBinString(const std::string &base64) {
  // return the converted value
  return binaryToBinString(base64ToBinary(base64));
}

/** binStringToBase64
*   \brief Convert a binary representation into its base64 encoded string.
*   \param[in] binString is the binary representation.
*   \return Returns the base64 encoded string.
**/
const std::string DrmControllerDataConverter::binStringToBase64(const std::string &binString) {
  // return the converted value
  return binaryToBase64(binStringToBinary(binString));
}

/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/

/** DrmControllerDataConverter
*   \brief Class constructor.
**/
DrmControllerDataConverter::DrmControllerDataConverter() {}

/** ~DrmControllerDataConverter
*   \brief Class destructor.
**/
DrmControllerDataConverter::~DrmControllerDataConverter() {}

/** isBase64
*   \brief Check the input byte is from a base64 encoded string.
*   \param[in] byte is the byte to check.
*   \return Returns true when the byte is from a base64 encoded string, false otherwise.
**/
bool DrmControllerDataConverter::isBase64(unsigned char byte) {
  return (isalnum(byte) || (byte == '+') || (byte == '/'));
}
