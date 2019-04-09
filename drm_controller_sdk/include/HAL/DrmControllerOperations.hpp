/**
*  \file      DrmControllerOperations.hpp
*  \version   3.2.0.0
*  \date      December 2018
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

#ifndef __DRM_CONTROLLER_OPERATIONS_HPP__
#define __DRM_CONTROLLER_OPERATIONS_HPP__

#include <HAL/DrmControllerRegisters.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerOperations DrmControllerOperations.hpp "include/DrmControllerOperations.hpp"
  *   \brief    Class DrmControllerOperations is an abstraction level to execute operations.
  **/
  class DrmControllerOperations: public DrmControllerRegisters {

    // public members, functions ...
    public:

      /** DrmControllerOperations
      *   \brief Class constructor.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerOperations(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction);

      /** ~DrmControllerOperations
      *   \brief Class destructor.
      **/
      virtual ~DrmControllerOperations();

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
      unsigned int waitAutonomousControllerDone(bool &heartBeatModeEnabled, bool &autoEnabled, bool &autoBusy) const;

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
      unsigned int waitAutonomousControllerDone(bool &autoEnabled, bool &autoBusy);

      /** waitAutoControllerDone
      *   \brief Wait until the Autonomous Controller is done.
      *   This method will access to the system bus to check the autonomous controller status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
      *           or the error code produced by the read/write register function.
      *   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
      *          should be called to get the exception description.
      **/
      unsigned int waitAutonomousControllerDone();

      /** extractDrmVersion
      *   \brief Extract the version of the DRM controller.
      *   This method will access to the system bus to extract the drm version.
      *   \param[out] drmVersion is the value of the drm version.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractDrmVersion(std::string &drmVersion) const;

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
      unsigned int extractDna(std::string &dna, bool &dnaReady, unsigned char &dnaErrorCode) const;

      /** extractDna
      *   \brief Extract the dna value.
      *   This method will access to the system bus to extract the dna value.
      *   \param[out] dna is the value of the dna.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured,
      *           or the error code produced by the read/write register function.
      *   \throw DrmControllerTimeOutException whenever a time out error occured. DrmControllerTimeOutException::what()
      *          should be called to get the exception description.
      **/
      unsigned int extractDna(std::string &dna) const;

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
      unsigned int extractVlnvFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &vlnvFile, bool &vlnvReady, unsigned char &vlnvErrorCode) const;
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
      unsigned int extractVlnvFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &vlnvFile) const;

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
      unsigned int initialization(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                  bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady);

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
      unsigned int initialization(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile);

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
      unsigned int loadLicenseTimerInit(const std::string &licenseTimerInit, bool &licenseTimerEnabled);

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
      unsigned int loadLicenseTimerInit(const std::string &licenseTimerInit);

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
      unsigned int activate(const std::string &licenseFile, bool &activationDone, unsigned char &activationErrorCode) const;

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
      unsigned int activate(const std::string &licenseFile) const;

      /** extractLicenseFile
      *   \brief Extract the license file.
      *   This method will access to the system bus to extract the license file.
      *   \param[in]  licenseFileSize is the number of 128 bits words to extract.
      *   \param[out] licenseFile is the value of the license file.
      *   \param[out] activationDone is the value of the status bit activation done.
      *   \param[out] activationErrorCode is the value of the error code related to activation.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractLicenseFile(const unsigned int &licenseFileSize, std::string &licenseFile, bool &activationDone, unsigned char &activationErrorCode) const;
      /** extractLicenseFile
      *   \brief Extract the license file.
      *   This method will access to the system bus to extract the license file.
      *   \param[in]  licenseFileSize is the number of 128 bits words to extract.
      *   \param[out] licenseFile is the value of the license file.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractLicenseFile(const unsigned int &licenseFileSize, std::string &licenseFile) const;

      /** extractTraceFile
      *   \brief Extract the trace file.
      *   This method will access to the system bus to extract the trace file.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] traceFile is the value of the trace file.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractTraceFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &traceFile) const;

      /** extractLogs
      *   \brief Extract the logs value.
      *   This method will access to the system bus to extract the logs register.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] logs is the value of the logs.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractLogs(unsigned int &numberOfDetectedIps, std::string &logs) const;

      /** extractMeteringFile
      *   \brief Extract the metering file.
      *   This method will access to the system bus to extract the metering file.
      *   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
      *               wait the status bit license timer init loaded to be at 0.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] saasChallenge is the value of the generated challenge for the saas.
      *   \param[out] meteringFile is the list of values of the metering file.
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
      unsigned int extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                       bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady, bool &licenseTimerEnabled, bool &licenseTimerInitLoaded, unsigned char &licenseTimerLoadErrorCode) const;
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
      unsigned int extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const;

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
      unsigned int waitNotTimerInitLoaded(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, bool &licenseTimerEnabled, bool &licenseTimerInitLoaded, unsigned char &licenseTimerLoadErrorCode) const;

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
      unsigned int waitNotTimerInitLoaded(const unsigned int &waitNotLicenseTimerInitLoadedTimeout) const;

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
      unsigned int synchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile, bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady) const;

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
      unsigned int synchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const;

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
      unsigned int asynchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                   bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady, bool &asynchronousMeteringReady) const;

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
      unsigned int asynchronousExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const;

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
      unsigned int endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                    bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady, bool &endSessionMeteringReady) const;

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
      unsigned int endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile) const;

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
      unsigned int sampleLicenseTimerCounter(unsigned int &licenseTimerCountMsb, unsigned int &licenseTimerCountLsb, bool &licenseTimerEnabled, bool &licenseTimerSampleReady) const;

      /** sampleLicenseTimerCounter
      **  \brief Sample the license timer counter.
      *   This method will access to the system bus to read the status and the error, and read the license timer counter.
      *   \param[out] licenseTimerCountMsb is the value of the 32 MSB of the license timer count.
      *   \param[out] licenseTimerCountLsb is the value of the 32 LSB of the license timer count.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled or the error code produced by the read/write register function.
      *   \throw DrmControllerFunctionalityDisabledException whenever the license timer is disabled. DrmControllerFunctionalityDisabledException::what()
      *          should be called to get the exception description.
      **/
      unsigned int sampleLicenseTimerCounter(unsigned int &licenseTimerCountMsb, unsigned int &licenseTimerCountLsb) const;

      /** getDrmApiMessage
      *   \brief Get the error message from the api error code.
      *   \param[in] drmApiErrorCode is the value of the api error code.
      *   \return Returns the error message.
      **/
      const char* getDrmApiMessage(const unsigned int &drmApiErrorCode) const;

    // protected members, functions ...
    protected:

    // private members, functions ...
    private:

      const unsigned char mDrmErrorNoError;  /**<!Error code value.*/
      const unsigned char mDrmErrorNotReady; /**<!Error code value.*/

      bool mHeartBeatModeEnabled; /**<Tell wether the heart beat mode is enabled on hardware**/
      // we suppose the heart beat mode to be disabled by default
      // it will be set if it is not the case during the wait auto controller function
      
      bool mLicenseTimerWasLoaded; /**<Tell wether the license timer was loaded at least one time.**/

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
      unsigned int extractMeteringFile(unsigned int &numberOfDetectedIps, std::vector<std::string> &meteringFile, bool &meteringEnabled, bool &meteringReady) const;
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
      unsigned int extractSaasChallenge(std::string &saasChallenge, bool &saasChallengeReady) const;

      /** checkLicenseTimerInitLoaded
      *   \brief Verify if the license timer has been loaded.
      *   This method will access to the system bus to read the status bit timer init loaded and the timer load error code.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_RESETED_ERROR if the DRM Controller has been reseted,
      *           or the error code produced by the read/write register function.
      *   \throw DrmControllerLicenseTimerResetedException whenever the license timer has been reseted. DrmControllerLicenseTimerResetedException::what()
      *          should be called to get the exception description.
      **/
      unsigned int checkLicenseTimerInitLoaded() const;

      /** extractMeteringFileAndSaasChallenge
      *   \brief Extract the metering file and the saas challenge.
      *   This method will access to the system bus to extract the metering file and the saas challenge.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] saasChallenge is the value of the saas challenge.
      *   \param[out] meteringFile is the value of the metering file.
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
      unsigned int extractMeteringFileAndSaasChallenge(unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                       bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady) const;

      /** extractMeteringFileAndSaasChallenge
      *   \brief Wait not timer init loaded and then extract the metering file and the saas challenge.
      *   This method will access to the system bus to extract the metering file and the saas challenge.
      *   \param[in]  waitNotLicenseTimerInitLoadedTimeout is the timeout value in seconds used to
      *               wait the status bit license timer init loaded to be at 0.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] saasChallenge is the value of the saas challenge.
      *   \param[out] meteringFile is the value of the metering file.
      *   \param[out] meteringEnabled is the value of the status bit metering enabled.
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
      unsigned int extractMeteringFileAndSaasChallenge(const unsigned int &waitNotLicenseTimerInitLoadedTimeout, unsigned int &numberOfDetectedIps, std::string &saasChallenge, std::vector<std::string> &meteringFile,
                                                       bool &meteringEnabled, bool &saasChallengeReady, bool &meteringReady, bool &licenseTimerEnabled, bool &licenseTimerInitLoaded, unsigned char &licenseTimerLoadErrorCode) const;

  }; // class DrmControllerOperations

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_OPERATIONS_HPP__
