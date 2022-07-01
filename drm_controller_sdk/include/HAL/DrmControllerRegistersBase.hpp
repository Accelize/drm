/**
*  \file      DrmControllerRegistersBase.hpp
*  \version   8.0.0.0
*  \date      March 2022
*  \brief     Class DrmControllerRegistersBase defines low level procedures for registers access.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_REGISTERS_BASE_HPP__
#define __DRM_CONTROLLER_REGISTERS_BASE_HPP__

#include <string.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <HAL/DrmControllerTypes.hpp>
#include <HAL/DrmControllerRegistersReport.hpp>
#include <DrmControllerCommon.hpp>
#include <DrmControllerDataConverter.hpp>

#include <DrmControllerTimeOutException.hpp>
#include <DrmControllerLicenseFileSizeException.hpp>
#include <DrmControllerUnsupportedFeatureException.hpp>
#include <DrmControllerLicenseTimerResetedException.hpp>
#include <DrmControllerFunctionalityDisabledException.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerRegistersBase DrmControllerRegistersBase.hpp "include/HAL/DrmControllerRegistersBase.hpp"
  *   \brief    Class DrmControllerRegistersBase defines low level procedures for registers access.
  **/
  class DrmControllerRegistersBase: public DrmControllerRegistersReport {

    // public members, functions ...
    public:

      /** DrmControllerRegistersBase
      *   \brief Class constructor.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerRegistersBase(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction);

      /** ~DrmControllerRegistersBase
      *   \brief Class destructor.
      **/
      ~DrmControllerRegistersBase();

      /** setIndexedRegisterName
      *   \brief Indexed register name setter.
      *   \param[in] indexedRegisterName is the name to set.
      **/
      void setIndexedRegisterName(const std::string &indexedRegisterName);

      /** getIndexedRegisterName
      *   \brief Indexed register name getter.
      *   \return Returns the value of the indexed register name.
      **/
      std::string getIndexedRegisterName() const;

      /** readRegister
      *   \brief Read the value from the register pointed by offset.
      *   \param[in] offset is the offset of the register to read.
      *   \param[inout] value is the read value of the register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read register functions otherwize.
      **/
      unsigned int readRegister(const unsigned int &offset, unsigned int &value) const;

      /** writeRegister
      *   \brief Write the value to the register pointed by offset.
      *   \param[in] offset is the offset of the register to read.
      *   \param[in] value is the value to write to the register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read register functions otherwize.
      **/
      unsigned int writeRegister(const unsigned int &offset, const unsigned int &value) const;

      /** bits
      *   \brief Get the value of a several and contigous bits.
      *   \param[in] lsb is the lsb position of the bits.
      *   \param[in] mask is the mask of the bits.
      *   \param[in] value is the value to extract the bits from.
      *   \return Returns is the masked and shifted value of the bits.
      **/
      unsigned int bits(const unsigned int &lsb, const unsigned int &mask, const unsigned int &value) const;

      /** readRegisterListFromIndex
      *   \brief Read a list of register starting from a specified index.
      *   This method will access to the system bus to read several registers.
      *   \param[in]  from is the first index of the register to read.
      *   \param[in]  n is the number of words to read.
      *   \param[out] value is the value of registers read.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      unsigned int readRegisterListFromIndex(const unsigned int &from, const unsigned int &n, std::vector<unsigned int> &value) const;

      /** readRegisterAtIndex
      *   \brief Read a register at a specified index.
      *   This method will access to the system bus to read a register.
      *   \param[in]  index is the index of the register word to read.
      *   \param[out] value is the value of the register read.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      unsigned int readRegisterAtIndex(const unsigned int &index, unsigned int &value) const;

      /** writeRegisterListFromIndex
      *   \brief Write a list of register starting from a specified index.
      *   This method will access to the system bus to write several registers.
      *   \param[in] from is the first index of the register to write.
      *   \param[in] n is the number of words to write.
      *   \param[in] value is the value of registers to write.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      unsigned int writeRegisterListFromIndex(const unsigned int &from, const unsigned int &n, const std::vector<unsigned int> &value) const;

      /** writeRegisterAtIndex
      *   \brief Write a register at a specified index.
      *   This method will access to the system bus to write a register.
      *   \param[in] index is the index of the register word to write.
      *   \param[in] value is the value of the register to write.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      unsigned int writeRegisterAtIndex(const unsigned int &index, const unsigned int &value) const;

      /** registerOffsetFromIndex
      *   \brief Get the register offset from index
      *   \param[in] index is the register index.
      *   \return Returns the offset of the register at the specified index.
      **/
      unsigned int registerOffsetFromIndex(const unsigned int &index) const;

      /** numberOfWords
      *   \brief Get the number of words used by a register.
      *   \param[in] registerSize is the size of the register in bits.
      *   \return Returns the number of words used for the given register size.
      **/
      unsigned int numberOfWords(const unsigned int& registerSize) const;

      /** throwLicenseFileSizeException
      *   \param[in]  expected is the expected size of the license file.
      *   \param[in]  actual is the actual size of the license file.
      *   \throw Throw DrmControllerLicenseFileSizeException. DrmControllerLicenseFileSizeException::what() should be called to get the exception description.
      **/
      void throwLicenseFileSizeException(unsigned int expected, unsigned int actual) const;

      /** throwUnsupportedFeatureException
      *   \param[in]  featureName is the name of the feature to be used when throwing unsupported feature error exception.
      *   \param[in]  drmVersion is the DRM Controller version to be used when throwing unsupported feature error exception.
      *   \throw Throw DrmControllerUnsupportedFeatureException. DrmControllerUnsupportedFeatureException::what() should be called to get the exception description.
      **/
      void throwUnsupportedFeatureException(const std::string &featureName, const std::string drmVersion) const;

      /** throwFunctionalityDisabledException
      *   \param[in]  name is the name of the functionality to be used when throwing functionality disabled error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used when throwing functionality disabled error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used when throwing functionality disabled error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \throw Throw DrmControllerFunctionalityDisabledException. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
      **/
      void throwFunctionalityDisabledException(const std::string &name,
                                               const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                               const bool &expectedStatus, const bool &actualStatus) const;

      /** throwFunctionalityDisabledException
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \throw Throw DrmControllerLicenseTimerResetedException. DrmControllerLicenseTimerResetedException::what() should be called to get the exception description.
      **/
      void throwLicenseTimerResetedException(const bool &expectedStatus, const bool &actualStatus, const unsigned char &expectedError, const unsigned char &actualError,
                                             const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

      /** throwTimeoutException
      *   \brief Throw a timeout exception.
      *   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg) const;

      /** throwTimeoutException
      *   \brief Throw a timeout exception.
      *   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used when throwing timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used when throwing timeout error exception.
      *   \param[in]  expectedErrorMsg is the expected error message to be used when throwing timeout error exception.
      *   \param[in]  actualErrorMsg is the actual error message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                 const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                 const unsigned int &expectedStatus, const unsigned int &actualStatus,
                                 const unsigned char &expectedError, const unsigned char &actualError,
                                 const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

      /** throwTimeoutException
      *   \brief Throw a timeout exception.
      *   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used when throwing timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used when throwing timeout error exception.
      *   \param[in]  expectedErrorMsg is the expected error message to be used when throwing timeout error exception.
      *   \param[in]  actualErrorMsg is the actual error message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                 const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                 const bool &expectedStatus, const bool &actualStatus,
                                 const unsigned char &expectedError, const unsigned char &actualError,
                                 const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

      /** throwTimeoutException
      *   \brief Throw a timeout exception.
      *   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used when throwing timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                 const unsigned int &expectedStatus, const unsigned int &actualStatus) const;

      /** throwTimeoutException
      *   \brief Throw a timeout exception.
      *   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used when throwing timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used when throwing timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                 const bool &expectedStatus, const bool &actualStatus) const;

      /** throwTimeoutException
      *   \brief Throw a timeout exception.
      *   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
      *   \param[in]  expectedErrorMsg is the expected error message to be used when throwing timeout error exception.
      *   \param[in]  actualErrorMsg is the actual error message to be used when throwing timeout error exception.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                 const unsigned char &expectedError, const unsigned char &actualError,
                                 const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

    // protected members, functions ...
    protected:

    // private members, functions ...
    private:

      tDrmReadRegisterFunction  mReadRegisterFunction;
      tDrmWriteRegisterFunction mWriteRegisterFunction;

      std::string mIndexedRegisterName;

      /** unsupportedFeatureExceptionDescription
      *   \brief Generate the description of a unsupported feature exception.
      *   \param[in]  featureName is the name of the feature to be used with unsupported feature error exception.
      *   \param[in]  drmVersion is the DRM Controller version to be used with unsupported feature error exception.
      *   \return Returns a string containing the exception description.
      */
      std::string unsupportedFeatureExceptionDescription(const std::string &featureName, const std::string &drmVersion) const;

      /** functionalityDisabledExceptionDescription
      *   \param[in]  name is the name of the functionality to be used with functionality disabled error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used with functionality disabled error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used with functionality disabled error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \return Returns a string containing the exception description.
      **/
      std::string functionalityDisabledExceptionDescription(const std::string &name, const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                            const bool &expectedStatus, const bool &actualStatus) const;

      /** timeoutExceptionDescription
      *   \brief Generate the description of a timeout exception.
      *   \param[in]  headerMsg is the header message to be used with timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used with timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used with timeout error exception.
      *   \param[in]  expectedErrorMsg is the expected error message to be used with timeout error exception.
      *   \param[in]  actualErrorMsg is the actual error message to be used with timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \return Returns a string containing the exception description.
      */
      std::string timeoutExceptionDescription(const std::string &headerMsg,
                                              const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                              const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                              const unsigned int &expectedStatus, const unsigned int &actualStatus,
                                              const unsigned char &expectedError, const unsigned char &actualError,
                                              const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

      /** timeoutExceptionDescription
      *   \brief Generate the description of a timeout exception.
      *   \param[in]  headerMsg is the header message to be used with timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used with timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used with timeout error exception.
      *   \param[in]  expectedErrorMsg is the expected error message to be used with timeout error exception.
      *   \param[in]  actualErrorMsg is the actual error message to be used with timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \return Returns a string containing the exception description.
      */
      std::string timeoutExceptionDescription(const std::string &headerMsg,
                                              const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                              const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                              const bool &expectedStatus, const bool &actualStatus,
                                              const unsigned char &expectedError, const unsigned char &actualError,
                                              const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

      /** timeoutExceptionDescription
      *   \brief Generate the description of a timeout exception.
      *   \param[in]  headerMsg is the header message to be used with timeout error exception.
      *   \param[in]  expectedErrorMsg is the expected error message to be used with timeout error exception.
      *   \param[in]  actualErrorMsg is the actual error message to be used with timeout error exception.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
      *   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
      *   \return Returns a string containing the exception description.
      */
      std::string timeoutExceptionDescription(const std::string &headerMsg,
                                              const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                              const unsigned char &expectedError, const unsigned char &actualError,
                                              const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const;

      /** timeoutExceptionDescription
      *   \brief Generate the description of a timeout exception.
      *   \param[in]  headerMsg is the header message to be used with timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used with timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used with timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \return Returns a string containing the exception description.
      */
      std::string timeoutExceptionDescription(const std::string &headerMsg,
                                              const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                              const unsigned int &expectedStatus, const unsigned int &actualStatus) const;

      /** timeoutExceptionDescription
      *   \brief Generate the description of a timeout exception.
      *   \param[in]  headerMsg is the header message to be used with timeout error exception.
      *   \param[in]  expectedStatusMsg is the expected status message to be used with timeout error exception.
      *   \param[in]  actualStatusMsg is the actual status message to be used with timeout error exception.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \return Returns a string containing the exception description.
      */
      std::string timeoutExceptionDescription(const std::string &headerMsg,
                                              const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                              const bool &expectedStatus, const bool &actualStatus) const;

      /** exceptionHeaderDescription
      *   \brief Generate the header description of an exception.
      *   \param[in]  headerMsg is the header message to be used with error exception.
      *   \return Returns a string containing the exception header description.
      */
      std::string exceptionHeaderDescription(const std::string &headerMsg) const;

      /** exceptionStatusDescription
      *   \brief Generate the status description of an exception.
      *   \param[in]  statusMsg is the status message to be used with error exception.
      *   \param[in]  status is the value of the status to be used.
      *   \return Returns a string containing the exception status description.
      */
      std::string exceptionStatusDescription(const std::string &statusMsg, const unsigned int &status) const;

      /** exceptionStatusDescription
      *   \brief Generate the status description of an exception.
      *   \param[in]  statusMsg is the status message to be used with error exception.
      *   \param[in]  status is the value of the status to be used.
      *   \return Returns a string containing the exception status description.
      */
      std::string exceptionStatusDescription(const std::string &statusMsg, const bool &status) const;

      /** exceptionErrorDescription
      *   \brief Generate the error description of an exception.
      *   \param[in]  errorMsg is the error message to be used with error exception.
      *   \param[in]  error is the value of the error to be used.
      *   \param[in]  errorCodeMsg is the value of the message of the error.
      *   \return Returns a string containing the exception error description.
      */
      std::string exceptionErrorDescription(const std::string &errorMsg, const unsigned char &error, const std::string &errorCodeMsg) const;

      /** binaryDescription
      *   \brief Generate the description of a binary value.
      *   \param[in] binary is the binary value to be used in the description.
      *   \return Returns a string containing the description of a binary value.
      **/
      std::string binaryDescription(const bool &binary) const;

      /** hexadecimalDescription
      *   \brief Generate the description of a hexadecimal value.
      *   \param[in] hexadecimal is the hexadecimal value to be used in the description.
      *   \return Returns a string containing the description of a hexadecimal value.
      **/
      std::string hexadecimalDescription(const unsigned int &hexadecimal) const;

      /** hexadecimalDescription
      *   \brief Generate the description of a hexadecimal value.
      *   \param[in] hexadecimal is the hexadecimal value to be used in the description.
      *   \return Returns a string containing the description of a hexadecimal value.
      **/
      std::string hexadecimalDescription(const unsigned char &hexadecimal) const;

      /** exceptionHeader
      *   \brief Generate the header of an exception.
      *   \return Returns a string containing the exception header.
      */
      std::string exceptionHeader() const;

      /** exceptionEmptyHeader
      *   \brief Generate the empty header of an exception.
      *   \return Returns a string containing the exception empty header.
      */
      std::string exceptionEmptyHeader() const;

      /** exceptionFooter
      *   \brief Generate the footer of an exception.
      *   \return Returns a string containing the exception footer.
      */
      std::string exceptionFooter() const;

  }; // class DrmControllerRegistersBase

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_REGISTERS_BASE_HPP__
