/**
*  \file      DrmControllerRegistersStrategyInterface.cpp
*  \version   4.2.0.0
*  \date      July 2020
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

#include <HAL/DrmControllerRegistersStrategyInterface.hpp>

// name space usage
using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** DrmControllerRegistersStrategyInterface
*   \brief Class constructor.
*   \param[in] readRegisterFunction function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] writeRegisterFunction function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
**/
DrmControllerRegistersStrategyInterface::DrmControllerRegistersStrategyInterface(tDrmReadRegisterFunction readRegisterFunction,
                                                                                 tDrmWriteRegisterFunction writeRegisterFunction)
: DrmControllerRegistersBase(readRegisterFunction, writeRegisterFunction)
{
    // Set the sleep period from environment variable if existing, use default value otherwise.
    const char* sleep = std::getenv("DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS");
    mSleepInMicroSeconds = (sleep == NULL) ? DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS : std::stoul(std::string(sleep));
}

/** ~DrmControllerRegistersStrategyInterface
*   \brief Class destructor.
**/
DrmControllerRegistersStrategyInterface::~DrmControllerRegistersStrategyInterface()
{}

/** readLicenseStartAddressRegister
*   \brief Read the value of the license start address.
*   This method will access to the system bus to read the license start address register.
*   \param[in] licenseStartAddress is the license start address value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readLicenseStartAddressRegister(std::string &licenseStartAddress) const {
  std::vector<unsigned int> licenseStartAddressList;
  unsigned int errorCode = readLicenseStartAddressRegister(licenseStartAddressList);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  licenseStartAddress = DrmControllerDataConverter::binaryToHexString(licenseStartAddressList);
  return errorCode;
}

/** writeLicenseStartAddressRegister
*   \brief Write the value of the license start address.
*   This method will access to the system bus to write the license start address register.
*   \param[in] licenseStartAddress is the license start address value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::writeLicenseStartAddressRegister(const std::string &licenseStartAddress) const {
  return writeLicenseStartAddressRegister(DrmControllerDataConverter::hexStringToBinary(licenseStartAddress));
}

/** readLicenseTimerInitRegister
*   \brief Read the value of the license timer.
*   This method will access to the system bus to read the license timer register.
*   \param[out] licenseTimerInit is the license timer value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readLicenseTimerInitRegister(std::string &licenseTimerInit) const {
  std::vector<unsigned int> licenseTimerInitList;
  unsigned int errorCode = readLicenseTimerInitRegister(licenseTimerInitList);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  licenseTimerInit = DrmControllerDataConverter::binaryToHexString(licenseTimerInitList);
  return errorCode;
}

/** writeLicenseTimerInitRegister
*   \brief Write the value of the license timer.
*   This method will access to the system bus to write the license timer register.
*   \param[in] licenseTimerInit is the license timer value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::writeLicenseTimerInitRegister(const std::string &licenseTimerInit) const {
  return writeLicenseTimerInitRegister(DrmControllerDataConverter::hexStringToBinary(licenseTimerInit));
}

/** checkMeteringEnabledStatusRegister
*   \brief Read the status register and check the metering enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException if the metering is disabled. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::checkMeteringEnabledStatusRegister(bool &meteringEnabled) const {
  unsigned int errorCode = readMeteringEnabledStatusRegister(meteringEnabled);
  if (errorCode != mDrmApi_NO_ERROR || meteringEnabled == true) return errorCode;
  throwFunctionalityDisabledException("Metering", "Expected Metering Enabled Status", "Actual Metering Enabled Status", true, meteringEnabled);
  return mDrmApi_METERING_DISABLED_ERROR;
}

/** checkLicenseTimerEnabledStatusRegister
*   \brief Read the status register and check license timer enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException if the metering is disabled. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::checkLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const {
  unsigned int errorCode = readLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  if (errorCode != mDrmApi_NO_ERROR || licenseTimerEnabled == true) return errorCode;
  throwFunctionalityDisabledException("License Timer", "Expected License Timer Enabled Status", "Actual License Timer Enabled Status", true, licenseTimerEnabled);
  return mDrmApi_LICENSE_TIMER_DISABLED_ERROR;
}

/** readDnaRegister
*   \brief Read the dna register and get the value.
*   This method will access to the system bus to read the dna register.
*   \param[out] dna is the dna value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readDnaRegister(std::string &dna) const {
  std::vector<unsigned int> tmpDna;
  unsigned int errorCode = readDnaRegister(tmpDna);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  dna = DrmControllerDataConverter::binaryToHexString(tmpDna);
  return errorCode;
}

/** readSaasChallengeRegister
*   \brief Read the saas challenge register and get the value.
*   This method will access to the system bus to read the saas challenge register.
*   \param[out] saasChallenge is the saas challenge value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readSaasChallengeRegister(std::string &saasChallenge) const {
  std::vector<unsigned int> tmpChallenge;
  unsigned int errorCode = readSaasChallengeRegister(tmpChallenge);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  saasChallenge = DrmControllerDataConverter::binaryToHexString(tmpChallenge);
  return errorCode;
}

/** readAdaptativeProportionTestFailuresRegister
*   \brief Read the Adaptative Proportion Test Failures register and get the value.
*   This method will access to the system bus to read the Adaptative Proportion Test Failures register.
*   \param[out] adaptativeProportionTestFailures is the Adaptative Proportion Test Failures value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readAdaptativeProportionTestFailuresRegister(std::string &adaptativeProportionTestFailures) const {
  std::vector<unsigned int> tmpAdaptativeProportionTestFailures;
  unsigned int errorCode = readAdaptativeProportionTestFailuresRegister(tmpAdaptativeProportionTestFailures);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  adaptativeProportionTestFailures = DrmControllerDataConverter::binaryToHexString(tmpAdaptativeProportionTestFailures);
  return errorCode;
}

/** readRepetitionCountTestFailuresRegister
*   \brief Read the Repetition Count Test Failures register and get the value.
*   This method will access to the system bus to read the Repetition Count Test Failures register.
*   \param[out] repetitionCountTestFailures is the Repetition Count Test Failures value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readRepetitionCountTestFailuresRegister(std::string &repetitionCountTestFailures) const {
  std::vector<unsigned int> tmpRepetitionCountTestFailures;
  unsigned int errorCode = readRepetitionCountTestFailuresRegister(tmpRepetitionCountTestFailures);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  repetitionCountTestFailures = DrmControllerDataConverter::binaryToHexString(tmpRepetitionCountTestFailures);
  return errorCode;
}

/** readLicenseTimerCounterRegister
*   \brief Read the license timer counter register and get the value.
*   This method will access to the system bus to read the license timer counter register.
*   \param[out] licenseTimerCounter is the license timer counter value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readLicenseTimerCounterRegister(std::string &licenseTimerCounter) const {
  std::vector<unsigned int> tmpLicenseTimerCounter;
  unsigned int errorCode = readLicenseTimerCounterRegister(tmpLicenseTimerCounter);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  licenseTimerCounter = DrmControllerDataConverter::binaryToHexString(tmpLicenseTimerCounter);
  return errorCode;
}

/** readDrmVersionRegister
*   \brief Read the drm version register and get the value.
*   This method will access to the system bus to read the drm version register.
*   \param[out] drmVersion is the drm version value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readDrmVersionRegister(std::string &drmVersion) const {
  unsigned int tmpVersion;
  unsigned int errorCode = readDrmVersionRegister(tmpVersion);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  drmVersion = DrmControllerDataConverter::binaryToHexString(tmpVersion);
  drmVersion = drmVersion.substr(drmVersion.size()-DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT_BYTES, DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT_BYTES);
  return errorCode;
}

/** readLogsRegister
*   \brief Read the logs register and get the value.
*   This method will access to the system bus to read the logs register.
*   \param[in] numberOfIPs is the total number of IPs.
*   \param[out] logs is the logs value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readLogsRegister(const unsigned int &numberOfIPs, std::string &logs) const {
  std::vector<unsigned int> tmpLogs;
  unsigned int errorCode = readLogsRegister(numberOfIPs, tmpLogs);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  logs = DrmControllerDataConverter::binaryToBinString(tmpLogs);
  logs = logs.substr(logs.size()-numberOfIPs, numberOfIPs);
  return errorCode;
}

/** printHwReport
* \brief Print all register content accessible through AXI-4 Lite Control channel.
* \param[in] file: Reference to output file where register contents are saved. By default print on standard output
*/
void DrmControllerRegistersStrategyInterface::printHwReport(std::ostream &file) const {
  file << header() << std::endl;
  file << componentName("DRM CONTROLLER") << std::endl;
  printDrmVersionHwReport(file);
  printPageHwReport(file);
  printCommandHwReport(file);
  printLicenseStartAddressHwReport(file);
  printLicenseTimerHwReport(file);
  printStatusHwReport(file);
  printErrorHwReport(file);
  printDnaHwReport(file);
  printSaasChallengeHwReport(file);
  printLicenseTimerCounterHwReport(file);
  printLogsHwReport(file);
  printVlnvFileHwReport(file);
  printLicenseFileHwReport(file);
  printTraceFileHwReport(file);
  printMeteringFileHwReport(file);
  printMailBoxFileHwReport(file);
  printAdaptativeProportionTestFailuresHwReport(file);
  printRepetitionCountTestFailuresHwReport(file);
  file << std::endl << footer() << std::endl << std::endl;
}

/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/** readStatusRegister
*   \brief Read the value of a several and contigous status bits.
*   This method will access to the system bus to read the status register.
*   \param[in] bitPosition is the lsb position of the status bits.
*   \param[in] mask is the mask of the status bits.
*   \param[out] value is the value of the status bit.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readStatusRegister(const unsigned int &bitPosition, const unsigned int &mask, unsigned int &value) const {
  unsigned int status;
  unsigned int errorCode = readStatusRegister(status);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  value = bits(bitPosition, mask, status);
  return errorCode;
}

/** readStatusRegister
*   \brief Read the value of a specific status bit.
*   This method will access to the system bus to read the status register.
*   \param[in] bitPosition is the position of the status bit.
*   \param[in] mask is the mask of the status bit.
*   \param[out] value is the value of the status bit.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readStatusRegister(const unsigned int &bitPosition, const unsigned int &mask, bool &value) const {
  unsigned int status;
  unsigned int errorCode = readStatusRegister(bitPosition, mask, status);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // check if the lsb is set to 1
  value = (status == 1 ? true : false);
  return errorCode;
}

/** waitStatusRegister
*   \brief Wait status to reach specified value.
*   This method will access to the system bus to read the status register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  bitPosition is the position of the status bit.
*   \param[in]  mask is the mask of the status bit.
*   \param[in]  expected is the value of the status to be expected.
*   \param[out] actual is the value of the status bit read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::waitStatusRegister(const unsigned int &timeout, const unsigned int &bitPosition, const unsigned int &mask, const bool &expected, bool &actual) const {
  unsigned int actualStatus;
  unsigned int errorCode = waitStatusRegister(timeout, bitPosition, mask, (expected == true ? 1 : 0), actualStatus);
  actual = (actualStatus == 1 ? true : false);
  return errorCode;
}

/** waitStatusRegister
*   \brief Wait status to reach specified value.
*   This method will access to the system bus to read the status register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  bitPosition is the position of the msb of the status.
*   \param[in]  mask is the mask of the status.
*   \param[in]  expected is the value of the status to be expected.
*   \param[out] actual is the value of the status read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::waitStatusRegister(const unsigned int &timeout, const unsigned int &bitPosition, const unsigned int &mask, const unsigned int &expected, unsigned int &actual) const {
  unsigned int errorCode = writeRegistersPageRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get current time
  timeval startTimePoint, currentTimePoint;
  gettimeofday(&startTimePoint, NULL);
  // loop while expected status and error not reached
  unsigned int timeTaken = 0;
  do {
    // read status
    errorCode = readStatusRegister(bitPosition, mask, actual);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    // get current time
    gettimeofday(&currentTimePoint, NULL);
    // get time duration
    timeTaken = (unsigned int)(currentTimePoint.tv_sec - startTimePoint.tv_sec)*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND +
                              (currentTimePoint.tv_usec - startTimePoint.tv_usec);
    // check timeout reached
    if (timeout > 0 && timeTaken > timeout)
      // return timeout error
      return mDrmApi_HARDWARE_TIMEOUT_ERROR;
    // exit loop when expected status is reached
    if (actual == expected)
      break;
    // sleep
    usleep(mSleepInMicroSeconds);
  } while (true);
  return errorCode;
}

/** readErrorRegister
*   \brief Get the value of a specific error byte.
*   This method will access to the system bus to read the error register.
*   \param[in] position is the position of the error byte.
*   \param[in] mask is the mask of the error byte.
*   \param[out] error is the value of the error byte.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegistersStrategyInterface::readErrorRegister(const unsigned int &position, const unsigned int &mask, unsigned char &error) const {
  unsigned int errorRead;
  unsigned int errorCode = readErrorRegister(errorRead);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  error = bits(position, mask, errorRead);
  return errorCode;
}

/** waitErrorRegister
*   \brief Wait error to reach specified value.
*   This method will access to the system bus to read the error register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in] position is the position of the error byte.
*   \param[in] mask is the mask of the error byte.
*   \param[in]  expected is the value of the error to be expected.
*   \param[out] actual is the value of the error read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
**/
unsigned int DrmControllerRegistersStrategyInterface::waitErrorRegister(const unsigned int &timeout, const unsigned int &position, const unsigned int &mask, const unsigned char &expected, unsigned char &actual) const {
  unsigned int errorCode = writeRegistersPageRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get current time
  timeval startTimePoint, currentTimePoint;
  gettimeofday(&startTimePoint, NULL);
  // loop while expected status and error not reached
  unsigned int timeTaken = 0;
  do {
    // read status
    errorCode = readErrorRegister(position, mask, actual);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    // get current time
    gettimeofday(&currentTimePoint, NULL);
    // get time duration
    timeTaken = (unsigned int)(currentTimePoint.tv_sec - startTimePoint.tv_sec)*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND +
                              (currentTimePoint.tv_usec - startTimePoint.tv_usec);
    // check timeout reached
    if (timeout > 0 && timeTaken > timeout)
      // return timeout error
      return mDrmApi_HARDWARE_TIMEOUT_ERROR;
    // exit loop when expected status is reached
    if (actual == expected)
      break;
    // sleep
    usleep(mSleepInMicroSeconds);
  } while (true);
  return errorCode;
}

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/
