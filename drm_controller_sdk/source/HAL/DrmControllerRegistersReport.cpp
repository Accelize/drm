/**
*  \file      DrmControllerRegistersReport.hpp
*  \version   6.0.1.0
*  \date      May 2021
*  \brief     Class DrmControllerRegistersReport defines low level procedures for registers reporting.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#include <HAL/DrmControllerRegistersReport.hpp>

// namespace usage
using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** DrmControllerRegistersReport
*   \brief Class constructor.
**/
DrmControllerRegistersReport::DrmControllerRegistersReport()
{ }

/** ~DrmControllerRegistersReport
*   \brief Class destructor.
**/
DrmControllerRegistersReport::~DrmControllerRegistersReport()
{ }


/** header
*   \brief Get the header of the hardware report.
*   \return Returns a string containing the header.
**/
std::string DrmControllerRegistersReport::header() const {
  return padRight('-',80,'-');
}

/** footer
*   \brief Get the footer of the hardware report.
*   \return Returns a string containing the footer.
**/
std::string DrmControllerRegistersReport::footer() const {
  return padRight('-',80,'-');
}

/** componentName
*   \brief Get the component name of the hardware report.
*   \param[in] name is the name of the component to write.
*   \return Returns a string containing the component name.
**/
std::string DrmControllerRegistersReport::componentName(const std::string &name) const {
  return wsConcat(name, "COMPONENT");
}

/** registerName
*   \brief Get the register name of the hardware report.
*   \param[in] name is the name of the register to write.
*   \return Returns a string containing the register name.
**/
std::string DrmControllerRegistersReport::registerName(const std::string &name) const {
  return wsConcat(padRight(' ',3,'-'), wsConcat(name, "REGISTER"));
}

/** versionName
*   \brief Get the version name of the hardware report.
*   \param[in] name is the name of the version to write.
*   \return Returns a string containing the version name.
**/
std::string DrmControllerRegistersReport::versionName(const std::string &name) const {
  return wsConcat(padRight(' ',3,'-'), wsConcat(name, "VERSION"));
}

/** fileName
*   \brief Get the file name of the hardware report.
*   \param[in] name is the name of the file to write.
*   \return Returns a string containing the file name.
**/
std::string DrmControllerRegistersReport::fileName(const std::string &name) const {
  return wsConcat(padRight(' ',3,'-'), wsConcat(name, "FILE"));
}

/** registerValue
*   \brief Get the value of the string register of the hardware report.
*   \param[in] value is the string value of the register to write.
*   \param[in] description is the description of the register to write.
*   \return Returns a string containing the register value.
**/
std::string DrmControllerRegistersReport::registerValue(const std::string &value, const std::string &description) const {
  return stringValue(concat("0x", value), description);
}

/** registerValue
*   \brief Get the value of the integer register of the hardware report.
*   \param[in] value is the integer value of the register to write.
*   \param[in] description is the description of the register to write.
*   \return Returns a string containing the integer value.
**/
std::string DrmControllerRegistersReport::registerValue(const unsigned int &value, const std::string &description) const {
  std::ostringstream writter;
  writter << value;
  return stringValue(writter.str(), description);
}

/** generateBitRegisterValue
*   \brief Get the value of the bit register of the hardware report.
*   \param[in] value is the bit value of the register to write.
*   \param[in] description is the description of the register to write.
*   \return Returns a string containing the bit value.
**/
std::string DrmControllerRegistersReport::registerValue(const bool &value, const std::string &description) const {
  std::ostringstream writter;
  writter << "0b" << value;
  return stringValue(writter.str(), wsConcat(wsConcat(description, "BIT"),""));
}

/** registerValue
*   \brief Get the value of the N bits word register of the hardware report.
*   \param[in] value is the N bits word value of the register to write.
*   \param[in] description is the description of the register to write.
*   \param[in] nibbles is the number of nibble to display.
*   \return Returns a string containing the word value.
**/
std::string DrmControllerRegistersReport::registerValue(const unsigned int &value, const std::string &description, const unsigned int &nibbles) const {
  std::ostringstream writter;
  writter << std::hex << value;
  return stringValue(concat("0x",padLeft('0', nibbles, writter.str())), description);
}

/** registerValue
*   \brief generate the value of the array of string register of the hardware report.
*   \param[in] value is the array of string value of the register to write.
*   \param[in] description is the description of the register to write.
*   \param[in] start is the value of index of the first word to display.
**/
std::string DrmControllerRegistersReport::registerValue(const std::vector<std::string> &value, const std::string &description, const int &start) const {
  // the number of digits
  std::ostringstream writter;
  if (start < 0)
    writter << value.size()-2;
  else
    writter << value.size()-1;
  size_t digits = writter.str().size();
  if (start < 0) digits++;
  int ii = start;
  writter.str("");
  for (std::vector<std::string>::const_iterator it = value.begin(); it != value.end(); it++) {
    if (ii < 0)
      writter << stringValue(concat("0x", *it), concat(wsConcat(description, "-"),padRight('0',digits-1,-ii))) << std::endl;
    else
      writter << stringValue(concat("0x", *it), wsConcat(description,padRight('0',digits,ii))) << std::endl;
    ii++;
  }
  return writter.str();
}

/** registerValue
*   \brief generate the value of the array of string register of the hardware report.
*   \param[in] value is the string value of the register to write.
*   \param[in] description is the description of the register to write.
*   \param[in] size is the size of a word.
*   \param[in] start is the value of index of the first word to display.
**/
std::string DrmControllerRegistersReport::registerValue(const std::string &value, const std::string &description, const unsigned int &size, const int &start) const {
  // the number of digits for the max number of words
  std::ostringstream writter;
  writter << value.size()/size;
  const size_t digits = writter.str().size();
  writter.str("");
  for (unsigned int ii = 0; ii < value.size(); ii += size)
    writter << stringValue(concat("0x", value.substr(ii, size)), wsConcat(description, padRight('0', digits, (int)(ii/size)))) << std::endl;
  return writter.str();
}

/** stringValue
*   \brief Get the value of the string of the hardware report.
*   \param[in] value is the string value to write.
*   \param[in] description is the description to write.
*   \return Returns a string containing the string value.
**/
std::string DrmControllerRegistersReport::stringValue(const std::string &value, const std::string &description) const {
  return wsConcat(wsConcat(wsConcat(padRight(' ', 6, '-'), padLeft('.', 48, wsConcat(description, ""))), ":"), value);
}

/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/** concat
*   \brief Concatenate 2 strings.
*   \param[in] str1 is the first string.
*   \param[in] str2 is the second string.
*   \return Returns a str1 concatenated with str2.
**/
std::string DrmControllerRegistersReport::concat(const std::string &str1, const std::string &str2) const {
  return str1 + str2;
}

/** wsConcat
*   \brief wsConcatenate 2 strings with a whitespace.
*   \param[in] str1 is the first string.
*   \param[in] str2 is the second string.
*   \return Returns a str1 wsConcatenated with a whitespace and str2.
**/
std::string DrmControllerRegistersReport::wsConcat(const std::string &str1, const std::string &str2) const {
  std::string str("");
  str += DRM_CONTROLLER_ERROR_SPACE;
  return concat(concat(str1, str), str2);
}

/** padLeft
*   \brief Pad to the left a string.
*   \param[in] fill is the filler character to use.
*   \param[in] width is the max width of the padding.
*   \param[in] str is the string to pad on the left.
*   \return Returns a left padded string.
**/
std::string DrmControllerRegistersReport::padLeft(const char &fill, const unsigned int &width, const std::string &str) const {
  std::ostringstream writter;
  writter << std::left << std::setfill(fill) << std::setw(width) << str;
  return writter.str();
}

/** padLeft
*   \brief Pad to the left a character.
*   \param[in] fill is the filler character to use.
*   \param[in] width is the max width of the padding.
*   \param[in] c is the character to pad on the left.
*   \return Returns a left padded character.
**/
std::string DrmControllerRegistersReport::padLeft(const char &fill, const unsigned int &width, const char &c) const {
  std::string str("");
  str += c;
  return padLeft(fill, width, str);
}

/** padLeft
*   \brief Pad to the left a number.
*   \param[in] fill is the filler character to use.
*   \param[in] width is the max width of the padding.
*   \param[in] n is the number to pad on the left.
*   \return Returns a left padded number.
**/
std::string DrmControllerRegistersReport::padLeft(const char &fill, const unsigned int &width, const int &n) const {
  std::ostringstream writter;
  writter << n;
  return padLeft(fill, width, writter.str());
}


/** padRight
*   \brief Pad to the right a string.
*   \param[in] fill is the filler character to use.
*   \param[in] width is the max width of the padding.
*   \param[in] str is the string to pad on the right.
*   \return Returns a right padded string.
**/
std::string DrmControllerRegistersReport::padRight(const char &fill, const unsigned int &width, const std::string &str) const {
  std::ostringstream writter;
  writter << std::right << std::setfill(fill) << std::setw(width) << str;
  return writter.str();
}

/** padRight
*   \brief Pad to the right a character.
*   \param[in] fill is the filler character to use.
*   \param[in] width is the max width of the padding.
*   \param[in] c is the character to pad on the right.
*   \return Returns a right padded character.
**/
std::string DrmControllerRegistersReport::padRight(const char &fill, const unsigned int &width, const char &c) const {
  std::string str("");
  str += c;
  return padRight(fill, width, str);
}

/** padRight
*   \brief Pad to the right a number.
*   \param[in] fill is the filler character to use.
*   \param[in] width is the max width of the padding.
*   \param[in] n is the number to pad on the right.
*   \return Returns a right padded number.
**/
std::string DrmControllerRegistersReport::padRight(const char &fill, const unsigned int &width, const int &n) const {
  std::ostringstream writter;
  writter << n;
  return padRight(fill, width, writter.str());
}

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/
