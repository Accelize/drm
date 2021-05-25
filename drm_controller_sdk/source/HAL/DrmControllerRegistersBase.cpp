/**
*  \file      DrmControllerRegistersBase.cpp
*  \version   6.0.0.0
*  \date      April 2021
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

#include <HAL/DrmControllerRegistersBase.hpp>

// name space usage
using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** DrmControllerRegistersBase
*   \brief Class constructor.
*   \param[in] readRegisterFunction function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] writeRegisterFunction function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
**/
DrmControllerRegistersBase::DrmControllerRegistersBase(tDrmReadRegisterFunction readRegisterFunction,
                                                       tDrmWriteRegisterFunction writeRegisterFunction)
: DrmControllerRegistersReport(),
  mReadRegisterFunction(readRegisterFunction),
  mWriteRegisterFunction(writeRegisterFunction),
  mIndexedRegisterName("")
{ }

/** ~DrmControllerRegistersBase
*   \brief Class destructor.
**/
DrmControllerRegistersBase::~DrmControllerRegistersBase() {

}

/** setIndexedRegisterName
*   \brief Indexed register name setter.
*   \param[in] indexedRegisterName is the name to set.
**/
void DrmControllerRegistersBase::setIndexedRegisterName(const std::string &indexedRegisterName) {
  mIndexedRegisterName = indexedRegisterName;
}

/** getIndexedRegisterName
*   \brief Indexed register name getter.
*   \return Returns the value of the indexed register name.
**/
std::string DrmControllerRegistersBase::getIndexedRegisterName() const {
  return mIndexedRegisterName;
}

/** readRegister
*   \brief Read the value from the register pointed by name.
*   \param[in] name is the name of the register to read.
*   \param[inout] value is the read value of the register.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read register functions otherwize.
**/
unsigned int DrmControllerRegistersBase::readRegister(const std::string &name, unsigned int &value) const {
  return mReadRegisterFunction(name, value);
}

/** writeRegister
*   \brief Write the value to the register pointed by name.
*   \param[in] name is the name of the register to read.
*   \param[in] value is the value to write to the register.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read register functions otherwize.
**/
unsigned int DrmControllerRegistersBase::writeRegister(const std::string &name, const unsigned int &value) const {
  return mWriteRegisterFunction(name, value);
}

/** bits
*   \brief Get the value of a several and contigous bits.
*   \param[in] lsb is the lsb position of the bits.
*   \param[in] mask is the mask of the bits.
*   \param[in] value is the value to extract the bits from.
*   \return Returns is the masked and shifted value of the bits.
**/
unsigned int DrmControllerRegistersBase::bits(const unsigned int &lsb, const unsigned int &mask, const unsigned int &value) const {
  return (value & mask) >> lsb;
}

/** readRegisterListFromIndex
*   \brief Read a list of register starting from a specified index.
*   This method will access to the system bus to read several registers.
*   \param[in]  from is the first index of the register to read.
*   \param[in] n is the number of words to read.
*   \param[out] value is the value of registers read.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
**/
unsigned int DrmControllerRegistersBase::readRegisterListFromIndex(const unsigned int &from, const unsigned int &n, std::vector<unsigned int> &value) const {
  value.clear();
  unsigned int end(from+n);
  for (unsigned int ii = from; ii < end; ii++) {
    unsigned int data(0);
    unsigned int errorCode = readRegisterAtIndex(ii, data);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    value.push_back(data);
  }
  return mDrmApi_NO_ERROR;
}

/** readRegisterAtIndex
*   \brief Read a register at a specified index.
*   This method will access to the system bus to read a register.
*   \param[in]  index is the index of the register word to read.
*   \param[out] value is the value of the register read.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
**/
unsigned int DrmControllerRegistersBase::readRegisterAtIndex(const unsigned int &index, unsigned int &value) const {
  return readRegister(registerNameFromIndex(index), value);
}

/** writeRegisterListFromIndex
*   \brief Write a list of register starting from a specified index.
*   This method will access to the system bus to write several registers.
*   \param[in] from is the first index of the register to write.
*   \param[in] n is the number of words to write.
*   \param[in] value is the value of registers to write.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
**/
unsigned int DrmControllerRegistersBase::writeRegisterListFromIndex(const unsigned int &from, const unsigned int &n, const std::vector<unsigned int> &value) const {
  unsigned int ii(from);
  unsigned int end(from+n);
  for (std::vector<unsigned int>::const_iterator it = value.cbegin(); it != value.cend() && ii < end; it++) {
    unsigned int errorCode = writeRegisterAtIndex(ii++, *it);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  return mDrmApi_NO_ERROR;
}

/** writeRegisterAtIndex
*   \brief Write a register at a specified index.
*   This method will access to the system bus to write a register.
*   \param[in] index is the index of the register word to write.
*   \param[in] value is the value of the register to write.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
**/
unsigned int DrmControllerRegistersBase::writeRegisterAtIndex(const unsigned int &index, const unsigned int &value) const {
  // write register at index
  return writeRegister(registerNameFromIndex(index), value);
}

/** registerNameFromIndex
*   \brief Get the register name from index
*   \param[in] index is the register index.
*   \return Returns the name of the register at the specified index.
**/
const std::string DrmControllerRegistersBase::registerNameFromIndex(const unsigned int &index) const {
  std::ostringstream stringStream;
  stringStream << mIndexedRegisterName << index;
  return stringStream.str();
}

/** numberOfWords
*   \brief Get the number of words used by a register.
*   \param[in] registerSize is the size of the register in bits.
*   \return Returns the number of words used for the given register size.
**/
unsigned int DrmControllerRegistersBase::numberOfWords(const unsigned int& registerSize) const {
  if (registerSize <= DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE) {
  return 1;
  }
  else if ((registerSize % DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE) == 0) {
  return registerSize/DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE;
  }
  else {
  return registerSize/DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE + 1;
  }
}

/** throwLicenseFileSizeException
*   \param[in]  expected is the expected size of the license file.
*   \param[in]  actual is the actual size of the license file.
*   \throw Throw DrmControllerLicenseFileSizeException. DrmControllerLicenseFileSizeException::what() should be called to get the exception description.
**/
void DrmControllerRegistersBase::throwLicenseFileSizeException(unsigned int expected, unsigned int actual) const {
  std::ostringstream writter;
  writter << exceptionHeaderDescription("The size of the license file is lower than the minimum required");
  writter << exceptionStatusDescription("Minimum license file size required (in number of 32 bits words)", expected);
  writter << exceptionStatusDescription("Current license file size (in number of 32 bits words)", actual);
  throw DrmControllerLicenseFileSizeException(writter.str());
}

/** throwUnsupportedFeatureException
*   \param[in]  featureName is the name of the feature to be used when throwing unsupported feature error exception.
*   \param[in]  drmVersion is the DRM Controller version to be used when throwing unsupported feature error exception.
*   \throw Throw DrmControllerUnsupportedFeatureException. DrmControllerUnsupportedFeatureException::what() should be called to get the exception description.
**/
void DrmControllerRegistersBase::throwUnsupportedFeatureException(const std::string &featureName, const std::string drmVersion) const {
  throw DrmControllerUnsupportedFeatureException(unsupportedFeatureExceptionDescription(featureName, drmVersion));
}

/** throwFunctionalityDisabledException
*   \param[in]  name is the name of the functionality to be used when throwing functionality disabled error exception.
*   \param[in]  actualStatusMsg is the actual status message to be used when throwing functionality disabled error exception.
*   \param[in]  actualMsg is the actual status message to be used when throwing functionality disabled error exception.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \throw Throw DrmControllerFunctionalityDisabledException. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
**/
void DrmControllerRegistersBase::throwFunctionalityDisabledException(const std::string &name,
                                                                     const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                                     const bool &expectedStatus, const bool &actualStatus) const {
  throw DrmControllerFunctionalityDisabledException(functionalityDisabledExceptionDescription(name, expectedStatusMsg, actualStatusMsg, expectedStatus, actualStatus));
}

/** throwFunctionalityDisabledException
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[in]  actualError is the value of the error read.
*   \param[in]  expectedErrorCodeMsg is the value of the message of the error to be expected.
*   \param[in]  actualErrorCodeMsg is the value of the message of the error read.
*   \throw Throw DrmControllerLicenseTimerResetedException. DrmControllerLicenseTimerResetedException::what() should be called to get the exception description.
**/
void DrmControllerRegistersBase::throwLicenseTimerResetedException(const bool &expectedStatus, const bool &actualStatus,
                                                                   const unsigned char &expectedError, const unsigned char &actualError,
                                                                   const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  std::ostringstream writter;
  writter << exceptionHeaderDescription("DRM Controller License Timer Load is not ready");
  writter << exceptionEmptyHeader() << "The DRM Controller might have been reseted" << exceptionFooter() << std::endl;
  writter << exceptionEmptyHeader() << "The initialization step must be redone by calling initialize function" << exceptionFooter() << std::endl;
  writter << exceptionStatusDescription("Expected License Timer Loaded Status", expectedStatus);
  writter << exceptionErrorDescription("Expected License Timer Loaded Error", expectedError, expectedErrorCodeMsg);
  writter << exceptionStatusDescription("Actual License Timer Loaded Status", actualStatus);
  writter << exceptionErrorDescription("Actual License Timer Loaded Error", actualError, actualErrorCodeMsg);
  throw DrmControllerLicenseTimerResetedException(writter.str());
}

/** throwTimeoutException
*   \brief Throw a timeout exception.
*   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
*   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
*/
void DrmControllerRegistersBase::throwTimeoutException(const std::string &headerMsg) const {
  throw DrmControllerTimeOutException(exceptionHeaderDescription(headerMsg));
}

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
void DrmControllerRegistersBase::throwTimeoutException(const std::string &headerMsg,
                                                       const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                       const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                                       const unsigned int &expectedStatus, const unsigned int &actualStatus,
                                                       const unsigned char &expectedError, const unsigned char &actualError,
                                                       const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  throw DrmControllerTimeOutException(timeoutExceptionDescription(headerMsg, expectedStatusMsg, actualStatusMsg, expectedErrorMsg, actualErrorMsg,
                                                                  expectedStatus, actualStatus, expectedError, actualError, expectedErrorCodeMsg, actualErrorCodeMsg));
}

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
void DrmControllerRegistersBase::throwTimeoutException(const std::string &headerMsg,
                                                       const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                       const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                                       const bool &expectedStatus, const bool &actualStatus,
                                                       const unsigned char &expectedError, const unsigned char &actualError,
                                                       const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  throw DrmControllerTimeOutException(timeoutExceptionDescription(headerMsg, expectedStatusMsg, actualStatusMsg, expectedErrorMsg, actualErrorMsg,
                                                                  expectedStatus, actualStatus, expectedError, actualError, expectedErrorCodeMsg, actualErrorCodeMsg));
}

/** throwTimeoutException
*   \brief Throw a timeout exception.
*   \param[in]  expectedStatusMsg is the expected status message to be used when throwing timeout error exception.
*   \param[in]  actualStatusMsg is the actual status message to be used when throwing timeout error exception.
*   \param[in]  actualMsg is the actual status message to be used when throwing timeout error exception.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
*/
void DrmControllerRegistersBase::throwTimeoutException(const std::string &headerMsg,
                                                       const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                       const unsigned int &expectedStatus, const unsigned int &actualStatus) const {
  throw DrmControllerTimeOutException(timeoutExceptionDescription(headerMsg,
                                                                  expectedStatusMsg, actualStatusMsg,
                                                                  expectedStatus, actualStatus));
}

/** throwTimeoutException
*   \brief Throw a timeout exception.
*   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
*   \param[in]  expectedStatusMsg is the expected status message to be used when throwing timeout error exception.
*   \param[in]  actualStatusMsg is the actual status message to be used when throwing timeout error exception.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
*/
void DrmControllerRegistersBase::throwTimeoutException(const std::string &headerMsg,
                                                       const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                       const bool &expectedStatus, const bool &actualStatus) const {
  throw DrmControllerTimeOutException(timeoutExceptionDescription(headerMsg,
                                                                  expectedStatusMsg, actualStatusMsg,
                                                                  expectedStatus, actualStatus));
}

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
void DrmControllerRegistersBase::throwTimeoutException(const std::string &headerMsg,
                                                       const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                                       const unsigned char &expectedError, const unsigned char &actualError,
                                                       const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  throw DrmControllerTimeOutException(timeoutExceptionDescription(headerMsg, expectedErrorMsg, actualErrorMsg,
                                                                  expectedError, actualError, expectedErrorCodeMsg, actualErrorCodeMsg));
}

/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/

/** unsupportedFeatureExceptionDescription
*   \brief Generate the description of a unsupported feature exception.
*   \param[in]  featureName is the name of the feature to be used with unsupported feature error exception.
*   \param[in]  drmVersion is the DRM Controller version to be used with unsupported feature error exception.
*   \return Returns a string containing the exception description.
*/
std::string DrmControllerRegistersBase::unsupportedFeatureExceptionDescription(const std::string &featureName, const std::string &drmVersion) const {
  std::ostringstream writter;
  writter << exceptionHeaderDescription("Feature is not supported");
  writter << exceptionEmptyHeader() << "The feature" << DRM_CONTROLLER_ERROR_SPACE << featureName << DRM_CONTROLLER_ERROR_SPACE;
  writter << "is not supported in the version" << DRM_CONTROLLER_ERROR_SPACE << drmVersion << DRM_CONTROLLER_ERROR_SPACE << "of the DRM Controller" << exceptionFooter() << std::endl;
  return writter.str();
}

/** functionalityDisabledExceptionDescription
*   \param[in]  name is the name of the functionality to be used with functionality disabled error exception.
*   \param[in]  actualStatusMsg is the actual status message to be used with functionality disabled error exception.
*   \param[in]  actualMsg is the actual status message to be used with functionality disabled error exception.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \return Returns a string containing the exception description.
**/
std::string DrmControllerRegistersBase::functionalityDisabledExceptionDescription(const std::string &name, const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                                                  const bool &expectedStatus, const bool &actualStatus) const {
  std::ostringstream writter;
  writter << exceptionHeaderDescription("Functionality is disabled");
  writter << exceptionEmptyHeader() << "The functionality" << DRM_CONTROLLER_ERROR_SPACE << name << DRM_CONTROLLER_ERROR_SPACE << "is disabled" << exceptionFooter() << std::endl;
  writter << exceptionStatusDescription(expectedStatusMsg, expectedStatus);
  writter << exceptionStatusDescription(actualStatusMsg, actualStatus);
  return writter.str();
}

/** timeoutExceptionDescription
*   \brief Generate the description of a timeout exception.
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
std::string DrmControllerRegistersBase::timeoutExceptionDescription(const std::string &headerMsg,
                                                                    const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                                    const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                                                    const unsigned int &expectedStatus, const unsigned int &actualStatus,
                                                                    const unsigned char &expectedError, const unsigned char &actualError,
                                                                    const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  std::ostringstream writter;
  writter << timeoutExceptionDescription(headerMsg, expectedStatusMsg, actualStatusMsg, expectedStatus, actualStatus);
  writter << timeoutExceptionDescription("", expectedErrorMsg, actualErrorMsg, expectedError, actualError, expectedErrorCodeMsg, actualErrorCodeMsg);
  return writter.str();
}

/** timeoutExceptionDescription
*   \brief Generate the description of a timeout exception.
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
std::string DrmControllerRegistersBase::timeoutExceptionDescription(const std::string &headerMsg,
                                                                    const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                                    const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                                                    const bool &expectedStatus, const bool &actualStatus,
                                                                    const unsigned char &expectedError, const unsigned char &actualError,
                                                                    const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  std::ostringstream writter;
  writter << timeoutExceptionDescription(headerMsg, expectedStatusMsg, actualStatusMsg, expectedStatus, actualStatus);
  writter << timeoutExceptionDescription("", expectedErrorMsg, actualErrorMsg, expectedError, actualError, expectedErrorCodeMsg, actualErrorCodeMsg);
  return writter.str();
}

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
std::string DrmControllerRegistersBase::timeoutExceptionDescription(const std::string &headerMsg,
                                                                    const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                                                    const unsigned char &expectedError, const unsigned char &actualError,
                                                                    const std::string &expectedErrorCodeMsg, const std::string &actualErrorCodeMsg) const {
  std::ostringstream writter;
  if (headerMsg.empty() == false)
    writter << exceptionHeaderDescription(headerMsg);
  writter << exceptionErrorDescription(expectedErrorMsg, expectedError, expectedErrorCodeMsg);
  writter << exceptionErrorDescription(actualErrorMsg, actualError, actualErrorCodeMsg);
  return writter.str();
}

/** timeoutExceptionDescription
*   \brief Generate the description of a timeout exception.
*   \param[in]  headerMsg is the header message to be used with timeout error exception.
*   \param[in]  expectedStatusMsg is the expected status message to be used with timeout error exception.
*   \param[in]  actualStatusMsg is the actual status message to be used with timeout error exception.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \return Returns a string containing the exception description.
*/
std::string DrmControllerRegistersBase::timeoutExceptionDescription(const std::string &headerMsg,
                                                                    const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                                    const unsigned int &expectedStatus, const unsigned int &actualStatus) const {
  std::ostringstream writter;
  writter << exceptionHeaderDescription(headerMsg);
  writter << exceptionStatusDescription(expectedStatusMsg, expectedStatus);
  writter << exceptionStatusDescription(actualStatusMsg, actualStatus);
  return writter.str();
}

/** timeoutExceptionDescription
*   \brief Generate the description of a timeout exception.
*   \param[in]  headerMsg is the header message to be used with timeout error exception.
*   \param[in]  expectedStatusMsg is the expected status message to be used with timeout error exception.
*   \param[in]  actualStatusMsg is the actual status message to be used with timeout error exception.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \return Returns a string containing the exception description.
*/
std::string DrmControllerRegistersBase::timeoutExceptionDescription(const std::string &headerMsg,
                                                                    const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                                    const bool &expectedStatus, const bool &actualStatus) const {
  std::ostringstream writter;
  writter << exceptionHeaderDescription(headerMsg);
  writter << exceptionStatusDescription(expectedStatusMsg, expectedStatus);
  writter << exceptionStatusDescription(actualStatusMsg, actualStatus);
  return writter.str();
}

/** exceptionHeaderDescription
*   \brief Generate the header description of an exception.
*   \param[in]  headerMsg is the header message to be used with error exception.
*   \return Returns a string containing the exception header description.
*/
std::string DrmControllerRegistersBase::exceptionHeaderDescription(const std::string &headerMsg) const {
  std::ostringstream writter;
  writter << exceptionHeader() << headerMsg << exceptionFooter() << std::endl;
  return writter.str();
}

/** exceptionStatusDescription
*   \brief Generate the status description of an exception.
*   \param[in]  statusMsg is the status message to be used with error exception.
*   \param[in]  status is the value of the status to be used.
*   \return Returns a string containing the exception status description.
*/
std::string DrmControllerRegistersBase::exceptionStatusDescription(const std::string &statusMsg, const unsigned int &status) const {
  std::ostringstream writter;
  writter << exceptionEmptyHeader() << statusMsg << DRM_CONTROLLER_ERROR_COLON << DRM_CONTROLLER_ERROR_SPACE << hexadecimalDescription(status) << exceptionFooter() << std::endl;
  return writter.str();
}

/** exceptionStatusDescription
*   \brief Generate the status description of an exception.
*   \param[in]  statusMsg is the status message to be used with error exception.
*   \param[in]  status is the value of the status to be used.
*   \return Returns a string containing the exception status description.
*/
std::string DrmControllerRegistersBase::exceptionStatusDescription(const std::string &statusMsg, const bool &status) const {
  std::ostringstream writter;
  writter << exceptionEmptyHeader() << statusMsg << DRM_CONTROLLER_ERROR_COLON << DRM_CONTROLLER_ERROR_SPACE << binaryDescription(status) << exceptionFooter() << std::endl;
  return writter.str();
}

/** exceptionErrorDescription
*   \brief Generate the error description of an exception.
*   \param[in]  errorMsg is the error message to be used with error exception.
*   \param[in]  error is the value of the error to be used.
*   \param[in]  errorCodeMsg is the value of the message of the error.
*   \return Returns a string containing the exception error description.
*/
std::string DrmControllerRegistersBase::exceptionErrorDescription(const std::string &errorMsg, const unsigned char &error, const std::string &errorCodeMsg) const {
  std::ostringstream writter;
  writter << exceptionEmptyHeader() << errorMsg << DRM_CONTROLLER_ERROR_COLON << DRM_CONTROLLER_ERROR_SPACE << errorCodeMsg << " (" << hexadecimalDescription(error) << ") " << exceptionFooter() << std::endl;
  return writter.str();
}

/** binaryDescription
*   \brief Generate the description of a binary value.
*   \param[in] binary is the binary value to be used in the description.
*   \return Returns a string containing the description of a binary value.
**/
std::string DrmControllerRegistersBase::binaryDescription(const bool &binary) const {
  std::ostringstream writter;
  writter << DRM_CONTROLLER_ERROR_BIN_PREFIX << binary;
  return writter.str();
}

/** hexadecimalDescription
*   \brief Generate the description of a hexadecimal value.
*   \param[in] hexadecimal is the hexadecimal value to be used in the description.
*   \return Returns a string containing the description of a hexadecimal value.
**/
std::string DrmControllerRegistersBase::hexadecimalDescription(const unsigned int &hexadecimal) const {
  std::ostringstream writter;
  writter << DRM_CONTROLLER_ERROR_HEX_PREFIX << std::setfill(DRM_CONTROLLER_ERROR_HEX_FILL) << std::setw(sizeof(unsigned int)*(DRM_CONTROLLER_BYTE_SIZE/DRM_CONTROLLER_NIBBLE_SIZE)) << std::hex << hexadecimal;
  return writter.str();
}

/** hexadecimalDescription
*   \brief Generate the description of a hexadecimal value.
*   \param[in] hexadecimal is the hexadecimal value to be used in the description.
*   \return Returns a string containing the description of a hexadecimal value.
**/
std::string DrmControllerRegistersBase::hexadecimalDescription(const unsigned char &hexadecimal) const {
  std::ostringstream writter;
  writter << DRM_CONTROLLER_ERROR_HEX_PREFIX << std::setfill(DRM_CONTROLLER_ERROR_HEX_FILL) << std::setw(sizeof(unsigned char)*(DRM_CONTROLLER_BYTE_SIZE/DRM_CONTROLLER_NIBBLE_SIZE)) << std::hex << (int)hexadecimal;
  return writter.str();
}

/** exceptionHeader
*   \brief Generate the header of an exception.
*   \return Returns a string containing the exception header.
*/
std::string DrmControllerRegistersBase::exceptionHeader() const {
  return std::string(DRM_CONTROLLER_ERROR_HEADER);
}

/** exceptionEmptyHeader
*   \brief Generate the empty header of an exception.
*   \return Returns a string containing the exception empty header.
*/
std::string DrmControllerRegistersBase::exceptionEmptyHeader() const {
  std::ostringstream writter;
  writter << std::right << std::setfill(DRM_CONTROLLER_ERROR_SPACE) << std::setw(strlen(DRM_CONTROLLER_ERROR_HEADER)) << DRM_CONTROLLER_ERROR_SPACE;
  return writter.str();
}

/** exceptionFooter
*   \brief Generate the footer of an exception.
*   \return Returns a string containing the exception footer.
*/
std::string DrmControllerRegistersBase::exceptionFooter() const {
  return std::string(DRM_CONTROLLER_ERROR_FOOTER);
}
