/**
*  \file      DrmControllerOperations.cpp
*  \version   4.1.0.0
*  \date      March 2020
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

using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** DrmControllerOperations
*   \brief Class constructor.
*   \param[in] readRegisterFunction function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] writeRegisterFunction function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
**/
DrmControllerOperations::DrmControllerOperations(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction)
  : DrmControllerRegisters(readRegisterFunction, writeRegisterFunction),
    mDrmErrorNoError(0x00),
    mDrmErrorNotReady(0xFF),
    mHeartBeatModeEnabled(false),
    mLicenseTimerWasLoaded(false)
{
  // Set the operation timeout from environment variable if existing or use default value otherwise.
  const char* timeout = std::getenv("DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS");
  mTimeoutInMicroSeconds = (timeout == NULL) ? DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS : std::stoul(std::string(timeout));

  // wait controller done for heart beat mode detection
  waitAutonomousControllerDone();
}

/** ~DrmController
*   \brief Class destructor.
**/
DrmControllerOperations::~DrmControllerOperations() { }

/** waitAutoControllerDone
*   \brief Wait until the Autonomous Controller is done.
*   This method will access to the system bus to check the autonomous controller status.
*   \param[out] heartBeatModeEnabled is the deducted value of the heart beat mode of the drm controller.
*   \param[out] autoEnabled is the value of the status bit auto controller enabled.
*   \param[out] autoBusy is the value of the status bit auto controller busy.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerOperations::waitAutonomousControllerDone(bool &heartBeatModeEnabled, bool &autoEnabled, bool &autoBusy) const {
  // get auto controller enabled status
  unsigned int errorCode = readAutonomousControllerEnabledStatusRegister(autoEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get heart beat mode enabled status
  bool heartBeatModeEnabledStatusSupported(true);
  try {
    errorCode = readHeartBeatModeEnabledStatusRegister(heartBeatModeEnabled);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  catch(DrmControllerUnsupportedFeatureException const &e) {
    // heart beat mode status is unsupported
    heartBeatModeEnabledStatusSupported = false;
  }
  // heart beat mode status is unsupported
  if (heartBeatModeEnabledStatusSupported == false) {
    // wait for activation done status
    if (autoEnabled == true) {
      try {
        bool activationDone(false);
        errorCode = waitActivationDoneStatusRegister(mTimeoutInMicroSeconds, true, activationDone);
      }
      catch (DrmControllerTimeOutException const &e) {
        throwTimeoutException("DRM Controller Initialization After Reset is in timeout");
        return mDrmApi_HARDWARE_TIMEOUT_ERROR;
      }
      if (errorCode != mDrmApi_NO_ERROR) return errorCode;
      // check auto controller busy
      errorCode = readAutonomousControllerBusyStatusRegister(autoBusy);
      if (errorCode != mDrmApi_NO_ERROR) return errorCode;
      // heartbeat mode enabled takes auto busy status value
      heartBeatModeEnabled = autoBusy;
    }
  }
  else {
    // heart beat mode status is supported
    // wait activation done status if heart beat mode is enabled
    try {
      if (heartBeatModeEnabled == true) {
        // activation done status
        bool activationDone(false);
        autoBusy = true;
        errorCode = waitActivationDoneStatusRegister(mTimeoutInMicroSeconds, true, activationDone);
      } // heartBeatModeEnabled == true
      else if (autoEnabled == true) {
        // wait autonomous controller not busy if autocontroller is enabled
        errorCode = waitAutonomousControllerBusyStatusRegister(mTimeoutInMicroSeconds, false, autoBusy);
      }
    }
    catch (DrmControllerTimeOutException const &e) {
      throwTimeoutException("DRM Controller Initialization After Reset is in timeout");
      return mDrmApi_HARDWARE_TIMEOUT_ERROR;
    }
  }
  return errorCode;
}

/** waitAutoControllerDone
*   \brief Wait until the Autonomous Controller is done.
*   This method will access to the system bus to check the autonomous controller status.
*   \param[out] autoEnabled is the value of the status bit auto controller enabled.
*   \param[out] autoBusy is the value of the status bit auto controller busy.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerOperations::waitAutonomousControllerDone(bool &autoEnabled, bool &autoBusy) {
  // wait auto controller done
  return waitAutonomousControllerDone(mHeartBeatModeEnabled, autoEnabled, autoBusy);
}

/** waitAutoControllerDone
*   \brief Wait until the Autonomous Controller is done.
*   This method will access to the system bus to check the autonomous controller status.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerOperations::waitAutonomousControllerDone() {
  // status
  bool autoEnabled(false), autoBusy(false);
  // wait controller done
  return waitAutonomousControllerDone(autoEnabled, autoBusy);
}

/** extractDrmVersion
*   \brief Extract the version of the DRM controller.
*   This method will access to the system bus to extract the drm version.
*   \param[out] drmVersion is the value of the drm version.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerOperations::extractDrmVersion(std::string &drmVersion) const {
  return readDrmVersionRegister(drmVersion);
}

/** extractDna
*   \brief Extract the dna value.
*   This method will access to the system bus to extract the dna value.
*   \param[out] dna is the value of the dna.
*   \param[out] dnaReady is the value of the status bit dna ready.
*   \param[out] dnaErrorCode is the value of the error code related to dna extraction.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerOperations::extractDna(std::string &dna, bool &dnaReady, unsigned char &dnaErrorCode) const {
  unsigned int errorCode;
  // check heart beat mode is disabled
  if (mHeartBeatModeEnabled == false) {
    // set nop command
    errorCode = writeNopCommandRegister();
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    // set dna extract command
    errorCode = writeDnaExtractCommandRegister();
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  try {
    // wait dna ready status
    errorCode = waitDnaReadyStatusRegister(mTimeoutInMicroSeconds, true, dnaReady);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    errorCode = waitExtractDnaErrorRegister(mTimeoutInMicroSeconds, mDrmErrorNoError, dnaErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  catch (DrmControllerTimeOutException const &e) {
    errorCode = readExtractDnaErrorRegister(dnaErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    throwTimeoutException("DRM Controller DNA Extraction is in timeout", "Expected DNA Ready Status Bit", "Actual DNA Ready Status Bit",
                          "Expected DNA Extract Error", "Actual DNA Extract Error", true, dnaReady, mDrmErrorNoError, dnaErrorCode);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  // read dna
  return readDnaRegister(dna);
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
unsigned int DrmControllerOperations::extractDna(std::string &dna) const {
  // status and error
  bool dnaReady(false);
  unsigned char dnaErrorCode(mDrmErrorNotReady);
  // extract dna
  return extractDna(dna, dnaReady, dnaErrorCode);
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
unsigned int DrmControllerOperations::extractVlnvFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &vlnvFile, bool &vlnvReady, unsigned char &vlnvErrorCode) const {
  unsigned int errorCode;
  // check heart beat mode is disabled
  if (mHeartBeatModeEnabled == false) {
    // set nop command
    errorCode = writeNopCommandRegister();
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    // set vlnv extract command
    errorCode = writeVlnvExtractCommandRegister();
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  try {
    // wait vlnv extract status and error
    errorCode = waitVlnvReadyStatusRegister(mTimeoutInMicroSeconds, true, vlnvReady);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    errorCode = waitExtractVlnvErrorRegister(mTimeoutInMicroSeconds, mDrmErrorNoError, vlnvErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  catch (DrmControllerTimeOutException const &e) {
    errorCode = readExtractVlnvErrorRegister(vlnvErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    throwTimeoutException("DRM Controller VLNV Extraction is in timeout", "Expected VLNV Ready Status Bit", "Actual VLNV Ready Status Bit",
                          "Expected VLNV Extract Error", "Actual VLNV Extract Error", true, vlnvReady, mDrmErrorNoError, vlnvErrorCode);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  // get the number of detected ips
  errorCode = readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get vlnv file
  return readVlnvFileRegister(numberOfDetectedIps, vlnvFile);
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
unsigned int DrmControllerOperations::extractVlnvFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &vlnvFile) const {
  // status and error
  bool vlnvReady(false);
  unsigned char vlnvErrorCode(mDrmErrorNotReady);
  // extract vlnv file
  return extractVlnvFile(numberOfDetectedIps, vlnvFile, vlnvReady, vlnvErrorCode);
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
unsigned int DrmControllerOperations::initialization(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                     bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady) {
  // the license timer is not loaded
  mLicenseTimerWasLoaded = false;
  // call extract metering file and saas challenge
  return extractMeteringFileAndSaasChallenge(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
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
unsigned int DrmControllerOperations::initialization(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) {
  // status bits
  bool meteringEnabled(false), saasChallengeReady(false), meteringReady(false);
  // extract metering file
  return initialization(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
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
unsigned int DrmControllerOperations::loadLicenseTimerInit(const std::string &licenseTimerInit, bool &licenseTimerEnabled) {
  // check license timer enabled status
  unsigned int errorCode = checkLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // check license timer if it was loaded the previous call
  if (mLicenseTimerWasLoaded == true) {
    errorCode = checkLicenseTimerInitLoaded();
    if (errorCode != mDrmApi_NO_ERROR) {
      mLicenseTimerWasLoaded = false;
      return errorCode;
    }
  }
  // write license timer value
  errorCode = writeLicenseTimerInitRegister(licenseTimerInit);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // everything was fine, the license timer has been loaded
  mLicenseTimerWasLoaded = true;
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
unsigned int DrmControllerOperations::loadLicenseTimerInit(const std::string &licenseTimerInit) {
  // status
  bool licenseTimerEnabled(false);
  // load license timer
  return loadLicenseTimerInit(licenseTimerInit, licenseTimerEnabled);
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
unsigned int DrmControllerOperations::activate(const std::string &licenseFile, bool &activationDone, unsigned char &activationErrorCode) const {
  unsigned int errorCode;
  // set the license file
  errorCode = writeLicenseFileRegister(licenseFile);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // set license start address
  std::vector<unsigned int> licenseStartAddress = { 0, 0 };
  errorCode = writeLicenseStartAddressRegister(licenseStartAddress);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // check heart beat mode is disabled
  if (mHeartBeatModeEnabled == false) {
    // set nop command
    errorCode = writeNopCommandRegister();
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    // set activate command
    errorCode = writeActivateCommandRegister();
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  } // mHeartBeatModeEnabled == false
  try {
    // wait activation status and error
    errorCode = waitActivationDoneStatusRegister(mTimeoutInMicroSeconds, true, activationDone);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    errorCode = waitActivationErrorRegister(mTimeoutInMicroSeconds, mDrmErrorNoError, activationErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  catch (DrmControllerTimeOutException const &e) {
    errorCode = readActivationErrorRegister(activationErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    throwTimeoutException("DRM Controller Activation is in timeout", "Expected Activation Done Status Bit", "Actual Activation Done Status Bit",
                          "Expected Activation Error", "Actual Activation Error", true, activationDone, mDrmErrorNoError, activationErrorCode);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
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
unsigned int DrmControllerOperations::activate(const std::string &licenseFile) const {
  // status and error
  bool activationDone(false);
  unsigned char activationErrorCode(mDrmErrorNotReady);
  // activate
  return activate(licenseFile, activationDone, activationErrorCode);
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
unsigned int DrmControllerOperations::extractLicenseFile(const unsigned int &licenseFileSize, std::string &licenseFile, bool &activationDone, unsigned char &activationErrorCode) const {
  unsigned int errorCode;
  // get activation done status
  errorCode = readActivationDoneStatusRegister(activationDone);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get activation error
  errorCode = readActivationErrorRegister(activationErrorCode);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // read the license file
  return readLicenseFileRegister(licenseFileSize, licenseFile);
}

/** extractLicenseFile
*   \brief Extract the license file.
*   This method will access to the system bus to extract the license file.
*   \param[in]  licenseFileSize is the number of 128 bits words to extract.
*   \param[out] licenseFile is the value of the license file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerOperations::extractLicenseFile(const unsigned int &licenseFileSize, std::string &licenseFile) const {
  // status and error
  bool activationDone(false);
  unsigned char activationErrorCode(mDrmErrorNotReady);
  // extract license file
  return extractLicenseFile(licenseFileSize, licenseFile, activationDone, activationErrorCode);
}

/** extractTraceFile
*   \brief Extract the trace file.
*   This method will access to the system bus to extract the trace file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] traceFile is the value of the trace file.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerOperations::extractTraceFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &traceFile) const {
  unsigned int errorCode;
  // get the number of detected ips
  errorCode = readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // read the trace file
  return readTraceFileRegister(numberOfDetectedIps, traceFile);
}

/** extractLogs
*   \brief Extract the logs value.
*   This method will access to the system bus to extract the logs register.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] logs is the value of the logs.
*   \return Returns the error code produced by the read/write register function.
**/
unsigned int DrmControllerOperations::extractLogs(unsigned int &numberOfDetectedIps, std::string &logs) const {
  // get the number of detected ips
  unsigned int errorCode = readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // read logs
  return readLogsRegister(numberOfDetectedIps, logs);
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
*   \deprecated This function is kept for backward compatibility. Use the combination of functions waitNotTimerInitLoaded() and synchronousExtractMeteringFile() instead.
**/
unsigned int DrmControllerOperations::extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                          bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady, bool &licenseTimerEnabled, bool &licenseTimerInitLoaded, unsigned char &licenseTimerLoadErrorCode) const {
  // call extract metering file and saas challenge
  return extractMeteringFileAndSaasChallenge(waitNotLicenseTimerInitLoadedTimeout, numberOfDetectedIps, saasChallenge, meteringFile,
                                             meteringEnabled, saasChallengeReady, meteringReady, licenseTimerEnabled, licenseTimerInitLoaded, licenseTimerLoadErrorCode);
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
*   \deprecated This function is kept for backward compatibility. Use the combination of functions waitNotTimerInitLoaded() and synchronousExtractMeteringFile() instead.
**/
unsigned int DrmControllerOperations::extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const {
  // status bits
  bool meteringEnabled(false), saasChallengeReady(false), meteringReady(false), licenseTimerEnabled(false), licenseTimerInitLoaded(false);
  unsigned char licenseTimerLoadErrorCode(mDrmErrorNotReady);
  // extract metering file
  return extractMeteringFile(waitNotLicenseTimerInitLoadedTimeout, numberOfDetectedIps, saasChallenge, meteringFile,
                             meteringEnabled, saasChallengeReady, meteringReady, licenseTimerEnabled, licenseTimerInitLoaded, licenseTimerLoadErrorCode);
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
unsigned int DrmControllerOperations::waitNotTimerInitLoaded(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, bool &licenseTimerEnabled, bool &licenseTimerInitLoaded, unsigned char &licenseTimerLoadErrorCode) const {
  // check license timer enabled status
  unsigned int errorCode = checkLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // wait license timer load status and error
  try {
    errorCode = waitLicenseTimerInitLoadedStatusRegister(waitNotLicenseTimerInitLoadedTimeout*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND, false, licenseTimerInitLoaded);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    errorCode = waitLicenseTimerLoadErrorRegister(waitNotLicenseTimerInitLoadedTimeout*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND, mDrmErrorNoError, licenseTimerLoadErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  }
  catch (DrmControllerTimeOutException const &e) {
    errorCode = readLicenseTimerLoadErrorRegister(licenseTimerLoadErrorCode);
    if (errorCode != mDrmApi_NO_ERROR) return errorCode;
    throwTimeoutException("DRM Controller Load License Timer Init is in timeout", "Expected License Timer Init Loaded Status Bit", "Actual License Timer Init Loaded Status Bit",
                          "Expected Load License Timer Init Error", "Actual Load License Timer Init Error", true, licenseTimerInitLoaded, mDrmErrorNoError, licenseTimerLoadErrorCode);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  return errorCode;
}

/** waitNotTimerInitLoaded
*   \brief Wait not timer init loaded.
*   This method will access to the system bus to read the status bit timer init loaded and the timer load error code.
*   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
*               wait the status bit license timer init loaded to be at 0.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::waitNotTimerInitLoaded(const unsigned int &waitNotLicenseTimerInitLoadedTimeout) const {
  // status bits
  bool licenseTimerEnabled(false), licenseTimerInitLoaded(false);
  unsigned char licenseTimerLoadErrorCode(mDrmErrorNotReady);
  // wait license timer init not loaded
  return waitNotTimerInitLoaded(waitNotLicenseTimerInitLoadedTimeout, licenseTimerEnabled, licenseTimerInitLoaded, licenseTimerLoadErrorCode);
}

/** synchronousExtractMeteringFile
*   \brief Extract the metering file.
*   This method will access to the system bus to extract the metering file.
*   \param[out] numberOfDetectedIps is the number of detected IPs,
*               excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the generated challenge for the saas.
*   \param[out] meteringFile is the list of values of the metering file.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::synchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                                     bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady) const {
  // extract metering file and saas challenge
  return extractMeteringFileAndSaasChallenge(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
}

/** synchronousExtractMeteringFile
*   \brief Extract the metering file.
*   This method will access to the system bus to extract the metering file.
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
unsigned int DrmControllerOperations::synchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const {
  // status bits
  bool meteringEnabled(false), saasChallengeReady(false), meteringReady(false);
  // extract metering file
  return synchronousExtractMeteringFile(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
}

/** asynchronousExtractMeteringFile
*   \brief Extract the metering file and the saas challenge after writing the command extract metering.
*   This method will access to the system bus to extract the metering file and the saas challenge.
*   \param[out] numberOfDetectedIps is the number of detected IPs, excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \param[out] asynchronousMeteringReady is the value of the status bit asynchronous metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::asynchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile, bool &meteringEnabled,
                                                                      bool &saasChallengeReady, bool &meteringReady, bool &asynchronousMeteringReady) const {
  // check metering enabled status
  unsigned int errorCode = checkMeteringEnabledStatusRegister(meteringEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // write asynchronous metering extract command
  errorCode = writeMeteringExtractCommandRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  try {
    // wait asynchronous metering ready status
    errorCode = waitAsynchronousMeteringReadyStatusRegister(mTimeoutInMicroSeconds, true, asynchronousMeteringReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller Asynchronous Metering Extract is in timeout", "Expected Asynchronous Metering Ready Status Bit",
                          "Actual Asynchronous Metering Ready Status Bit", true, asynchronousMeteringReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // extract metering file and saas challenge
  errorCode = extractMeteringFileAndSaasChallenge(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // write nop command
  errorCode = writeNopCommandRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // wait not asynchronous metering ready status
  try {
    errorCode = waitAsynchronousMeteringReadyStatusRegister(mTimeoutInMicroSeconds, false, asynchronousMeteringReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller Asynchronous Metering Extract is in timeout", "Expected Asynchronous Metering Ready Status Bit",
                          "Actual Asynchronous Metering Ready Status Bit", false, asynchronousMeteringReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  return errorCode;
}

/** asynchronousExtractMeteringFile
*   \brief Extract the metering file and the saas challenge after writing the command extract metering.
*   This method will access to the system bus to extract the metering file and the saas challenge.
*   \param[out] numberOfDetectedIps is the number of detected IPs, excluding the drm controller ip.
*   \param[out] saasChallenge is the value of the saas challenge.
*   \param[out] meteringFile is the value of the metering file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
*           mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, or the error code produced by the read/write register function.
*   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
*          should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException whenever the metering is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::asynchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const {
  // status bits
  bool meteringEnabled(false), saasChallengeReady(false), meteringReady(false), asynchronousMeteringReady(false);
  // asynchronous extract metering file
  return asynchronousExtractMeteringFile(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady, asynchronousMeteringReady);
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
unsigned int DrmControllerOperations::endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile, bool &meteringEnabled,
                                                                       bool &saasChallengeReady, bool &meteringReady, bool &endSessionMeteringReady) const {
  // check metering enabled status
  unsigned int errorCode = checkMeteringEnabledStatusRegister(meteringEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // set end session extract metering command
  errorCode = writeEndSessionMeteringExtractCommandRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  try {
    // wait end session metering ready status
    errorCode = waitEndSessionMeteringReadyStatusRegister(mTimeoutInMicroSeconds, true, endSessionMeteringReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller End Session Metering Extract is in timeout", "Expected End Session Metering Ready Status Bit",
                          "Actual End Session Metering Ready Status Bit", true, endSessionMeteringReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // extract metering file and saas challenge
  errorCode = extractMeteringFileAndSaasChallenge(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // write nop command
  errorCode = writeNopCommandRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  try {
    // wait not end session metering ready status
    errorCode = waitEndSessionMeteringReadyStatusRegister(mTimeoutInMicroSeconds, false, endSessionMeteringReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller End Session Metering Extract is in timeout", "Expected End Session Metering Ready Status Bit",
                          "Actual End Session Metering Ready Status Bit", false, endSessionMeteringReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  return errorCode;
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
unsigned int DrmControllerOperations::endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const {
  // status bits
  bool meteringEnabled(false), saasChallengeReady(false), meteringReady(false), endSessionMeteringReady(false);
  // end session extract metering file
  return endSessionAndExtractMeteringFile(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady, endSessionMeteringReady);
}

/** sampleLicenseTimerCounter
**  \brief Sample the license timer counter.
*   This method will access to the system bus to read the status and the error, and read the license timer counter.
*   \param[out] licenseTimerCountMsb is the value of the 32 MSB of the license timer count.
*   \param[out] licenseTimerCountLsb is the value of the 32 LSB of the license timer count.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \param[out] licenseTimerSampleReady is the value of the status bit license timer sample ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::sampleLicenseTimerCounter(unsigned int &licenseTimerCountMsb, unsigned int &licenseTimerCountLsb, bool &licenseTimerEnabled, bool &licenseTimerSampleReady) const {
  // check license timer enabled status
  unsigned int errorCode = checkLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // write sample license timer command
  errorCode = writeSampleLicenseTimerCounterCommandRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  try {
    // wait license timer sample ready status
    errorCode = waitLicenseTimerSampleReadyStatusRegister(mTimeoutInMicroSeconds, true, licenseTimerSampleReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller License Timer Counter Sample is in timeout", "Expected License Timer Sample Ready Status Bit",
                          "Actual License Timer Sample Ready Status Bit", true, licenseTimerSampleReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // read license timer counter
  std::vector<unsigned int> licenseTimerCount;
  errorCode = readLicenseTimerCounterRegister(licenseTimerCount);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  licenseTimerCountMsb = licenseTimerCount.at(0);
  licenseTimerCountLsb = licenseTimerCount.at(1);
  // write register page to registers
  errorCode = writeNopCommandRegister();
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  try {
    // wait not license timer sample ready status
    errorCode = waitLicenseTimerSampleReadyStatusRegister(mTimeoutInMicroSeconds, false, licenseTimerSampleReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller License Timer Counter Sample is in timeout", "Expected License Timer Sample Ready Status Bit",
                          "Actual License Timer Sample Ready Status Bit", false, licenseTimerSampleReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  return errorCode;
}

/** sampleLicenseTimerCounter
**  \brief Sample the license timer counter.
*   This method will access to the system bus to read the status and the error, and read the license timer counter.
*   \param[out] licenseTimerCountMsb is the value of the 32 MSB of the license timer count.
*   \param[out] licenseTimerCountLsb is the value of the 32 LSB of the license timer count.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
*   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::sampleLicenseTimerCounter(unsigned int &licenseTimerCountMsb, unsigned int &licenseTimerCountLsb) const {
  // status
  bool licenseTimerEnabled(false), licenseTimerSampleReady(false);
  // sample license timer
  return sampleLicenseTimerCounter(licenseTimerCountMsb, licenseTimerCountLsb, licenseTimerEnabled, licenseTimerSampleReady);
}

/** getDrmApiMessage
*   \brief Get the error message from the api error code.
*   \param[in] drmApiErrorCode is the value of the api error code.
*   \return Returns the error message.
**/
const char* DrmControllerOperations::getDrmApiMessage(const unsigned int &drmApiErrorCode) const {
  // iterate on each element of the error array
  for (unsigned int ii = 0; ii < mDrmApiErrorArraySize; ii++) {
    // verify error register to return the error text
    if (mDrmApiErrorArray[ii].mDrmApiErrorCode == (tDrmApiErrorCode)drmApiErrorCode) {
      std::ostringstream stringStream;
      stringStream << mDrmApiErrorArray[ii].mDrmApiErrorText;
      return stringStream.str().c_str();
    }
  }
  // return by default an unknown error
  return "UNKNOWN ERROR";
}

/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/

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
unsigned int DrmControllerOperations::extractMeteringFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &meteringFile, bool &meteringEnabled, bool &meteringReady) const {
  // check metering enabled status
  unsigned int errorCode = checkMeteringEnabledStatusRegister(meteringEnabled);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  try {
    // wait metering ready status
    errorCode = waitMeteringReadyStatusRegister(mTimeoutInMicroSeconds, true, meteringReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller Metering Extract is in timeout", "Expected Metering Ready Status Bit",
                          "Actual Metering Ready Status Bit", true, meteringReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get the number of detected ips
  errorCode = readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get metering file
  return readMeteringFileRegister(numberOfDetectedIps, meteringFile);
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
unsigned int DrmControllerOperations::extractSaasChallenge(std::string &saasChallenge, bool &saasChallengeReady) const {
  unsigned int errorCode(mDrmApi_NO_ERROR);
  try {
    // wait saas challenge ready status
    errorCode = waitSaasChallengeReadyStatusRegister(mTimeoutInMicroSeconds, true, saasChallengeReady);
  }
  catch (DrmControllerTimeOutException const &e) {
    throwTimeoutException("DRM Controller Saas Challenge Extract is in timeout", "Expected Saas Challenge Ready Status Bit",
                          "Actual Saas Challenge Ready Status Bit", true, saasChallengeReady);
    return mDrmApi_HARDWARE_TIMEOUT_ERROR;
  }
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get saas challenge
  return readSaasChallengeRegister(saasChallenge);
}

/** checkLicenseTimerInitLoaded
*   \brief Verify if the license timer has been loaded.
*   This method will access to the system bus to read the status bit timer init loaded and the timer load error code.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_RESETED_ERROR if the DRM Controller has been reseted,
*           or the error code produced by the read/write register function.
*   \throw DrmControllerLicenseTimerResetedException whenever the license timer has been reseted. DrmControllerLicenseTimerResetedException::what()
*          should be called to get the exception description.
**/
unsigned int DrmControllerOperations::checkLicenseTimerInitLoaded() const {
  // get license timer load done bit status
  bool licenseTimerInitLoaded;
  unsigned int errorCode = readLicenseTimerInitLoadedStatusRegister(licenseTimerInitLoaded);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // get license timer load error code
  unsigned char licenseTimerLoadErrorCode;
  errorCode = readLicenseTimerLoadErrorRegister(licenseTimerLoadErrorCode);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // verify license timer load error
  if (licenseTimerLoadErrorCode == mDrmErrorNotReady) {
    throwLicenseTimerResetedException(licenseTimerInitLoaded, licenseTimerInitLoaded, mDrmErrorNoError, licenseTimerLoadErrorCode);
    return mDrmApi_LICENSE_TIMER_RESETED_ERROR;
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
unsigned int DrmControllerOperations::extractMeteringFileAndSaasChallenge(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                                          bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady) const {
  // extract metering file
  unsigned int errorCode = extractMeteringFile(numberOfDetectedIps, meteringFile, meteringEnabled, meteringReady);
  if (errorCode != mDrmApi_NO_ERROR) return errorCode;
  // extract saas challenge
  return extractSaasChallenge(saasChallenge, saasChallengeReady);
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
unsigned int DrmControllerOperations::extractMeteringFileAndSaasChallenge(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile, bool &meteringEnabled,
                                                                          bool &saasChallengeReady, bool &meteringReady, bool &licenseTimerEnabled, bool &licenseTimerInitLoaded, unsigned char &licenseTimerLoadErrorCode) const {
  // wait not timer init loaded
  unsigned int errorCode = waitNotTimerInitLoaded(waitNotLicenseTimerInitLoadedTimeout, licenseTimerEnabled, licenseTimerInitLoaded, licenseTimerLoadErrorCode);
  if (errorCode != mDrmApi_NO_ERROR || licenseTimerLoadErrorCode != mDrmErrorNoError) return errorCode;
  // extract metering file and saas challenge
  return extractMeteringFileAndSaasChallenge(numberOfDetectedIps, saasChallenge, meteringFile, meteringEnabled, saasChallengeReady, meteringReady);
}
