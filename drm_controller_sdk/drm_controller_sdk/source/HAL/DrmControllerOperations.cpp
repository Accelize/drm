/**
*  \file      DrmControllerOperations.cpp
*  \version   3.0.0.1
*  \date      September 2018
*  \brief     Class DrmControllerOperations is an abstraction level to execute operations.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#include <HAL/DrmControllerOperations.hpp>

/** DrmControllerOperations
*   \brief Class constructor.
*   \param[in] f_read_reg32 function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] f_write_reg32 function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
**/
DrmControllerLibrary::DrmControllerOperations::DrmControllerOperations(t_drmReadRegisterFunction f_read_reg32,
                                                                       t_drmWriteRegisterFunction f_write_reg32)
  : DrmControllerRegisters::DrmControllerRegisters(f_read_reg32,
                                                   f_write_reg32),
    mHeartBeatModeEnabled(false),
    mLicenseTimerWasLoaded(false)
{
  // check version
  DrmControllerOperations::checkVersion();
  // wait controller done for heart beat mode detection
  DrmControllerOperations::waitAutonomousControllerDone();
}

/** ~DrmController
*   \brief Class destructor.
**/
DrmControllerLibrary::DrmControllerOperations::~DrmControllerOperations() { }

/** waitAutoControllerDone
*   \brief Wait until the Autonomous Controller is done.
*   This method will access to the system bus to check the autonomous controller status.
*   \param[out] heartBeatModeEnabled is the deducted value of the heart beat mode of the drm controller.
*   \param[out] autoEnabled is the value of the status bit auto controller enabled.
*   \param[out] autoBusy is the value of the status bit auto controller busy.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitAutonomousControllerDone(bool &heartBeatModeEnabled,
                                                                                    bool &autoEnabled,
                                                                                    bool &autoBusy) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get auto controller enabled status
  errorCode = DrmControllerRegisters::readAutonomousControllerEnabledStatusRegister(autoEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check auto controller enabled
  if (autoEnabled == true) {
    // get current time
    timeval startTimePoint, currentTimePoint;
    gettimeofday(&startTimePoint, NULL);
    // loop while expected status and error not reached
    unsigned int timeTaken = 0;
    // loop while auto controller busy or heart beat mode enabled and timeout not reached
    do {
      // get status busy
      errorCode = DrmControllerRegisters::readAutonomousControllerBusyStatusRegister(autoBusy);
      // check no error
      if (errorCode != mDrmApi_NO_ERROR)
        // return error result
        return errorCode;
      // exit loop if controller not busy
      if (autoBusy == false)
        break;
      // dna ready, vlnv ready and activation done statuses
      bool dnaReady(false), vlnvReady(false), activationDone(false);
      // get dna ready
      errorCode = DrmControllerRegisters::readDnaReadyStatusRegister(dnaReady);
      // check no error
      if (errorCode != mDrmApi_NO_ERROR)
        // return error result
        return errorCode;
      // get vlnv ready
      errorCode = DrmControllerRegisters::readVlnvReadyStatusRegister(vlnvReady);
      // check no error
      if (errorCode != mDrmApi_NO_ERROR)
        // return error result
        return errorCode;
      // get activation done
      errorCode = DrmControllerRegisters::readActivationDoneStatusRegister(activationDone);
      // check no error
      if (errorCode != mDrmApi_NO_ERROR)
        // return error result
        return errorCode;
      // enable heart beat mode status and exit loop when operations are done
      if (dnaReady == true && vlnvReady == true && activationDone == true) {
        heartBeatModeEnabled = true;
        break;
      }
      // get current time
      gettimeofday(&currentTimePoint, NULL);
      // get time duration
      timeTaken = (unsigned int)(currentTimePoint.tv_sec - startTimePoint.tv_sec)*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND +
                                (currentTimePoint.tv_usec - startTimePoint.tv_usec);
      // exit loop when timeout reached
      if (DRM_CONTROLLER_AUTONOMOUS_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS > 0 && timeTaken > DRM_CONTROLLER_AUTONOMOUS_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS)
        break;
    } while (true); // do
    // return error when timeout reached
    if (DRM_CONTROLLER_AUTONOMOUS_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS > 0 && timeTaken > DRM_CONTROLLER_AUTONOMOUS_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS) {
      // build the exception message
      std::ostringstream stringStream;
      stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_INIT_AFTER_RESET_TIMEOUT_ERROR << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
      // throw exception
      throw DrmControllerTimeOutException(stringStream.str());
      // return time out error code
      return mDrmApi_HARDWARE_TIMEOUT_ERROR;
    }
  } // autoEnabled == true
  // return error result
  return errorCode;
}

/** waitAutoControllerDone
*   \brief Wait until the Autonomous Controller is done.
*   This method will access to the system bus to check the autonomous controller status.
*   \param[out] autoEnabled is the value of the status bit auto controller enabled.
*   \param[out] autoBusy is the value of the status bit auto controller busy.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitAutonomousControllerDone(bool &autoEnabled,
                                                                                    bool &autoBusy) {
  // wait auto controller done
  return DrmControllerOperations::waitAutonomousControllerDone(mHeartBeatModeEnabled,
                                                                   autoEnabled,
                                                                   autoBusy);
}

/** waitAutoControllerDone
*   \brief Wait until the Autonomous Controller is done.
*   This method will access to the system bus to check the autonomous controller status.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitAutonomousControllerDone() {
  // status
  bool autoEnabled(false), autoBusy(false);
  // wait controller done
  return DrmControllerOperations::waitAutonomousControllerDone(autoEnabled,
                                                                   autoBusy);
}

/** extractDrmVersion
*   \brief Extract the version of the DRM controller.
*   This method will access to the system bus to extract the drm version.
*   \param[out] drmVersion is the value of the drm version.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractDrmVersion(std::string &drmVersion) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read version
  return DrmControllerOperations::readDrmVersionRegister(drmVersion);
}

/** extractDna
*   \brief Extract the dna value.
*   This method will access to the system bus to extract the dna value.
*   \param[out] dna is the value of the dna.
*   \param[out] dnaReady is the value of the status bit dna ready.
*   \param[out] dnaErrorCode is the value of the error code related to dna extraction.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractDna(std::string &dna,
                                                                  bool &dnaReady,
                                                                  unsigned char &dnaErrorCode) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check heart beat mode is disabled
  if (mHeartBeatModeEnabled == false) {
    // set nop command
    errorCode = DrmControllerRegisters::writeNopCommandRegister();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
    // set dna extract command
    errorCode = DrmControllerRegisters::writeDnaExtractCommandRegister();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
  } // mHeartBeatModeEnabled == false
  // wait dna extract status and error
  errorCode = DrmControllerOperations::waitDnaExtractStatusAndError(DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_IN_MICRO_SECONDS,
                                                                        true,
                                                                        mDrmErrorNoError,
                                                                        dnaReady,
                                                                        dnaErrorCode);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_STATUS << dnaReady << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_ERROR_CODE;
    stringStream << std::setfill ('0') << std::setw(2) << std::hex << (int)dnaErrorCode << std::dec << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_ERROR_MESSAGE;
    stringStream << DrmControllerOperations::getDrmErrorRegisterMessage(dnaErrorCode) << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read dna
  return DrmControllerOperations::readDnaRegister(dna);
}

/** extractDna
*   \brief Extract the dna value.
*   This method will access to the system bus to extract the dna value.
*   \param[out] dna is the value of the dna.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractDna(std::string &dna) const {
  // status and error
  bool dnaReady(false);
  unsigned char dnaErrorCode(mDrmErrorNotReady);
  // extract dna
  return DrmControllerOperations::extractDna(dna,
                                                 dnaReady,
                                                 dnaErrorCode);
}

/** extractVlnvFile
*   \brief Extract the vlnv file.
*   This method will access to the system bus to extract the vlnv file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] vlnvFile is the value of the vlnv file,
*               including the vlnv of the drm controller at the first position.
*   \param[out] vlnvReady is the value of the status bit vlnv ready.
*   \param[out] vlnvErrorCode is the value of the error code related to vlnv extraction.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractVlnvFile(unsigned int &numberOfDetectedIps,
                                                                       std::vector<std::string> &vlnvFile,
                                                                       bool &vlnvReady,
                                                                       unsigned char &vlnvErrorCode) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check heart beat mode is disabled
  if (mHeartBeatModeEnabled == false) {
    // set nop command
    errorCode = DrmControllerRegisters::writeNopCommandRegister();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
    // set vlnv extract command
    errorCode = DrmControllerRegisters::writeVlnvExtractCommandRegister();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
  }
  // wait vlnv extract status and error
  errorCode = DrmControllerOperations::waitVlnvExtractStatusAndError(DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_IN_MICRO_SECONDS,
                                                                         true,
                                                                         mDrmErrorNoError,
                                                                         vlnvReady,
                                                                         vlnvErrorCode);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_STATUS << vlnvReady << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_ERROR_CODE;
    stringStream << std::setfill ('0') << std::setw(2) << std::hex << (int)vlnvErrorCode << std::dec << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_ERROR_MESSAGE;
    stringStream << DrmControllerOperations::getDrmErrorRegisterMessage(vlnvErrorCode) << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to vlnv file
  errorCode = DrmControllerRegisters::writeVlnvFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get vlnv file
  return DrmControllerOperations::readVlnvFileRegister(numberOfDetectedIps, vlnvFile);
}

/** extractVlnvFile
*   \brief Extract the vlnv file.
*   This method will access to the system bus to extract the vlnv file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] vlnvFile is the value of the vlnv file,
*               including the vlnv of the drm controller at the first position.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractVlnvFile(unsigned int &numberOfDetectedIps,
                                                                       std::vector<std::string> &vlnvFile) const {
  // status and error
  bool vlnvReady(false);
  unsigned char vlnvErrorCode(mDrmErrorNotReady);
  // extract vlnv file
  return DrmControllerOperations::extractVlnvFile(numberOfDetectedIps,
                                                      vlnvFile,
                                                      vlnvReady,
                                                      vlnvErrorCode);
}

/** initialization
*   \brief Make the initialization step.
*   This method will access to the system bus to read the status, saas challenge and metering file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::initialization(unsigned int &numberOfDetectedIps,
                                                                      std::string &saasChallenge,
                                                                      std::vector<std::string> &meteringFile,
                                                                      bool &meteringEnabled,
                                                                      bool &saasChallengeReady,
                                                                      bool &meteringReady) {
  // the license timer is not loaded
  mLicenseTimerWasLoaded = false;
  // call extract metering file and saas challenge
  return DrmControllerOperations::extractMeteringFileAndSaasChallenge(numberOfDetectedIps,
                                                                          saasChallenge,
                                                                          meteringFile,
                                                                          meteringEnabled,
                                                                          saasChallengeReady,
                                                                          meteringReady);
}

/** initialization
*   \brief Make the initialization step.
*   This method will access to the system bus to read the status, saas challenge and metering file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::initialization(unsigned int &numberOfDetectedIps,
                                                                      std::string &saasChallenge,
                                                                      std::vector<std::string> &meteringFile) {
  // status bits
  bool meteringEnabled(false), saasChallengeReady(false), meteringReady(false);
  // extract metering file
  return DrmControllerOperations::initialization(numberOfDetectedIps,
                                                     saasChallenge,
                                                     meteringFile,
                                                     meteringEnabled,
                                                     saasChallengeReady,
                                                     meteringReady);
}

/** loadLicenseTimerInit
**  \brief Load the license timer value
*   This method will access to the system bus to read the status and the error, and write the license timer.
*   \param[in] licenseTimerInit is the value of the license timer.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_RESETED_ERROR if the DRM Controller has been reseted,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseTimerResetedException whenever the license timer has been reseted. DrmControllerLicenseTimerResetedException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::loadLicenseTimerInit(const std::string &licenseTimerInit,
                                                                            bool &licenseTimerEnabled) {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read license timer enabled status
  errorCode = DrmControllerRegisters::readLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check license timer enabled status
  if (licenseTimerEnabled == false) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_LICENSE_TIMER_DISABLED_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_DISABLED_STATUS << licenseTimerEnabled << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerFunctionalityDisabledException(stringStream.str());
    // return time out error code
    return mDrmApi_LICENSE_TIMER_DISABLED_ERROR;
  }
  // check license timer if it was loaded the previous call
  if (mLicenseTimerWasLoaded == true) {
    errorCode = DrmControllerOperations::checkLicenseTimerInitLoaded();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR) {
      // license timer was reseted
      mLicenseTimerWasLoaded = false;
      // return error result
      return errorCode;
    }
  }
  // write license timer value
  errorCode = DrmControllerOperations::writeLicenseTimerInitRegister(licenseTimerInit);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // everything was fine, the license timer has been loaded
  mLicenseTimerWasLoaded = true;
  // return error result
  return errorCode;
}

/** loadLicenseTimerInit
**  \brief Load the license timer value
*   This method will access to the system bus to read the status and the error, and write the license timer.
*   \param[in] licenseTimerInit is the value of the license timer.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_RESETED_ERROR if the DRM Controller has been reseted,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseTimerResetedException whenever the license timer has been reseted. DrmControllerLicenseTimerResetedException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::loadLicenseTimerInit(const std::string &licenseTimerInit) {
  // status
  bool licenseTimerEnabled(false);
  // load license timer
  return DrmControllerOperations::loadLicenseTimerInit(licenseTimerInit,
                                                           licenseTimerEnabled);
}

/** activate
*   \brief Launch the activation procedure.
*   This method will access to the system bus to write the license file and launch the activation.
*   \param[in] licenseFile is the value of the license file.
*   \param[out] activationDone is the value of the status bit activation done.
*   \param[out] activationErrorCode is the value of the error code related to activation.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_FILE_SIZE_ERROR if the license file size is lower than the minimum required,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what()
*          should be called to get the exception description.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::activate(const std::string &licenseFile,
                                                                bool &activationDone,
                                                                unsigned char &activationErrorCode) const {
  // set page to license file
  unsigned int errorCode = DrmControllerRegisters::writeLicenseFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set the license file
  errorCode = DrmControllerOperations::writeLicenseFileRegister(licenseFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to register
  errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set license start address
  std::string licenseStartAddress;
  licenseStartAddress.assign(DRM_CONTROLLER_LIC_START_ADDRESS_WORD_NBR*DRM_CONTROLLER_BYTE_SIZE, '0');
  errorCode = DrmControllerOperations::writeLicenseStartAddressRegister(licenseStartAddress);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check heart beat mode is disabled
  if (mHeartBeatModeEnabled == false) {
    // set nop command
    errorCode = DrmControllerRegisters::writeNopCommandRegister();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
    // set activate command
    errorCode = DrmControllerRegisters::writeActivateCommandRegister();
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
  } // mHeartBeatModeEnabled == false
  // wait activation status and error
  errorCode = DrmControllerOperations::waitActivationStatusAndError(DRM_CONTROLLER_ACTIVATE_TIMEOUT_IN_MICRO_SECONDS,
                                                                        true,
                                                                        mDrmErrorNoError,
                                                                        activationDone,
                                                                        activationErrorCode);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_ACTIVATION_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_ACTIVATION_TIMEOUT_STATUS << activationDone << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_ACTIVATION_TIMEOUT_ERROR_CODE;
    stringStream << std::setfill ('0') << std::setw(2) << std::hex << (int)activationErrorCode << std::dec << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_ACTIVATION_TIMEOUT_ERROR_MESSAGE;
    stringStream << DrmControllerOperations::getDrmErrorRegisterMessage(activationErrorCode) << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  // return error result
  return errorCode;
}

/** activate
*   \brief Launch the activation procedure.
*   This method will access to the system bus to write the license file and launch the activation.
*   \param[in] licenseFile is the value of the license file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_FILE_SIZE_ERROR if the license file size is lower than the minimum required,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what()
*          should be called to get the exception description.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::activate(const std::string &licenseFile) const {
  // status and error
  bool activationDone(false);
  unsigned char activationErrorCode(mDrmErrorNotReady);
  // activate
  return DrmControllerOperations::activate(licenseFile, activationDone, activationErrorCode);
}

/** extractLicenseFile
*   \brief Extract the license file.
*   This method will access to the system bus to extract the license file.
*   \param[in]  licenseFileSize is the number of 128 bits words to extract.
*   \param[out] licenseFile is the value of the license file.
*   \param[out] activationDone is the value of the status bit activation done.
*   \param[out] activationErrorCode is the value of the error code related to activation.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractLicenseFile(const unsigned int &licenseFileSize,
                                                                          std::string &licenseFile,
                                                                          bool &activationDone,
                                                                          unsigned char &activationErrorCode) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get activation done status
  errorCode = DrmControllerRegisters::readActivationDoneStatusRegister(activationDone);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get activation error
  errorCode = DrmControllerRegisters::readActivationErrorRegister(activationErrorCode);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to license file
  errorCode = DrmControllerRegisters::writeLicenseFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the license file
  return DrmControllerOperations::readLicenseFileRegister(licenseFileSize, licenseFile);
}

/** extractLicenseFile
*   \brief Extract the license file.
*   This method will access to the system bus to extract the license file.
*   \param[in]  licenseFileSize is the number of 128 bits words to extract.
*   \param[out] licenseFile is the value of the license file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractLicenseFile(const unsigned int &licenseFileSize,
                                                                          std::string &licenseFile) const {
  // status and error
  bool activationDone(false);
  unsigned char activationErrorCode(mDrmErrorNotReady);
  // extract license file
  return DrmControllerOperations::extractLicenseFile(licenseFileSize,
                                                         licenseFile,
                                                         activationDone,
                                                         activationErrorCode);
}

/** extractTraceFile
*   \brief Extract the trace file.
*   This method will access to the system bus to extract the trace file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] traceFile is the value of the trace file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractTraceFile(unsigned int &numberOfDetectedIps,
                                                                        std::vector<std::string> &traceFile) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to trace file
  errorCode = DrmControllerRegisters::writeTraceFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the trace file
  return DrmControllerOperations::readTraceFileRegister(numberOfDetectedIps, traceFile);
}

/** extractLogs
*   \brief Extract the logs value.
*   This method will access to the system bus to extract the logs register.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] logs is the value of the logs.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractLogs(unsigned int &numberOfDetectedIps,
                                                                   std::string &logs) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read logs
  return DrmControllerOperations::readLogsRegister(numberOfDetectedIps, logs);
}

/** extractMeteringFile
*   \brief Extract the metering file.
*   This method will access to the system bus to extract the metering file.
*   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
*               wait the status bit license timer init loaded to be at 0.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \param[out] licenseTimerEnabled is the value of the status bit metering enabled.
*   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init loaded.
*   \param[out] licenseTimerLoadErrorCode is the value of the error code related to license timer loading.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled, mDrmApi_METERING_DISABLED_ERROR if the metering is disabled,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer or the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                                                           unsigned int &numberOfDetectedIps,
                                                                           std::string &saasChallenge,
                                                                           std::vector<std::string> &meteringFile,
                                                                           bool &meteringEnabled,
                                                                           bool &saasChallengeReady,
                                                                           bool &meteringReady,
                                                                           bool &licenseTimerEnabled,
                                                                           bool &licenseTimerInitLoaded,
                                                                           unsigned char &licenseTimerLoadErrorCode) const {
  // call extract metering file and saas challenge
  return DrmControllerOperations::extractMeteringFileAndSaasChallenge(waitNotLicenseTimerInitLoadedTimeout,
                                                                          numberOfDetectedIps,
                                                                          saasChallenge,
                                                                          meteringFile,
                                                                          meteringEnabled,
                                                                          saasChallengeReady,
                                                                          meteringReady,
                                                                          licenseTimerEnabled,
                                                                          licenseTimerInitLoaded,
                                                                          licenseTimerLoadErrorCode);
}

/** extractMeteringFile
*   \brief Extract the metering file.
*   This method will access to the system bus to extract the metering file.
*   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
*               wait the status bit license timer init loaded to be at 0.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled, mDrmApi_METERING_DISABLED_ERROR if the metering is disabled,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer or the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                                                           unsigned int &numberOfDetectedIps,
                                                                           std::string &saasChallenge,
                                                                           std::vector<std::string> &meteringFile) const {
  // status bits
  bool meteringEnabled(false),
       saasChallengeReady(false),
       meteringReady(false),
       licenseTimerEnabled(false),
       licenseTimerInitLoaded(false);
  unsigned char licenseTimerLoadErrorCode(mDrmErrorNotReady);
  // extract metering file
  return DrmControllerOperations::extractMeteringFile(waitNotLicenseTimerInitLoadedTimeout,
                                                          numberOfDetectedIps,
                                                          saasChallenge,
                                                          meteringFile,
                                                          meteringEnabled,
                                                          saasChallengeReady,
                                                          meteringReady,
                                                          licenseTimerEnabled,
                                                          licenseTimerInitLoaded,
                                                          licenseTimerLoadErrorCode);
}

/** endSessionAndExtractMeteringFile
*   \brief Extract the metering file and the saas challenge after writing the command end session extract metering.
*   This method will access to the system bus to extract the metering file and the saas challenge.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \param[out] endSessionMeteringReady is the value of the status bit end session metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps,
                                                                                        std::string &saasChallenge,
                                                                                        std::vector<std::string> &meteringFile,
                                                                                        bool &meteringEnabled,
                                                                                        bool &saasChallengeReady,
                                                                                        bool &meteringReady,
                                                                                        bool &endSessionMeteringReady) const {
  // write register page to registers
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read metering enabled status
  errorCode = DrmControllerRegisters::readMeteringEnabledStatusRegister(meteringEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check metering enabled status
  if (meteringEnabled == false) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_METERING_DISABLED_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_METERING_DISABLED_STATUS << meteringEnabled << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerFunctionalityDisabledException(stringStream.str());
    // return time out error code
    return mDrmApi_METERING_DISABLED_ERROR;
  }
  // set end session extract metering command
  errorCode = DrmControllerRegisters::writeEndSessionMeteringExtractCommandRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // wait end session metering ready status
  errorCode = DrmControllerOperations::waitEndSessionMeteringReadyStatus(DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_IN_MICRO_SECONDS,
                                                                             true,
                                                                             endSessionMeteringReady);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_END_SESSION_EXTRACT_METERING_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_END_SESSION_EXTRACT_METERING_TIMEOUT_STATUS << endSessionMeteringReady << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // extract metering file and saas challenge
  errorCode = DrmControllerOperations::extractMeteringFileAndSaasChallenge(numberOfDetectedIps,
                                                                               saasChallenge,
                                                                               meteringFile,
                                                                               meteringEnabled,
                                                                               saasChallengeReady,
                                                                               meteringReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set nop command
  return DrmControllerRegisters::writeNopCommandRegister();
}

/** endSessionAndExtractMeteringFile
*   \brief Extract the metering file and the saas challenge after writing the command end session extract metering.
*   This method will access to the system bus to extract the metering file and the saas challenge.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps,
                                                                                        std::string &saasChallenge,
                                                                                        std::vector<std::string> &meteringFile) const {
  // status bits
  bool meteringEnabled(false),
       saasChallengeReady(false),
       meteringReady(false),
       endSessionMeteringReady(false);
  // end session extract metering file
  return DrmControllerOperations::endSessionAndExtractMeteringFile(numberOfDetectedIps,
                                                                       saasChallenge,
                                                                       meteringFile,
                                                                       meteringEnabled,
                                                                       saasChallengeReady,
                                                                       meteringReady,
                                                                       endSessionMeteringReady);
}

/** writeLicenseStartAddressRegister
*   \brief Write the value of the license start address.
*   This method will access to the system bus to write the license start address register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[in] licenseStartAddress is the license start address value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::writeLicenseStartAddressRegister(const std::string &licenseStartAddress) const {
  // write the license start address
  return DrmControllerRegisters::writeLicenseStartAddressRegister(DrmControllerDataConverter::hexStringToBinary(licenseStartAddress));
}

/** readLicenseStartAddressRegister
*   \brief Read the value of the license start address.
*   This method will access to the system bus to read the license start address register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[in] licenseStartAddress is the license start address value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readLicenseStartAddressRegister(std::string &licenseStartAddress) const {
  // temporary license start address value
  std::vector<unsigned int> licenseStartAddressList;
  // read the license start address register
  unsigned int errorCode = DrmControllerRegisters::readLicenseStartAddressRegister(licenseStartAddressList);
  // check result
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert saas challenge from integer to string
  licenseStartAddress = DrmControllerDataConverter::binaryToHexString(licenseStartAddressList);
  // return error result
  return errorCode;
}

/** writeLicenseTimerInitRegister
*   \brief Write the value of the license timer.
*   This method will access to the system bus to write the license timer register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[in] licenseTimerInit is the license timer value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::writeLicenseTimerInitRegister(const std::string &licenseTimerInit) const {
  // write the license timer
  return DrmControllerRegisters::writeLicenseTimerInitRegister(DrmControllerDataConverter::hexStringToBinary(licenseTimerInit));
}

/** readLicenseTimerInitRegister
*   \brief Read the value of the license timer.
*   This method will access to the system bus to read the license timer register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[out] licenseTimerInit is the license timer value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readLicenseTimerInitRegister(std::string &licenseTimerInit) const {
  // temporary license timer value
  std::vector<unsigned int> licenseTimerInitList;
  // read the license timer register
  unsigned int errorCode = DrmControllerRegisters::readLicenseTimerInitRegister(licenseTimerInitList);
  // check result
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert
  licenseTimerInit = DrmControllerDataConverter::binaryToHexString(licenseTimerInitList);
  // return error result
  return errorCode;
}

/** readDrmVersionRegister
*   \brief Read the drm version register and get the value.
*   This method will access to the system bus to read the drm version register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[out] drmVersion is the drm version value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readDrmVersionRegister(std::string &drmVersion) const {
  // temporary version value
  unsigned int tmpVersion;
  // read the version
  unsigned int errorCode = DrmControllerRegisters::readDrmVersionRegister(tmpVersion);
  // check result
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert version from integer to string
  drmVersion = DrmControllerDataConverter::binaryToHexString(tmpVersion);
  drmVersion = drmVersion.substr(drmVersion.size()-DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT*DRM_CONTROLLER_VERSION_DIGIT_SIZE, DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT*DRM_CONTROLLER_VERSION_DIGIT_SIZE);
  // return error result
  return errorCode;
}

/** readDnaRegister
*   \brief Read the dna register and get the value.
*   This method will access to the system bus to read the dna register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[out] dna is the dna value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readDnaRegister(std::string &dna) const {
  // temporary dna value
  std::vector<unsigned int> tmpDna;
  // read the dna
  unsigned int errorCode = DrmControllerRegisters::readDnaRegister(tmpDna);
  // check result
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert dna from integer to string
  dna = DrmControllerDataConverter::binaryToHexString(tmpDna);
  // return error result
  return errorCode;
}

/** readSaasChallengeRegister
*   \brief Read the saas challenge register and get the value.
*   This method will access to the system bus to read the saas challenge register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[out] saasChallenge is the saas challenge value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readSaasChallengeRegister(std::string &saasChallenge) const {
  // temporary saas challenge value
  std::vector<unsigned int> tmpSaasChallenge;
  // read the saas challenge
  unsigned int errorCode = DrmControllerRegisters::readSaasChallengeRegister(tmpSaasChallenge);
  // check result
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert saas challenge from integer to string
  saasChallenge = DrmControllerDataConverter::binaryToHexString(tmpSaasChallenge);
  // return error result
  return errorCode;
}

/** readVlnvFileRegister
*   \brief Read the vlnv file and get the value.
*   This method will access to the system bus to read the vlnv file.
*   Before calling this function, the method writeVlnvPage() must be called
*   in order to write the page to select the vlnv file page.
*   The vlnv file will contains numberOfIps+1 elements. The first one
*   is dedicated to the drm controller, the others correspond for
*   the IPs connected to the drm controller.
*   \param[in] numberOfIPs is the total number of IPs.
*   \param[out] vlnvFile is the vlnv file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readVlnvFileRegister(const unsigned int numberOfIPs,
                                                                            std::vector<std::string> &vlnvFile) const {
  // temporary vlnv file
  std::vector<unsigned int> tmpVlnvFile;
  // get vlnv file
  unsigned int errorCode = DrmControllerRegisters::readVlnvFileRegister(numberOfIPs, tmpVlnvFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert
  vlnvFile = DrmControllerOperations::convertRegisterListToStringList(tmpVlnvFile,
                                                                          DRM_CONTROLLER_VLNV_WORD_WORD_NBR);
  // return error result
  return errorCode;
}

/** writeLicenseFileRegister
*   \brief Write the license file.
*   This method will access to the system bus to write the license file.
*   Before calling this function, the method writeLicenseFilePage() must be called
*   in order to write the page to select the license file page.
*   The license file is a string using a hexadecimal representation.
*   \param[in] licenseFile is the license file.
*   \return Returns mDrmApi_NO_ERROR if no error,
*           mDrmApi_LICENSE_FILE_SIZE_ERROR if the license file size is lower than the minimum required,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::writeLicenseFileRegister(const std::string &licenseFile) const {
  // convert into binary the license file
  std::vector<unsigned int> tmpLicenseFile = DrmControllerDataConverter::hexStringToBinary(licenseFile);
  // check license file size
  if (DrmControllerOperations::checkLicenseFileSize(tmpLicenseFile) == false) {
    // return error
    return mDrmApi_LICENSE_FILE_SIZE_ERROR;
  }
  // write the license file
  unsigned int licenseFileSize = tmpLicenseFile.size()/DRM_CONTROLLER_LICENSE_WORD_WORD_NBR;
  return DrmControllerRegisters::writeLicenseFileRegister(licenseFileSize, tmpLicenseFile);
}

/** readLicenseFileRegister
*   \brief Read the license file and get the value.
*   This method will access to the system bus to read the license file.
*   Before calling this function, the method writeLicenseFilePage() must be called
*   in order to write the page to select the license file page.
*   The license file is a string using a hexadecimal representation.
*   \param[in] licenseFileSize is the number of 128 bits words to read.
*   \param[out] licenseFile is the license file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readLicenseFileRegister(const unsigned int &licenseFileSize,
                                                                               std::string &licenseFile) const {
  // temporary license
  std::vector<unsigned int> tmpLicense;
  // read the license file
  unsigned int errorCode = DrmControllerRegisters::readLicenseFileRegister(licenseFileSize, tmpLicense);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert each element into a hexadecimal representation
  licenseFile = DrmControllerDataConverter::binaryToHexString(tmpLicense);
  // return error result
  return errorCode;
}

/** readLogsRegister
*   \brief Read the logs register and get the value.
*   This method will access to the system bus to read the logs register.
*   Before calling this function, the method writeRegistersPage() must be called
*   in order to write the page to select the registers page.
*   \param[in] numberOfIPs is the total number of IPs.
*   \param[out] logs is the logs value.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readLogsRegister(const unsigned int numberOfIPs,
                                                                        std::string &logs) const {
  // temporary logs
  std::vector<unsigned int> tmpLogs;
  // read logs
  unsigned int errorCode = DrmControllerRegisters::readLogsRegister(numberOfIPs, tmpLogs);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set logs
  logs = DrmControllerDataConverter::binaryToBinString(tmpLogs);
  // truncate the logs member using number of detected ips
  logs = logs.substr(logs.size()-numberOfIPs, numberOfIPs);
  // return error result
  return errorCode;
}

/** readTraceFileRegister
*   \brief Read the trace file and get the value.
*   This method will access to the system bus to read the trace file.
*   Before calling this function, the method writeTraceFilePage() must be called
*   in order to write the page to select the trace file page.
*   \param[in] numberOfIPs is the total number of IPs.
*   \param[out] traceFile is the trace file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readTraceFileRegister(const unsigned int numberOfIPs,
                                                                             std::vector<std::string> &traceFile) const {
  // temporary trace file
  std::vector<unsigned int> tmpTraceFile;
  // read the trace file
  unsigned int errorCode = DrmControllerRegisters::readTraceFileRegister(numberOfIPs, tmpTraceFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert
  traceFile = DrmControllerOperations::convertRegisterListToStringList(tmpTraceFile,
                                                                           DRM_CONTROLLER_TRACE_WORD_WORD_NBR);
  // return error result
  return errorCode;
}

/** readMeteringFileRegister
*   \brief Read the metering file and get the value.
*   This method will access to the system bus to read the metering file.
*   Before calling this function, the method writeMeteringFilePage() must be called
*   in order to write the page to select the metering file page.
*   \param[in] numberOfIPs is the total number of IPs.
*   \param[out] meteringFile is the metering file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readMeteringFileRegister(const unsigned int numberOfIPs,
                                                                                std::vector<std::string> &meteringFile) const {
  // temporary metering file
  std::vector<unsigned int> tmpMeteringFile;
  // read the metering file
  unsigned int errorCode = DrmControllerRegisters::readMeteringFileRegister(numberOfIPs, tmpMeteringFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // convert
  meteringFile = DrmControllerOperations::convertRegisterListToStringList(tmpMeteringFile,
                                                                              DRM_CONTROLLER_METERING_WORD_WORD_NBR);
  // return error result
  return errorCode;
}

/** getDrmErrorRegisterMessage
*   \brief Get the error message from the error register value.
*   \param[in] errorRegister is the value of the error register.
*   \return Returns the error message.
**/
const char* DrmControllerLibrary::DrmControllerOperations::getDrmErrorRegisterMessage(const unsigned char &errorRegister) {
  // iterate on each element of the error array
  for (unsigned int ii = 0; ii < mDrmErrorArraySize; ii++) {
    // verify error register to return the error text
    if (mDrmErrorArray[ii].mDrmErrorEnum == errorRegister) {
      return mDrmErrorArray[ii].mDrmErrorText.c_str();
    }
  }
  // return by default an unknown error
  return "Unknown error";
}

/** getDrmApiMessage
*   \brief Get the error message from the api error code.
*   \param[in] drmApiErrorCode is the value of the api error code.
*   \return Returns the error message.
**/
const char* DrmControllerLibrary::DrmControllerOperations::getDrmApiMessage(const unsigned int &drmApiErrorCode) const {
  // iterate on each element of the error array
  for (unsigned int ii = 0; ii < mDrmApiErrorArraySize; ii++) {
    // verify error register to return the error text
    if (mDrmApiErrorArray[ii].mDrmApiErrorCode == (t_drmApiErrorCode)drmApiErrorCode) {
      std::ostringstream stringStream;
      stringStream << mDrmApiErrorArray[ii].mDrmApiErrorText;
      return stringStream.str().c_str();
    }
  }
  // return by default an unknown error
  return "UNKNOWN ERROR";
}

/** checkVersion
*   \brief Check hardware version matches the software version.
*   \return Returns DRM_CONTROLLER_NO_ERROR_CODE when hardware and SDK version match,
*                   mDrmApi_VERSION_CHECK_ERROR otherwise.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_VERSION_CHECK_ERROR if the hardware
*           version do not match, the software version or the error code produced by the read/write register function.
*   \throw DrmControllerVersionCheckException whenever an error occured. DrmControllerVersionCheckException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::checkVersion() const {
  // extract hardware version
  std::string drmVersion;
  unsigned int errorCode = DrmControllerOperations::extractDrmVersion(drmVersion);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    return errorCode;
  // check version
  std::string drmVersionDot = DrmControllerDataConverter::binaryToVersionString(DrmControllerDataConverter::hexStringToBinary(drmVersion)[0]);
  return DrmControllerVersion::checkVersion(drmVersionDot);
}

/** extractMeteringFile
*   \brief Extract the metering file.
*   This method will access to the system bus to extract the metering file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractMeteringFile(unsigned int &numberOfDetectedIps,
                                                                           std::vector<std::string> &meteringFile,
                                                                           bool &meteringEnabled,
                                                                           bool &meteringReady) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read metering enabled status
  errorCode = DrmControllerRegisters::readMeteringEnabledStatusRegister(meteringEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check metering enabled status
  if (meteringEnabled == false) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_METERING_DISABLED_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_METERING_DISABLED_STATUS << meteringEnabled << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerFunctionalityDisabledException(stringStream.str());
    // return time out error code
    return mDrmApi_METERING_DISABLED_ERROR;
  }
  // wait metering ready status
  errorCode = DrmControllerOperations::waitMeteringReadyStatus(DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_IN_MICRO_SECONDS,
                                                                   true,
                                                                   meteringReady);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_STATUS << meteringReady << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to metering file
  errorCode = DrmControllerRegisters::writeMeteringFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get metering file
  return DrmControllerOperations::readMeteringFileRegister(numberOfDetectedIps, meteringFile);
}
/** extractSaasChallenge
*   \brief Extract the saas challenge.
*   This method will access to the system bus to extract the saas challenge.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractSaasChallenge(std::string &saasChallenge,
                                                                            bool &saasChallengeReady) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // wait saas challenge ready status
  errorCode = DrmControllerOperations::waitSaasChallengeReadyStatus(DRM_CONTROLLER_EXTRACT_SAAS_CHALLENGE_TIMEOUT_IN_MICRO_SECONDS,
                                                                        true,
                                                                        saasChallengeReady);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_EXTRACT_SAAS_CHALLENGE_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_EXTRACT_SAAS_CHALLENGE_TIMEOUT_STATUS << saasChallengeReady << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get saas challenge
  return DrmControllerOperations::readSaasChallengeRegister(saasChallenge);
}

/** checkLicenseTimerInitLoaded
*   \brief Verify if the license timer has been loaded.
*   This method will access to the system bus to read the status bit timer init loaded and the timer load error code.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_RESETED_ERROR if the DRM Controller has been reseted,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseTimerResetedException whenever the license timer has been reseted. DrmControllerLicenseTimerResetedException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::checkLicenseTimerInitLoaded() const {
  // get license timer load done bit status
  bool licenseTimerInitLoaded;
  unsigned int errorCode = DrmControllerRegisters::readLicenseTimerInitLoadedStatusRegister(licenseTimerInitLoaded);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get license timer load error code
  unsigned char licenseTimerLoadErrorCode;
  errorCode = DrmControllerRegisters::readLicenseTimerLoadErrorRegister(licenseTimerLoadErrorCode);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // verify license timer load error
  if (licenseTimerLoadErrorCode == mDrmErrorNotReady) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_DESCRIPTION_1 << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_DESCRIPTION_2 << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_STATUS << licenseTimerInitLoaded << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_CODE;
    stringStream << std::setfill ('0') << std::setw(2) << std::hex << (int)licenseTimerLoadErrorCode << std::dec << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_MESSAGE;
    stringStream << DrmControllerOperations::getDrmErrorRegisterMessage(licenseTimerLoadErrorCode) << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerLicenseTimerResetedException(stringStream.str());
    // return license timer reseted error code
    return mDrmApi_LICENSE_TIMER_RESETED_ERROR;
  }
  // return last error code
  return errorCode;
}

/** waitNotTimerInitLoaded
*   \brief Wait not timer init loaded.
*   This method will access to the system bus to read the status bit timer init loaded and the timer load error code.
*   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
*               wait the status bit license timer init loaded to be at 0.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init loaded.
*   \param[out] licenseTimerLoadErrorCode is the value of the error code related to license timer loading.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitNotTimerInitLoaded(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                                                              bool &licenseTimerEnabled,
                                                                              bool &licenseTimerInitLoaded,
                                                                              unsigned char &licenseTimerLoadErrorCode) const {
  //
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read license timer enabled status
  errorCode = DrmControllerRegisters::readLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // check license timer enabled status
  if (licenseTimerEnabled == false) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_LICENSE_TIMER_DISABLED_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_DISABLED_STATUS << licenseTimerEnabled << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerFunctionalityDisabledException(stringStream.str());
    // return time out error code
    return mDrmApi_LICENSE_TIMER_DISABLED_ERROR;
  }
  // wait license timer load status and error
  errorCode = DrmControllerOperations::waitLicenseTimerLoadStatusAndError(waitNotLicenseTimerInitLoadedTimeout*
                                                                              DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND,
                                                                              false,
                                                                              mDrmErrorNoError,
                                                                              licenseTimerInitLoaded,
                                                                              licenseTimerLoadErrorCode);
  // return error when timeout reached
  if (errorCode == mDrmApi_HARDWARE_TIMEOUT_ERROR) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_STATUS << licenseTimerInitLoaded << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_ERROR_CODE;
    stringStream << std::setfill ('0') << std::setw(2) << std::hex << (int)licenseTimerLoadErrorCode << std::dec << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_ERROR_MESSAGE;
    stringStream << DrmControllerOperations::getDrmErrorRegisterMessage(licenseTimerLoadErrorCode) << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerTimeOutException(stringStream.str());
    // return time out error code
    return errorCode;
  }
  return errorCode;
}

/** extractMeteringFileAndSaasChallenge
*   \brief Extract the metering file and the saas challenge.
*   This method will access to the system bus to extract the metering file and the saas challenge.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] meteringEnabled is the value of the status bit meterig enabled.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractMeteringFileAndSaasChallenge(unsigned int &numberOfDetectedIps,
                                                                                           std::string &saasChallenge,
                                                                                           std::vector<std::string> &meteringFile,
                                                                                           bool &meteringEnabled,
                                                                                           bool &saasChallengeReady,
                                                                                           bool &meteringReady) const {
  // extract metering file
  unsigned int errorCode = DrmControllerOperations::extractMeteringFile(numberOfDetectedIps,
                                                                              meteringFile,
                                                                              meteringEnabled,
                                                                              meteringReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // extract saas challenge
  return DrmControllerOperations::extractSaasChallenge(saasChallenge,
                                                           saasChallengeReady);
}

/** extractMeteringFileAndSaasChallenge
*   \brief Wait not timer init loaded and then extract the metering file and the saas challenge.
*   This method will access to the system bus to extract the metering file and the saas challenge.
*   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
*               wait the status bit license timer init loaded to be at 0.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init loaded.
*   \param[out] licenseTimerLoadErrorCode is the value of the error code related to license timer loading.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled, mDrmApi_METERING_DISABLED_ERROR if the metering is disabled,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer or the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::extractMeteringFileAndSaasChallenge(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                                                                           unsigned int &numberOfDetectedIps,
                                                                                           std::string &saasChallenge,
                                                                                           std::vector<std::string> &meteringFile,
                                                                                           bool &meteringEnabled,
                                                                                           bool &saasChallengeReady,
                                                                                           bool &meteringReady,
                                                                                           bool &licenseTimerEnabled,
                                                                                           bool &licenseTimerInitLoaded,
                                                                                           unsigned char &licenseTimerLoadErrorCode) const {
  // wait not timer init loaded
  unsigned int errorCode = DrmControllerOperations::waitNotTimerInitLoaded(waitNotLicenseTimerInitLoadedTimeout,
                                                                               licenseTimerEnabled,
                                                                               licenseTimerInitLoaded,
                                                                               licenseTimerLoadErrorCode);
  // check no error
  if ((errorCode != mDrmApi_NO_ERROR) || (licenseTimerLoadErrorCode != mDrmErrorNoError))
    // return error result
    return errorCode;
  // extract metering file and saas challenge
  return DrmControllerOperations::extractMeteringFileAndSaasChallenge(numberOfDetectedIps,
                                                                          saasChallenge,
                                                                          meteringFile,
                                                                          meteringEnabled,
                                                                          saasChallengeReady,
                                                                          meteringReady);
}

/** waitStatusAndError
*   \brief Wait status and error to reach specified value.
*   This method will access to the system bus to read status and error registers.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  statusBitPosition is the position of the status bit.
*   \param[in]  statusMask is the mask of the status bit.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  errorPosition is the position of the error byte.
*   \param[in]  errorMask is the mask of the error byte.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[out] status is the value of the status bit read.
*   \param[out] error is the value of the error byte read.
*   \param[in]  readStatusOnly allows to read the status only. Set to false by default.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitStatusAndError(const unsigned int &timeout,
                                                                          const t_drmStatusRegisterEnumValues &statusBitPosition,
                                                                          const t_drmStatusRegisterMaskEnumValues &statusMask,
                                                                          const bool &expectedStatus,
                                                                          const t_drmErrorRegisterBytePositionEnumValues &errorPosition,
                                                                          const t_drmErrorRegisterByteMaskEnumValues &errorMask,
                                                                          const unsigned char &expectedError,
                                                                          bool &status,
                                                                          unsigned char &error,
                                                                          const bool& readStatusOnly) const {
  //
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get current time
  timeval startTimePoint, currentTimePoint;
  gettimeofday(&startTimePoint, NULL);
  // loop while expected status and error not reached
  unsigned int timeTaken = 0;
  do {
    // read status only when required
    if (readStatusOnly == true) {
      // read status
      errorCode = DrmControllerRegisters::readStatusRegister(statusBitPosition,
                                                                 statusMask,
                                                                 status);
    }
    else {
      // read status and error
      errorCode = DrmControllerOperations::readStatusAndErrorRegisters(statusBitPosition,
                                                                           statusMask,
                                                                           expectedStatus,
                                                                           errorPosition,
                                                                           errorMask,
                                                                           status,
                                                                           error);
     }
    // check no error
    if (errorCode != mDrmApi_NO_ERROR)
      // return error result
      return errorCode;
    // get current time
    gettimeofday(&currentTimePoint, NULL);
    // get time duration
    timeTaken = (unsigned int)(currentTimePoint.tv_sec - startTimePoint.tv_sec)*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND +
                              (currentTimePoint.tv_usec - startTimePoint.tv_usec);
    // check timeout reached
    if (timeout > 0 && timeTaken > timeout)
      // return timeout error
      return mDrmApi_HARDWARE_TIMEOUT_ERROR;
    // exit loop when expected status and error were read
    if ((readStatusOnly == true && status == expectedStatus) ||
        (readStatusOnly == false && status == expectedStatus && error == expectedError))
      // exit loop
      break;
  } while (true);
  // return last error code
  return errorCode;
}

/** waitDnaExtractStatusAndError
*   \brief Wait status and error of dna extract operations to reach specified value.
*   This method will access to the system bus to read status and error registers.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[out] status is the value of the status bit read.
*   \param[out] error is the value of the error byte read.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitDnaExtractStatusAndError(const unsigned int &timeout,
                                                                                    const bool &expectedStatus,
                                                                                    const unsigned char &expectedError,
                                                                                    bool &status,
                                                                                    unsigned char &error) const {
  // wait status and error
  return DrmControllerOperations::waitStatusAndError(timeout,
                                                         mDrmStatusDnaReady,
                                                         mDrmStatusMaskDnaReady,
                                                         expectedStatus,
                                                         mDrmDnaExtractErrorPosition,
                                                         mDrmDnaExtractErrorMask,
                                                         expectedError,
                                                         status,
                                                         error);
}

/** waitVlnvExtractStatusAndError
*   \brief Wait status and error of vlnv extract operations to reach specified value.
*   This method will access to the system bus to read status and error registers.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[out] status is the value of the status bit read.
*   \param[out] error is the value of the error byte read.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitVlnvExtractStatusAndError(const unsigned int &timeout,
                                                                                     const bool &expectedStatus,
                                                                                     const unsigned char &expectedError,
                                                                                     bool &status,
                                                                                     unsigned char &error) const {
  // wait status and error
  return DrmControllerOperations::waitStatusAndError(timeout,
                                                         mDrmStatusVlnvReady,
                                                         mDrmStatusMaskVlnvReady,
                                                         expectedStatus,
                                                         mDrmVlnvExtractErrorPosition,
                                                         mDrmVlnvExtractErrorMask,
                                                         expectedError,
                                                         status,
                                                         error);
}

/** waitActivationStatusAndError
*   \brief Wait status and error of activation operations to reach specified value.
*   This method will access to the system bus to read status and error registers.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[out] status is the value of the status bit read.
*   \param[out] error is the value of the error byte read.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitActivationStatusAndError(const unsigned int &timeout,
                                                                                    const bool &expectedStatus,
                                                                                    const unsigned char &expectedError,
                                                                                    bool &status,
                                                                                    unsigned char &error) const {
  // wait status and error
  return DrmControllerOperations::waitStatusAndError(timeout,
                                                         mDrmStatusActivationDone,
                                                         mDrmStatusMaskActivationDone,
                                                         expectedStatus,
                                                         mDrmActivationErrorPosition,
                                                         mDrmActivationErrorMask,
                                                         expectedError,
                                                         status,
                                                         error);
}

/** waitLicenseTimerLoadStatusAndError
*   \brief Wait status and error of license timer load operations to reach specified value.
*   This method will access to the system bus to read status and error registers.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[out] status is the value of the status bit read.
*   \param[out] error is the value of the error byte read.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitLicenseTimerLoadStatusAndError(const unsigned int &timeout,
                                                                                          const bool &expectedStatus,
                                                                                          const unsigned char &expectedError,
                                                                                          bool &status,
                                                                                          unsigned char &error) const {
  // wait status and error
  return DrmControllerOperations::waitStatusAndError(timeout,
                                                         mDrmStatusLicenseTimerInitLoaded,
                                                         mDrmStatusMaskLicenseTimerInitLoaded,
                                                         expectedStatus,
                                                         mDrmLicenseTimerLoadErrorPosition,
                                                         mDrmLicenseTimerLoadErrorMask,
                                                         expectedError,
                                                         status,
                                                         error);
}

/** waitStatus
*   \brief Wait status to reach specified value.
*   This method will access to the system bus to read the status register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  statusBitPosition is the position of the status bit.
*   \param[in]  statusMask is the mask of the status bit.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[out] status is the value of the status bit read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitStatus(const unsigned int &timeout,
                                                                  const t_drmStatusRegisterEnumValues &statusBitPosition,
                                                                  const t_drmStatusRegisterMaskEnumValues &statusMask,
                                                                  const bool &expectedStatus,
                                                                  bool &status) const {
  // wait status and error but ignore error
  unsigned char error;
  return DrmControllerOperations::waitStatusAndError(timeout,
                                                         statusBitPosition,
                                                         statusMask,
                                                         expectedStatus,
                                                         mDrmLicenseTimerLoadErrorPosition,
                                                         mDrmLicenseTimerLoadErrorMask,
                                                         mDrmErrorNoError,
                                                         status,
                                                         error,
                                                         true);
}

/** waitMeteringReadyStatus
*   \brief Wait status metering ready to reach specified value.
*   This method will access to the system bus to read the status register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[out] status is the value of the status bit read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitMeteringReadyStatus(const unsigned int &timeout,
                                                                               const bool &expectedStatus,
                                                                               bool &status) const {
  // wait status
  return DrmControllerOperations::waitStatus(timeout,
                                                 mDrmStatusMeteringReady,
                                                 mDrmStatusMaskMeteringReady,
                                                 expectedStatus,
                                                 status);
}

/** waitSaasChallengeReadyStatus
*   \brief Wait status saas challenge ready to reach specified value.
*   This method will access to the system bus to read the status register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[out] status is the value of the status bit read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitSaasChallengeReadyStatus(const unsigned int &timeout,
                                                                                    const bool &expectedStatus,
                                                                                    bool &status) const {
  // wait status
  return DrmControllerOperations::waitStatus(timeout,
                                                 mDrmStatusSaasChallengeReady,
                                                 mDrmStatusMaskSaasChallengeReady,
                                                 expectedStatus,
                                                 status);
}

/** waitEndSessionMeteringReadyStatus
*   \brief Wait status end session metering ready to reach specified value.
*   This method will access to the system bus to read the status register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[out] status is the value of the status bit read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::waitEndSessionMeteringReadyStatus(const unsigned int &timeout,
                                                                                         const bool &expectedStatus,
                                                                                         bool &status) const {
  // wait status
  return DrmControllerOperations::waitStatus(timeout,
                                                 mDrmStatusEndSessionMeteringReady,
                                                 mDrmStatusMaskEndSessionMeteringReady,
                                                 expectedStatus,
                                                 status);
}

/** readStatusAndErrorRegisters
*   \brief Read values of status and eror registers.
*   This method will access to the system bus to read status and error registers.
*   \param[in]  statusBitPosition is the position of the status bit.
*   \param[in]  statusMask is the mask of the status bit.
*   \param[in]  statusToReadError is the value of the status to enable the error read.
*   \param[in]  errorPosition is the position of the error byte.
*   \param[in]  errorMask is the mask of the error byte.
*   \param[out] status is the value of the status bit.
*   \param[out] error is the value of the error byte.
*   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::readStatusAndErrorRegisters(const t_drmStatusRegisterEnumValues &statusBitPosition,
                                                                                   const t_drmStatusRegisterMaskEnumValues &statusMask,
                                                                                   const bool &statusToReadError,
                                                                                   const t_drmErrorRegisterBytePositionEnumValues &errorPosition,
                                                                                   const t_drmErrorRegisterByteMaskEnumValues &errorMask,
                                                                                   bool &status,
                                                                                   unsigned char &error) const {
  // read status register
  unsigned int errorCode = DrmControllerRegisters::readStatusRegister(statusBitPosition,
                                                                          statusMask,
                                                                          status);
  // check no error
  if (errorCode != mDrmErrorNoError)
    // return error
    return errorCode;
  // read error register if ok
  if (status == statusToReadError) {
    errorCode = DrmControllerRegisters::readErrorRegister(errorPosition,
                                                              errorMask,
                                                              error);
  }
  // return the error code
  return errorCode;
}

/** convertRegisterListToStringList
*   \brief Convert a list of register value into a list of strings.
*   \param[in] registerList is the list of register values.
*   \param[in] numberOfRegisterPerString is the number of registers to push per string.
*   \return Returns the list of strings generated.
**/
std::vector<std::string> DrmControllerLibrary::DrmControllerOperations::convertRegisterListToStringList(const std::vector<unsigned int> &registerList,
                                                                                                   const unsigned int &numberOfRegisterPerString) const {
  // create a string list
  std::vector<std::string> stringList;
  // clear the string list
  stringList.clear();
  // get each element from the register list
  for (unsigned int ii = 0; ii < registerList.size(); ii+=numberOfRegisterPerString) {
    // create a temporary vector
    std::vector<unsigned int> tmpRegisterList;
    // fill the vector
    if (registerList.begin()+ii < registerList.end() && registerList.begin()+ii+numberOfRegisterPerString <= registerList.end())
      tmpRegisterList = std::vector<unsigned int>(registerList.begin()+ii, registerList.begin()+ii+numberOfRegisterPerString);
    else
      tmpRegisterList = std::vector<unsigned int>(registerList.begin(), registerList.end());
    // convert from integer to string the trace file
    stringList.push_back(DrmControllerDataConverter::binaryToHexString(tmpRegisterList));
  }
  // return the string list
  return stringList;
}

/** checkLicenseFileSize
*   \brief Verify that the size of the license file is correct.
*   \param[in] licenseFile is the license file.
*   \return Returns true if license file size is correct, false otherwize.
*   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what()
*          should be called to get the exception description.
**/
bool DrmControllerLibrary::DrmControllerOperations::checkLicenseFileSize(const std::vector<unsigned int> &licenseFile) const {
  if (licenseFile.size() < DRM_CONTROLLER_MINIMUM_LICENSE_FILE_SIZE_SYSTEM_BUS_DATA_SIZE) {
    // build the exception message
    std::ostringstream stringStream;
    stringStream << DRM_CONTROLLER_ERROR_HEADER << DRM_CONTROLLER_LICENSE_FILE_SIZE_ERROR_HEADER << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_FILE_SIZE_REQUIRED;
    stringStream << DRM_CONTROLLER_MINIMUM_LICENSE_FILE_SIZE_SYSTEM_BUS_DATA_SIZE << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    stringStream << DRM_CONTROLLER_ERROR_HEADER_EMPTY << DRM_CONTROLLER_LICENSE_FILE_SIZE_CURRENT << licenseFile.size() << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    // throw exception
    throw DrmControllerLicenseFileSizeException(stringStream.str());
    return false;
  }
  return true;
}

/** printHwReport
* \brief Print all register content accessible through AXI-4 Lite Control channel.
* \param[in] file: Reference to output file where register contents are saved. By default print on standard output
* \return void.
*/
void DrmControllerLibrary::DrmControllerOperations::printHwReport(std::ostream &file) const {
  // display header
  DrmControllerOperations::printHeaderHwReport(file);
  // display component name
  DrmControllerOperations::printComponentNameHwReport(" DRM CONTROLLER", file);
  // display the drm version
  if (DrmControllerOperations::displayDrmVersion(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the page
  if (DrmControllerOperations::displayPage(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the command
  if (DrmControllerOperations::displayCommand(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the status
  if (DrmControllerOperations::displayStatus(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the error
  if (DrmControllerOperations::displayError(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the license start address
  if (DrmControllerOperations::displayLicenseStartAddress(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the license timer
  if (DrmControllerOperations::displayLicenseTimer(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the dna
  if (DrmControllerOperations::displayDna(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the saas challenge
  if (DrmControllerOperations::displaySaasChallenge(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the vlnv file
  if (DrmControllerOperations::displayVlnvFile(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the license file
  if (DrmControllerOperations::displayLicenseFile(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the metering file
  if (DrmControllerOperations::displayMeteringFile(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the logs
  if (DrmControllerOperations::displayLogs(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display the trace file
  if (DrmControllerOperations::displayTraceFile(file) != mDrmApi_NO_ERROR)
    // return
    return ;
  // display footer
  DrmControllerOperations::printFooterHwReport(file);
}

/** displayPage
*   \brief Display the value of the page register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayPage(std::ostream &file) const {
  // the page value
  unsigned int page(0);
  // read the page register
  unsigned int errorCode = DrmControllerRegisters::readPageRegister(page);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the page value
  DrmControllerOperations::printRegisterNameHwReport("PAGE", file);
  DrmControllerOperations::printWordRegisterValueHwReport(page, "PAGE", 1, file);
  // return error result
  return errorCode;
}

/** displayCommand
*   \brief Display the value of the command register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayCommand(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the command register
  unsigned int command(0);
  errorCode = DrmControllerRegisters::readCommandRegister(command);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display command value
  DrmControllerOperations::printRegisterNameHwReport("COMMAND", file);
  DrmControllerOperations::printWordRegisterValueHwReport(command, "COMMAND", 2, file);
  // return error result
  return errorCode;
}

/** displayLicenseStartAddress
*   \brief Display the value of the license start address register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayLicenseStartAddress(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the license start address register
  std::string licenseStartAddress;
  errorCode = DrmControllerOperations::readLicenseStartAddressRegister(licenseStartAddress);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the license start address
  DrmControllerOperations::printRegisterNameHwReport("LICENSE START ADDRESS", file);
  DrmControllerOperations::printStringRegisterValueHwReport(licenseStartAddress, "LICENSE START ADDRESS", file);
  // return error result
  return errorCode;
}

/** displayLicenseTimer
*   \brief Display the value of the license timer register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayLicenseTimer(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the license timer register
  std::string licenseTimerInitRegister;
  errorCode = DrmControllerOperations::readLicenseTimerInitRegister(licenseTimerInitRegister);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the license timer
  DrmControllerOperations::printRegisterNameHwReport("LICENSE TIMER INIT", file);
  DrmControllerOperations::printStringRegisterValueHwReport(licenseTimerInitRegister.substr(0,  32), "LICENSE TIMER INIT WORD 0", file);
  DrmControllerOperations::printStringRegisterValueHwReport(licenseTimerInitRegister.substr(32, 32), "LICENSE TIMER INIT WORD 1", file);
  DrmControllerOperations::printStringRegisterValueHwReport(licenseTimerInitRegister.substr(64, 32), "LICENSE TIMER INIT WORD 2", file);
  // return error result
  return errorCode;
}

/** displayStatus
*   \brief Display the value of the status register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayStatus(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read status
  bool dnaReady(false),
       vlnvReady(false),
       activationDone(false),
       autoEnabled(false),
       autoBusy(false),
       meteringEnabled(false),
       meteringReady(false),
       saasChallengeReady(false),
       licenseTimerEnabled(false),
       licenseTimerInitLoaded(false),
       endSessionMeteringReady(false);
  unsigned int numberOfDetectedIps(0);
  // read dna ready status
  errorCode = DrmControllerRegisters::readDnaReadyStatusRegister(dnaReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read vlnv ready status
  errorCode = DrmControllerRegisters::readVlnvReadyStatusRegister(vlnvReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read activation done status
  errorCode = DrmControllerRegisters::readActivationDoneStatusRegister(activationDone);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read auto enabled status
  errorCode = DrmControllerRegisters::readAutonomousControllerEnabledStatusRegister(autoEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read auto busy status
  errorCode = DrmControllerRegisters::readAutonomousControllerBusyStatusRegister(autoBusy);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read metering enabled status
  errorCode = DrmControllerRegisters::readMeteringEnabledStatusRegister(meteringEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read metering ready status
  errorCode = DrmControllerRegisters::readMeteringReadyStatusRegister(meteringReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read saas challenge ready status
  errorCode = DrmControllerRegisters::readSaasChallengeReadyStatusRegister(saasChallengeReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read license timer enabled status
  errorCode = DrmControllerRegisters::readLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read license timer init loaded status
  errorCode = DrmControllerRegisters::readLicenseTimerInitLoadedStatusRegister(licenseTimerInitLoaded);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read end session metering ready status
  errorCode = DrmControllerRegisters::readEndSessionMeteringReadyStatusRegister(endSessionMeteringReady);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read number of detected ips status
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display status bits
  DrmControllerOperations::printRegisterNameHwReport("STATUS", file);
  DrmControllerOperations::printBitRegisterValueHwReport(dnaReady,                "DNA READY STATUS",                     file);
  DrmControllerOperations::printBitRegisterValueHwReport(vlnvReady,               "VLNV READY STATUS",                    file);
  DrmControllerOperations::printBitRegisterValueHwReport(activationDone,          "ACTIVATION DONE STATUS",               file);
  DrmControllerOperations::printBitRegisterValueHwReport(autoEnabled,             "AUTONOMOUS CONTROLLER ENABLED STATUS", file);
  DrmControllerOperations::printBitRegisterValueHwReport(autoBusy,                "AUTONOMOUS CONTROLLER BUSY STATUS",    file);
  DrmControllerOperations::printBitRegisterValueHwReport(meteringEnabled,         "METERING ENABLED STATUS",              file);
  DrmControllerOperations::printBitRegisterValueHwReport(meteringReady,           "METERING READY STATUS",                file);
  DrmControllerOperations::printBitRegisterValueHwReport(saasChallengeReady,      "SAAS CHALLENGE READY STATUS",          file);
  DrmControllerOperations::printBitRegisterValueHwReport(licenseTimerEnabled,     "LICENSE TIMER ENABLED STATUS",         file);
  DrmControllerOperations::printBitRegisterValueHwReport(licenseTimerInitLoaded,  "LICENSE TIMER INIT LOADED STATUS",     file);
  DrmControllerOperations::printBitRegisterValueHwReport(endSessionMeteringReady, "END SESSION METERING READY STATUS",    file);
  DrmControllerOperations::printIntRegisterValueHwReport(numberOfDetectedIps,     "NUMBER OF DETECTED IPS STATUS",        file);
  return errorCode;
}

/** displayError
*   \brief Display the value of the error register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayError(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read error codes
  unsigned char extractDnaErrorCode, extractVlnvErrorCode, licenseTimerLoadErrorCode, activationErrorCode;
  // read extract dna error code
  errorCode = DrmControllerRegisters::readExtractDnaErrorRegister(extractDnaErrorCode);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read extract vlnv error code
  errorCode = DrmControllerRegisters::readExtractVlnvErrorRegister(extractVlnvErrorCode);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read license timer load error code
  errorCode = DrmControllerRegisters::readLicenseTimerLoadErrorRegister(licenseTimerLoadErrorCode);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read activation error code
  errorCode = DrmControllerRegisters::readActivationErrorRegister(activationErrorCode);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the error codes
  DrmControllerOperations::printRegisterNameHwReport("ERROR", file);

  DrmControllerOperations::printWordRegisterValueHwReport((unsigned int)extractDnaErrorCode,       "DNA EXTRACT ERROR",        2, file);
  DrmControllerOperations::printStringValueHwReport(DrmControllerOperations::getDrmErrorRegisterMessage(extractDnaErrorCode),
                                                        "DNA EXTRACT ERROR MESSAGE", file);

  DrmControllerOperations::printWordRegisterValueHwReport((unsigned int)extractVlnvErrorCode,      "VLNV EXTRACT ERROR",       2, file);
  DrmControllerOperations::printStringValueHwReport(DrmControllerOperations::getDrmErrorRegisterMessage(extractVlnvErrorCode),
                                                        "VLNV EXTRACT ERROR MESSAGE", file);

  DrmControllerOperations::printWordRegisterValueHwReport((unsigned int)licenseTimerLoadErrorCode, "LICENSE TIMER LOAD ERROR", 2, file);
  DrmControllerOperations::printStringValueHwReport(DrmControllerOperations::getDrmErrorRegisterMessage(licenseTimerLoadErrorCode),
                                                        "LICENSE TIMER LOAD ERROR MESSAGE", file);

  DrmControllerOperations::printWordRegisterValueHwReport((unsigned int)activationErrorCode,       "ACTIVATION ERROR",         2, file);
  DrmControllerOperations::printStringValueHwReport(DrmControllerOperations::getDrmErrorRegisterMessage(activationErrorCode),
                                                        "ACTIVATION ERROR MESSAGE", file);
  return errorCode;
}

/** displayDrmVersion
*   \brief Display the value of the drm version.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayDrmVersion(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the drm version
  std::string drmVersion;
  errorCode = DrmControllerOperations::readDrmVersionRegister(drmVersion);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the SDK version
  DrmControllerOperations::printVersionNameHwReport(" SOFTWARE", file);
  DrmControllerOperations::printStringValueHwReport(DRM_CONTROLLER_SDK_VERSION, " SDK VERSION", file);
  // display the hardware version
  std::string drmVersionDot = DrmControllerDataConverter::binaryToVersionString(DrmControllerDataConverter::hexStringToBinary(drmVersion)[0]);
  DrmControllerOperations::printVersionNameHwReport(" HARDWARE", file);
  DrmControllerOperations::printStringValueHwReport(drmVersionDot, " HDK VERSION", file);
  DrmControllerOperations::printStringValueHwReport(DrmControllerVersion::getSoftwareVersion(), " SDK VERSION", file);
  return errorCode;
}

/** displayDna
*   \brief Display the value of the dna.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayDna(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the dna
  std::string dna;
  errorCode = DrmControllerOperations::readDnaRegister(dna);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the dna
  DrmControllerOperations::printRegisterNameHwReport("DNA", file);
  DrmControllerOperations::printStringRegisterValueHwReport(dna, "DNA", file);
  // return error result
  return errorCode;
}

/** displaySaasChallenge
*   \brief Display the value of the saas challenge.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displaySaasChallenge(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the saas challenge
  std::string saasChallenge;
  errorCode = DrmControllerOperations::readSaasChallengeRegister(saasChallenge);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the saas challenge
  DrmControllerOperations::printRegisterNameHwReport("SAAS CHALLENGE", file);
  DrmControllerOperations::printStringRegisterValueHwReport(saasChallenge, "SAAS CHALLENGE", file);
  return errorCode;
}

/** displayLogs
*   \brief Display the value of the logs register.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayLogs(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  unsigned int numberOfDetectedIps;
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read the logs
  std::string logs;
  errorCode = DrmControllerOperations::readLogsRegister(numberOfDetectedIps, logs);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the logs
  DrmControllerOperations::printRegisterNameHwReport("LOGS", file);
  std::string str = "0b" + logs;
  DrmControllerOperations::printStringValueHwReport(str, "LOGS", file);
  return errorCode;
}

/** displayVlnvFile
*   \brief Display the value of the vlnv file.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayVlnvFile(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  unsigned int numberOfDetectedIps;
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to vlnv file
  errorCode = DrmControllerRegisters::writeVlnvFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read vlnv file
  std::vector<std::string> vlnvFile;
  errorCode = DrmControllerOperations::readVlnvFileRegister(numberOfDetectedIps, vlnvFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the data
  DrmControllerOperations::printFileNameHwReport("VLNV", file);
  DrmControllerOperations::printStringArrayRegisterValueHwReport(vlnvFile, "VLNV ", -1, file);
  // return error result
  return errorCode;
}

/** displayLicenseFile
*   \brief Display the license file.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayLicenseFile(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  unsigned int numberOfDetectedIps;
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to license file
  errorCode = DrmControllerRegisters::writeLicenseFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read license
  std::string licenseFile;
  unsigned int licenseFileSize = DRM_CONTROLLER_LICENSE_HEADER_BLOCK_SIZE
                               + numberOfDetectedIps*DRM_CONTROLLER_LICENSE_IP_BLOCK_SIZE;
  errorCode = DrmControllerOperations::readLicenseFileRegister(licenseFileSize, licenseFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // the size of a license word
  DrmControllerOperations::printFileNameHwReport("LICENSE", file);
  DrmControllerOperations::printStringArrayRegisterValueHwReport(licenseFile, "LICENSE WORD ",
                                                                     (DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE/DRM_CONTROLLER_NIBBLE_SIZE)*
                                                                     DRM_CONTROLLER_LICENSE_WORD_WORD_NBR,
                                                                     0, file);
  // return error result
  return errorCode;
}

/** displayTraceFile
*   \brief Display the trace file.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayTraceFile(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  unsigned int numberOfDetectedIps;
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to trace file
  errorCode = DrmControllerRegisters::writeTraceFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read trace file
  std::vector<std::string> traceFile;
  errorCode = DrmControllerOperations::readTraceFileRegister(numberOfDetectedIps, traceFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // the number of digits to show the ip index
  std::ostringstream stringStream;
  stringStream << numberOfDetectedIps;
  const size_t numberOfDigitIpIndex = stringStream.str().size();
  // display the data
  DrmControllerOperations::printFileNameHwReport("TRACE", file);
  // iterate for each ips
  for (unsigned int ii = 0; ii < numberOfDetectedIps; ii++) {
    // the header
    std::stringstream sstr;
    sstr << "IP " << std::setfill ('0') << std::setw(numberOfDigitIpIndex) << std::dec << ii << " - Trace ";
    // get the trace part for the current ip
    std::vector<std::string> currentTraceFile(traceFile.begin()+ii*DRM_CONTROLLER_NUMBER_OF_TRACES_PER_IP,
                                              traceFile.begin()+(ii+1)*DRM_CONTROLLER_NUMBER_OF_TRACES_PER_IP);
    DrmControllerOperations::printStringArrayRegisterValueHwReport(currentTraceFile, sstr.str(), 0, file);
  }
  // return error result
  return errorCode;
}

/** displayMeteringFile
*   \brief Display the value of the metering file.
*   \param[in] file is the stream to use for the data display.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerLibrary::DrmControllerOperations::displayMeteringFile(std::ostream &file) const {
  // set page to register
  unsigned int errorCode = DrmControllerRegisters::writeRegistersPageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // get the number of detected ips
  unsigned int numberOfDetectedIps;
  errorCode = DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // set page to metering file
  errorCode = DrmControllerRegisters::writeMeteringFilePageRegister();
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // read metering file
  std::vector<std::string> meteringFile;
  errorCode = DrmControllerOperations::readMeteringFileRegister(numberOfDetectedIps, meteringFile);
  // check no error
  if (errorCode != mDrmApi_NO_ERROR)
    // return error result
    return errorCode;
  // display the data
  DrmControllerOperations::printFileNameHwReport("METERING", file);
  DrmControllerOperations::printStringArrayRegisterValueHwReport(meteringFile, "METERING ", 0, file);
  // return error result
  return errorCode;
}

/** printHeaderHwReport
*   \brief Print the header of the hardware report to output stream.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printHeaderHwReport(std::ostream &outputStream) const {
  // write header
  outputStream << std::right << std::setfill('-') << std::setw(79) << "-" << std::endl << std::endl;
}

/** printComponentNameHwReport
*   \brief Print the component name of the hardware report to output stream.
*   \param[in] componentName is the name of the component to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printComponentNameHwReport(const std::string &componentName,
                                                                          std::ostream &outputStream) const {
  // write component name
  outputStream << componentName << " COMPONENT" << std::endl;
}

/** printStringNameHwReport
*   \brief Print the string to output stream.
*   \param[in] name is the name to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printStringNameHwReport(const std::string &name,
                                                                       std::ostream &outputStream) const {
  // write register name
  outputStream << std::right << std::setfill(' ') << std::setw(3) << "-" << name << std::endl;
}

/** printRegisterNameHwReport
*   \brief Print the register name of the hardware report to output stream.
*   \param[in] registerName is the name of the register to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printRegisterNameHwReport(const std::string &registerName,
                                                                         std::ostream &outputStream) const {
  // write register name
  std::string str = registerName + " REGISTER";
  DrmControllerOperations::printStringNameHwReport(str, outputStream);
}

/** printVersionNameHwReport
*   \brief Print the version name of the hardware report to output stream.
*   \param[in] versionName is the name of the version to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printVersionNameHwReport(const std::string &versionName,
                                                                        std::ostream &outputStream) const {
  // write version name
  std::string str = versionName + " VERSION";
  DrmControllerOperations::printStringNameHwReport(str, outputStream);
}

/** printFileNameHwReport
*   \brief Print the file name of the hardware report to output stream.
*   \param[in] fileName is the name of the file to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printFileNameHwReport(const std::string &fileName,
                                                                     std::ostream &outputStream) const {
  // write file name
  std::string str = fileName + " FILE";
  DrmControllerOperations::printStringNameHwReport(str, outputStream);
}

/** printStringValueHwReport
*   \brief Print the value of the string register of the hardware report to output stream.
*   \param[in] value is the string value to write.
*   \param[in] desc is the description to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printStringValueHwReport(const std::string &value,
                                                                        const std::string &desc,
                                                                        std::ostream &outputStream) const {
  // write string value
  std::string str = desc + " ";
  outputStream << std::right << std::setfill(' ') << std::setw(5) << "-";
  outputStream << std::left << std::setfill('.') << std::setw(48) << str;
  outputStream << " : " << value << std::endl;
}

/** printStringRegisterValueHwReport
*   \brief Print the value of the string register of the hardware report to output stream.
*   \param[in] registerValue is the string value of the register to write.
*   \param[in] registerDesc is the description of the register to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printStringRegisterValueHwReport(const std::string &registerValue,
                                                                                const std::string &registerDesc,
                                                                                std::ostream &outputStream) const {
  // write string register value
  std::string str = "0x" + registerValue;
  DrmControllerOperations::printStringValueHwReport(str, registerDesc, outputStream);
}

/** printIntRegisterValueHwReport
*   \brief Print the value of the integer register of the hardware report to output stream.
*   \param[in] registerValue is the int value of the register to write.
*   \param[in] registerDesc is the description of the register to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printIntRegisterValueHwReport(const unsigned int &registerValue,
                                                                             const std::string &registerDesc,
                                                                             std::ostream &outputStream) const {
  // write bit register value
  std::stringstream sstr;
  sstr << registerValue;
  DrmControllerOperations::printStringValueHwReport(sstr.str(), registerDesc, outputStream);
}

/** printBitRegisterValueHwReport
*   \brief Print the value of the bit register of the hardware report to output stream.
*   \param[in] registerValue is the bit value of the register to write.
*   \param[in] registerDesc is the description of the register to write.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printBitRegisterValueHwReport(const bool &registerValue,
                                                                             const std::string &registerDesc,
                                                                             std::ostream &outputStream) const {
  // write bit register value
  std::string strDesc = registerDesc + " BIT ";
  std::stringstream strValue;
  strValue << "0b" << registerValue;
  DrmControllerOperations::printStringValueHwReport(strValue.str(), strDesc, outputStream);

}

/** printWordRegisterValueHwReport
*   \brief Print the value of the N bits word register of the hardware report to output stream.
*   \param[in] registerValue is the N bits word value of the register to write.
*   \param[in] registerDesc is the description of the register to write.
*   \param[in] numberOfNibble is the number of nibble to display.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printWordRegisterValueHwReport(const unsigned int &registerValue,
                                                                              const std::string &registerDesc,
                                                                              const unsigned int &numberOfNibble,
                                                                              std::ostream &outputStream) const {
  // write n bit register value
  std::stringstream strValue;
  strValue << "0x" << std::right << std::setfill('0') << std::setw(numberOfNibble) << std::hex << registerValue << std::dec;
  DrmControllerOperations::printStringValueHwReport(strValue.str(), registerDesc, outputStream);
}

/** printStringArrayRegisterValueHwReport
*   \brief Print the value of the array of string register of the hardware report to output stream.
*   \param[in] stringArrayValue is the array of string value of the register to write.
*   \param[in] stringDesc is the description of the register to write.
*   \param[in] startIndex is the value of index of the first word to display.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printStringArrayRegisterValueHwReport(const std::vector<std::string> &stringArrayValue,
                                                                                     const std::string &stringDesc,
                                                                                     const int &startIndex,
                                                                                     std::ostream &outputStream) const {
  // the number of digits
  std::ostringstream stringStream;
  stringStream << stringArrayValue.size();
  size_t numberOfDigit = stringStream.str().size();
  if (startIndex < 0)
    numberOfDigit++;
  // write the data
  int currentIndex = startIndex;
  for (std::vector<std::string>::const_iterator it = stringArrayValue.begin(); it != stringArrayValue.end(); it++) {
    std::string strValue = "0x" + (*it);
    std::stringstream strDesc;
    strDesc << stringDesc << std::setfill ('0') << std::setw(numberOfDigit) << std::dec << currentIndex++;
    DrmControllerOperations::printStringValueHwReport(strValue, strDesc.str(), outputStream);
  }
}

/** printStringArrayRegisterValueHwReport
*   \brief Print the value of the array of string register of the hardware report to output stream.
*   \param[in] stringValue is the string value of the register to write.
*   \param[in] stringDesc is the description of the register to write.
*   \param[in] stringWordSize is the size of a word.
*   \param[in] startIndex is the value of index of the first word to display.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printStringArrayRegisterValueHwReport(const std::string &stringValue,
                                                                                     const std::string &stringDesc,
                                                                                     const unsigned int &stringWordSize,
                                                                                     const int &startIndex,
                                                                                     std::ostream &outputStream) const {
  //
  // the number of digits for the max number of words
  std::ostringstream stringStream;
  stringStream << stringValue.size()/stringWordSize;
  const size_t numberOfDigit = stringStream.str().size();
  // loop for each element of the buffer
  for (unsigned int ii = 0; ii < stringValue.size(); ii+=stringWordSize) {
    std::string strValue = "0x" + stringValue.substr(ii, stringWordSize);
    std::stringstream strDesc;
    strDesc << stringDesc << std::setfill ('0') << std::setw(numberOfDigit) << std::dec << (ii/stringWordSize);
    DrmControllerOperations::printStringValueHwReport(strValue, strDesc.str(), outputStream);
  }
}

/** printFooterHwReport
*   \brief Print the footer of the hardware report to output stream.
*   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
**/
void DrmControllerLibrary::DrmControllerOperations::printFooterHwReport(std::ostream &outputStream) const {
  // write footer
  outputStream << std::endl << std::right << std::setfill('-') << std::setw(79) << "-" << std::endl << std::endl;
}

/** operator<<
*   \brief Declaration of function for operator <<
*   \param[in] file is the stream to use for the data display.
*   \param[in] DrmControllerOperations is a reference to this object.
*   \return Returns the output stream used
**/
std::ostream& DrmControllerLibrary::operator<<(std::ostream &file, const DrmControllerLibrary::DrmControllerOperations &DrmControllerOperations) {
  // call the display method
  DrmControllerOperations.printHwReport(file);
  // return the stream
  return file;
}
