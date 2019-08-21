/**
*  \file      DrmControllerRegistersReport.hpp
*  \version   4.0.0.0
*  \date      July 2019
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

#ifndef __DRM_CONTROLLER_REGISTERS_REPORT_HPP__
#define __DRM_CONTROLLER_REGISTERS_REPORT_HPP__

#include <string.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <DrmControllerCommon.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerRegistersReport DrmControllerRegistersReport.hpp "include/HAL/DrmControllerRegistersReport.hpp"
  *   \brief    Class DrmControllerRegistersReport defines low level procedures for registers access.
  **/
  class DrmControllerRegistersReport {

    // public members, functions ...
    public:

      /** DrmControllerRegistersReport
      *   \brief Class constructor.
      **/
      DrmControllerRegistersReport();

      /** ~DrmControllerRegistersReport
      *   \brief Class destructor.
      **/
      ~DrmControllerRegistersReport();

      /** header
      *   \brief Get the header of the hardware report.
      *   \return Returns a string containing the header.
      **/
      std::string header() const;

      /** footer
      *   \brief Get the footer of the hardware report.
      *   \return Returns a string containing the footer.
      **/
      std::string footer() const;

      /** componentName
      *   \brief Get the component name of the hardware report.
      *   \param[in] name is the name of the component to write.
      *   \return Returns a string containing the component name.
      **/
      std::string componentName(const std::string &name) const;

      /** registerName
      *   \brief Get the register name of the hardware report.
      *   \param[in] name is the name of the register to write.
      *   \return Returns a string containing the register name.
      **/
      std::string registerName(const std::string &name) const;

      /** versionName
      *   \brief Get the version name of the hardware report.
      *   \param[in] name is the name of the version to write.
      *   \return Returns a string containing the version name.
      **/
      std::string versionName(const std::string &name) const;

      /** fileName
      *   \brief Get the file name of the hardware report.
      *   \param[in] name is the name of the file to write.
      *   \return Returns a string containing the file name.
      **/
      std::string fileName(const std::string &name) const;

      /** registerValue
      *   \brief Get the value of the string register of the hardware report.
      *   \param[in] value is the string value of the register to write.
      *   \param[in] description is the description of the register to write.
      *   \return Returns a string containing the register value.
      **/
      std::string registerValue(const std::string &value, const std::string &description) const;

      /** registerValue
      *   \brief Get the value of the integer register of the hardware report.
      *   \param[in] value is the integer value of the register to write.
      *   \param[in] description is the description of the register to write.
      *   \return Returns a string containing the integer value.
      **/
      std::string registerValue(const unsigned int &value, const std::string &description) const;

      /** generateBitRegisterValue
      *   \brief Get the value of the bit register of the hardware report.
      *   \param[in] value is the bit value of the register to write.
      *   \param[in] description is the description of the register to write.
      *   \return Returns a string containing the bit value.
      **/
      std::string registerValue(const bool &value, const std::string &description) const;

      /** registerValue
      *   \brief Get the value of the N bits word register of the hardware report.
      *   \param[in] value is the N bits word value of the register to write.
      *   \param[in] description is the description of the register to write.
      *   \param[in] nibbles is the number of nibble to display.
      *   \return Returns a string containing the word value.
      **/
      std::string registerValue(const unsigned int &value, const std::string &description, const unsigned int &nibbles) const;

      /** registerValue
      *   \brief generate the value of the array of string register of the hardware report.
      *   \param[in] value is the array of string value of the register to write.
      *   \param[in] description is the description of the register to write.
      *   \param[in] start is the value of index of the first word to display.
      **/
      std::string registerValue(const std::vector<std::string> &value, const std::string &description, const int &start) const;

      /** registerValue
      *   \brief generate the value of the array of string register of the hardware report.
      *   \param[in] value is the string value of the register to write.
      *   \param[in] description is the description of the register to write.
      *   \param[in] size is the size of a word.
      *   \param[in] start is the value of index of the first word to display.
      **/
      std::string registerValue(const std::string &value, const std::string &description, const unsigned int &size, const int &start) const;

      /** stringValue
      *   \brief Get the value of the string of the hardware report.
      *   \param[in] value is the string value to write.
      *   \param[in] description is the description to write.
      *   \return Returns a string containing the string value.
      **/
      std::string stringValue(const std::string &value, const std::string &description) const;

    // protected members, functions ...
    protected:

      /** concat
      *   \brief Concatenate 2 strings.
      *   \param[in] str1 is the first string.
      *   \param[in] str2 is the second string.
      *   \return Returns a str1 concatenated with str2.
      **/
      std::string concat(const std::string &str1, const std::string &str2) const;

      /** wsConcat
      *   \brief Concatenate 2 strings with a whitespace.
      *   \param[in] str1 is the first string.
      *   \param[in] str2 is the second string.
      *   \return Returns a str1 concatenated with a whitespace and str2.
      **/
      std::string wsConcat(const std::string &str1, const std::string &str2) const;

      /** padLeft
      *   \brief Pad to the left a string.
      *   \param[in] fill is the filler character to use.
      *   \param[in] width is the max width of the padding.
      *   \param[in] str is the string to pad on the left.
      *   \return Returns a left padded string.
      **/
      std::string padLeft(const char &fill, const unsigned int &width, const std::string &str) const;

      /** padLeft
      *   \brief Pad to the left a character.
      *   \param[in] fill is the filler character to use.
      *   \param[in] width is the max width of the padding.
      *   \param[in] c is the character to pad on the left.
      *   \return Returns a left padded character.
      **/
      std::string padLeft(const char &fill, const unsigned int &width, const char &c) const;

      /** padLeft
      *   \brief Pad to the left a number.
      *   \param[in] fill is the filler character to use.
      *   \param[in] width is the max width of the padding.
      *   \param[in] n is the number to pad on the left.
      *   \return Returns a left padded number.
      **/
      std::string padLeft(const char &fill, const unsigned int &width, const int &n) const;

      /** padRight
      *   \brief Pad to the right a string.
      *   \param[in] fill is the filler character to use.
      *   \param[in] width is the max width of the padding.
      *   \param[in] str is the string to pad on the right.
      *   \return Returns a right padded string.
      **/
      std::string padRight(const char &fill, const unsigned int &width, const std::string &str) const;

      /** padRight
      *   \brief Pad to the right a character.
      *   \param[in] fill is the filler character to use.
      *   \param[in] width is the max width of the padding.
      *   \param[in] c is the character to pad on the right.
      *   \return Returns a right padded character.
      **/
      std::string padRight(const char &fill, const unsigned int &width, const char &c) const;

      /** padRight
      *   \brief Pad to the right a number.
      *   \param[in] fill is the filler character to use.
      *   \param[in] width is the max width of the padding.
      *   \param[in] n is the number to pad on the right.
      *   \return Returns a right padded number.
      **/
      std::string padRight(const char &fill, const unsigned int &width, const int &n) const;

    // private members, functions ...
    private:

  }; // class DrmControllerRegistersReport

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_REGISTERS_REPORT_HPP__
