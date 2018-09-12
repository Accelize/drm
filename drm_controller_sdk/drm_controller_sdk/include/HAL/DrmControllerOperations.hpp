/**
*  \file      DrmControllerOperations.hpp
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

#ifndef __DRM_CONTROLLER_OPERATIONS_HPP__
#define __DRM_CONTROLLER_OPERATIONS_HPP__

#include <string>
#include <vector>
#include <bitset>
#include <time.h>
#include <sys/time.h>

#include <DrmControllerTimeOutException.hpp>
#include <DrmControllerLicenseFileSizeException.hpp>
#include <DrmControllerLicenseTimerResetedException.hpp>
#include <DrmControllerFunctionalityDisabledException.hpp>
#include <DrmControllerVersion.hpp>
#include <DrmControllerDataConverter.hpp>
#include <HAL/DrmControllerRegisters.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerOperations DrmControllerOperations.hpp "include/DrmControllerOperations.hpp"
  *   \brief    Class DrmControllerOperations is an abstraction level to execute operations.
  **/
  class DrmControllerOperations : public DrmControllerRegisters {

    // public members, functions ...
    public :

      /** DrmControllerOperations
      *   \brief Class constructor.
      *   \param[in] f_read_reg32 function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] f_write_reg32 function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerOperations(t_drmReadRegisterFunction f_read_reg32,
                              t_drmWriteRegisterFunction f_write_reg32);

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
      unsigned int waitAutonomousControllerDone(bool &heartBeatModeEnabled,
                                                bool &autoEnabled,
                                                bool &autoBusy) const;

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
      unsigned int waitAutonomousControllerDone(bool &autoEnabled,
                                                bool &autoBusy);

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
      unsigned int extractDna(std::string &dna,
                              bool &dnaReady,
                              unsigned char &dnaErrorCode) const;

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
      unsigned int extractVlnvFile(unsigned int &numberOfDetectedIps,
                                   std::vector<std::string> &vlnvFile,
                                   bool &vlnvReady,
                                   unsigned char &vlnvErrorCode) const;
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
      unsigned int extractVlnvFile(unsigned int &numberOfDetectedIps,
                                   std::vector<std::string> &vlnvFile) const;

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
      unsigned int initialization(unsigned int &numberOfDetectedIps,
                                  std::string &saasChallenge,
                                  std::vector<std::string> &meteringFile,
                                  bool &meteringEnabled,
                                  bool &saasChallengeReady,
                                  bool &meteringReady) const;

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
      unsigned int initialization(unsigned int &numberOfDetectedIps,
                                  std::string &saasChallenge,
                                  std::vector<std::string> &meteringFile) const;

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
      unsigned int loadLicenseTimerInit(const std::string &licenseTimerInit,
                                        bool &licenseTimerEnabled) const;

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
      unsigned int loadLicenseTimerInit(const std::string &licenseTimerInit) const;

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
      unsigned int activate(const std::string &licenseFile,
                            bool &activationDone,
                            unsigned char &activationErrorCode) const;

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
      unsigned int extractLicenseFile(const unsigned int &licenseFileSize,
                                      std::string &licenseFile,
                                      bool &activationDone,
                                      unsigned char &activationErrorCode) const;
      /** extractLicenseFile
      *   \brief Extract the license file.
      *   This method will access to the system bus to extract the license file.
      *   \param[in]  licenseFileSize is the number of 128 bits words to extract.
      *   \param[out] licenseFile is the value of the license file.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractLicenseFile(const unsigned int &licenseFileSize,
                                      std::string &licenseFile) const;

      /** extractTraceFile
      *   \brief Extract the trace file.
      *   This method will access to the system bus to extract the trace file.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] traceFile is the value of the trace file.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractTraceFile(unsigned int &numberOfDetectedIps,
                                    std::vector<std::string> &traceFile) const;

      /** extractLogs
      *   \brief Extract the logs value.
      *   This method will access to the system bus to extract the logs register.
      *   \param[out] numberOfDetectedIps is the number of detected IPs,
      *               excluding the drm controller ip.
      *   \param[out] logs is the value of the logs.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int extractLogs(unsigned int &numberOfDetectedIps,
                               std::string &logs) const;

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
      **/
      unsigned int extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                       unsigned int &numberOfDetectedIps,
                                       std::string &saasChallenge,
                                       std::vector<std::string> &meteringFile,
                                       bool &meteringEnabled,
                                       bool &saasChallengeReady,
                                       bool &meteringReady,
                                       bool &licenseTimerEnabled,
                                       bool &licenseTimerInitLoaded,
                                       unsigned char &licenseTimerLoadErrorCode) const;
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
      unsigned int extractMeteringFile(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                       unsigned int &numberOfDetectedIps,
                                       std::string &saasChallenge,
                                       std::vector<std::string> &meteringFile) const;

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
      unsigned int endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps,
                                                    std::string &saasChallenge,
                                                    std::vector<std::string> &meteringFile,
                                                    bool &meteringEnabled,
                                                    bool &saasChallengeReady,
                                                    bool &meteringReady,
                                                    bool &endSessionMeteringReady) const;

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
      unsigned int endSessionAndExtractMeteringFile(unsigned int &numberOfDetectedIps,
                                                    std::string &saasChallenge,
                                                    std::vector<std::string> &meteringFile) const;

      /** writeLicenseStartAddressRegister
      *   \brief Write the value of the license start address.
      *   This method will access to the system bus to write the license start address register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[in] licenseStartAddress is the license start address value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int writeLicenseStartAddressRegister(const std::string &licenseStartAddress) const;

      /** readLicenseStartAddressRegister
      *   \brief Read the value of the license start address.
      *   This method will access to the system bus to read the license start address register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[in] licenseStartAddress is the license start address value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readLicenseStartAddressRegister(std::string &licenseStartAddress) const;

      /** writeLicenseTimerInitRegister
      *   \brief Write the value of the license timer.
      *   This method will access to the system bus to write the license timer register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[in] licenseTimerInit is the license timer value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int writeLicenseTimerInitRegister(const std::string &licenseTimerInit) const;

      /** readLicenseTimerInitRegister
      *   \brief Read the value of the license timer.
      *   This method will access to the system bus to read the license timer register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[out] licenseTimerInit is the license timer value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readLicenseTimerInitRegister(std::string &licenseTimerInit) const;

      /** readDrmVersionRegister
      *   \brief Read the drm version register and get the value.
      *   This method will access to the system bus to read the drm version register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[out] drmVersion is the drm version value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readDrmVersionRegister(std::string &drmVersion) const;

      /** readDnaRegister
      *   \brief Read the dna register and get the value.
      *   This method will access to the system bus to read the dna register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[out] dna is the dna value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readDnaRegister(std::string &dna) const;

      /** readSaasChallengeRegister
      *   \brief Read the saas challenge register and get the value.
      *   This method will access to the system bus to read the saas challenge register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[out] saasChallenge is the saas challenge value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readSaasChallengeRegister(std::string &saasChallenge) const;

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
      unsigned int readVlnvFileRegister(const unsigned int numberOfIPs,
                                        std::vector<std::string> &vlnvFile) const;

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
      unsigned int writeLicenseFileRegister(const std::string &licenseFile) const;

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
      unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize,
                                           std::string &licenseFile) const;

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   Before calling this function, the method writeRegistersPage() must be called
      *   in order to write the page to select the registers page.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readLogsRegister(const unsigned int numberOfIPs,
                                    std::string &logs) const;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   Before calling this function, the method writeTraceFilePage() must be called
      *   in order to write the page to select the trace file page.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readTraceFileRegister(const unsigned int numberOfIPs,
                                         std::vector<std::string> &traceFile) const;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   Before calling this function, the method writeMeteringFilePage() must be called
      *   in order to write the page to select the metering file page.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int readMeteringFileRegister(const unsigned int numberOfIPs,
                                            std::vector<std::string> &meteringFile) const;

      /** getDrmErrorRegisterMessage
      *   \brief Get the error message from the error register value.
      *   \param[in] errorRegister is the value of the error register.
      *   \return Returns the error message.
      **/
      static const char* getDrmErrorRegisterMessage(const unsigned char &errorRegister);

      /** getDrmApiMessage
      *   \brief Get the error message from the api error code.
      *   \param[in] drmApiErrorCode is the value of the api error code.
      *   \return Returns the error message.
      **/
      const char* getDrmApiMessage(const unsigned int &drmApiErrorCode) const;

      /** printHwReport
      * \brief Print all register content accessible through AXI-4 Lite Control channel.
      * \param[in] file: Reference to output file where register contents are saved. By default print on standard output
      * \return void.
      */
      virtual void printHwReport(std::ostream &file = std::cout) const;

    // protected members, functions ...
    protected :

    // private members, functions ...
    private :

      bool mHeartBeatModeEnabled; /**<Tell wether the heart beat mode is enabled on hardware**/
      // we suppose the heart beat mode to be disabled by default
      // it will be set if it is not the case during the wait auto controller function

      /** checkVersion
      *   \brief Check hardware version matches the software version.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_VERSION_CHECK_ERROR if the hardware
      *           version do not match, the software version or the error code produced by the read/write register function.
      *   \throw DrmControllerVersionCheckException whenever an error occured. DrmControllerVersionCheckException::what()
      *          should be called to get the exception description.
      **/
      unsigned int checkVersion() const;

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
      unsigned int extractMeteringFile(unsigned int &numberOfDetectedIps,
                                       std::vector<std::string> &meteringFile,
                                       bool &meteringEnabled,
                                       bool &meteringReady) const;
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
      unsigned int extractSaasChallenge(std::string &saasChallenge,
                                        bool &saasChallengeReady) const;

      /** checkLicenseTimerInitLoaded
      *   \brief Verify if the license timer has been loaded.
      *   This method will access to the system bus to read the status bit timer init loaded and the timer load error code.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_LICENSE_TIMER_RESETED_ERROR if the DRM Controller has been reseted,
      *           or the error code produced by the read/write register function.
      *   \throw DrmControllerLicenseTimerResetedException whenever the license timer has been reseted. DrmControllerLicenseTimerResetedException::what()
      *          should be called to get the exception description.
      **/
      unsigned int checkLicenseTimerInitLoaded() const;

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
      unsigned int waitNotTimerInitLoaded(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                          bool &licenseTimerEnabled,
                                          bool &licenseTimerInitLoaded,
                                          unsigned char &licenseTimerLoadErrorCode) const;

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
      unsigned int extractMeteringFileAndSaasChallenge(unsigned int &numberOfDetectedIps,
                                                       std::string &saasChallenge,
                                                       std::vector<std::string> &meteringFile,
                                                       bool &meteringEnabled,
                                                       bool &saasChallengeReady,
                                                       bool &meteringReady) const;

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
      unsigned int extractMeteringFileAndSaasChallenge(const unsigned int &waitNotLicenseTimerInitLoadedTimeout,
                                                       unsigned int &numberOfDetectedIps,
                                                       std::string &saasChallenge,
                                                       std::vector<std::string> &meteringFile,
                                                       bool &meteringEnabled,
                                                       bool &saasChallengeReady,
                                                       bool &meteringReady,
                                                       bool &licenseTimerEnabled,
                                                       bool &licenseTimerInitLoaded,
                                                       unsigned char &licenseTimerLoadErrorCode) const;

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
      **/
      unsigned int waitStatusAndError(const unsigned int &timeout,
                                      const t_drmStatusRegisterEnumValues &statusBitPosition,
                                      const t_drmStatusRegisterMaskEnumValues &statusMask,
                                      const bool &expectedStatus,
                                      const t_drmErrorRegisterBytePositionEnumValues &errorPosition,
                                      const t_drmErrorRegisterByteMaskEnumValues &errorMask,
                                      const unsigned char &expectedError,
                                      bool &status,
                                      unsigned char &error,
                                      const bool& readStatusOnly = false) const;

      /** waitDnaExtractStatusAndError
      *   \brief Wait status and error of dna extract operations to reach specified value.
      *   This method will access to the system bus to read status and error registers.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \param[out] error is the value of the error byte read.
      **/
      unsigned int waitDnaExtractStatusAndError(const unsigned int &timeout,
                                                const bool &expectedStatus,
                                                const unsigned char &expectedError,
                                                bool &status,
                                                unsigned char &error) const;

      /** waitVlnvExtractStatusAndError
      *   \brief Wait status and error of vlnv extract operations to reach specified value.
      *   This method will access to the system bus to read status and error registers.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \param[out] error is the value of the error byte read.
      **/
      unsigned int waitVlnvExtractStatusAndError(const unsigned int &timeout,
                                                 const bool &expectedStatus,
                                                 const unsigned char &expectedError,
                                                 bool &status,
                                                 unsigned char &error) const;

      /** waitActivationStatusAndError
      *   \brief Wait status and error of activation operations to reach specified value.
      *   This method will access to the system bus to read status and error registers.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \param[out] error is the value of the error byte read.
      **/
      unsigned int waitActivationStatusAndError(const unsigned int &timeout,
                                                const bool &expectedStatus,
                                                const unsigned char &expectedError,
                                                bool &status,
                                                unsigned char &error) const;

      /** waitLicenseTimerLoadStatusAndError
      *   \brief Wait status and error of license timer load operations to reach specified value.
      *   This method will access to the system bus to read status and error registers.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \param[out] error is the value of the error byte read.
      **/
      unsigned int waitLicenseTimerLoadStatusAndError(const unsigned int &timeout,
                                                      const bool &expectedStatus,
                                                      const unsigned char &expectedError,
                                                      bool &status,
                                                      unsigned char &error) const;

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
      unsigned int waitStatus(const unsigned int &timeout,
                              const t_drmStatusRegisterEnumValues &statusBitPosition,
                              const t_drmStatusRegisterMaskEnumValues &statusMask,
                              const bool &expectedStatus,
                              bool &status) const;

      /** waitMeteringReadyStatus
      *   \brief Wait status metering ready to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
      **/
      unsigned int waitMeteringReadyStatus(const unsigned int &timeout,
                                           const bool &expectedStatus,
                                           bool &status) const;

      /** waitSaasChallengeReadyStatus
      *   \brief Wait status saas challenge ready to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
      **/
      unsigned int waitSaasChallengeReadyStatus(const unsigned int &timeout,
                                                const bool &expectedStatus,
                                                bool &status) const;

      /** waitEndSessionMeteringReadyStatus
      *   \brief Wait status end session metering ready to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[out] status is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout error occured.
      **/
      unsigned int waitEndSessionMeteringReadyStatus(const unsigned int &timeout,
                                                     const bool &expectedStatus,
                                                     bool &status) const;

      /** readStatusAndErrorRegisters
      *   \brief Read values of status and eror registers.
      *   This method will access to the system bus to read status and error registers.
      *   \param[in]  statusBitPosition is the position of the status bit.
      *   \param[in]  statusMask is the mask of the status bit.
      *   \param[in]  statusToReadError is the value of the status to enable the error read.
      *   \param[in]  errorPosition is the position of the error byte.
      *   \param[in]  errorMask is the mask of the error byte.
      *   \param[out] status is the value of the status bit read.
      *   \param[out] error is the value of the error byte read.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      unsigned int readStatusAndErrorRegisters(const t_drmStatusRegisterEnumValues &statusBitPosition,
                                               const t_drmStatusRegisterMaskEnumValues &statusMask,
                                               const bool &statusToReadError,
                                               const t_drmErrorRegisterBytePositionEnumValues &errorPosition,
                                               const t_drmErrorRegisterByteMaskEnumValues &errorMask,
                                               bool &status,
                                               unsigned char &error) const;

      /** convertRegisterListToStringList
      *   \brief Convert a list of register value into a list of strings.
      *   \param[in] registerList is the list of register values.
      *   \param[in] numberOfRegisterPerString is the number of registers to push per string.
      *   \return Returns the list of strings generated.
            **/
      std::vector<std::string> convertRegisterListToStringList(const std::vector<unsigned int> &registerList,
                                                               const unsigned int &numberOfRegisterPerString) const;

      /** checkLicenseFileSize
      *   \brief Verify that the size of the license file is correct.
      *   \param[in] licenseFile is the license file.
      *   \return Returns true if license file size is correct, false otherwize.
      *   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what()
      *          should be called to get the exception description.
      **/
      bool checkLicenseFileSize(const std::vector<unsigned int> &licenseFile) const;

      /** displayPage
      *   \brief Display the value of the page register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayPage(std::ostream &file = std::cout) const;

      /** displayCommand
      *   \brief Display the value of the command register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayCommand(std::ostream &file = std::cout) const;

      /** displayLicenseStartAddress
      *   \brief Display the value of the license start address register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayLicenseStartAddress(std::ostream &file = std::cout) const;

      /** displayLicenseTimer
      *   \brief Display the value of the license timer register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayLicenseTimer(std::ostream &file = std::cout) const;

      /** displayStatus
      *   \brief Display the value of the status register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayStatus(std::ostream &file = std::cout) const;

      /** displayError
      *   \brief Display the value of the error register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayError(std::ostream &file = std::cout) const;

      /** displayDrmVersion
      *   \brief Display the value of the drm version.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayDrmVersion(std::ostream &file = std::cout) const;

      /** displayDna
      *   \brief Display the value of the dna.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayDna(std::ostream &file = std::cout) const;

      /** displaySaasChallenge
      *   \brief Display the value of the saas challenge.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displaySaasChallenge(std::ostream &file = std::cout) const;

      /** displayLogs
      *   \brief Display the value of the logs register.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayLogs(std::ostream &file = std::cout) const;

      /** displayVlnvFile
      *   \brief Display the value of the vlnv file.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayVlnvFile(std::ostream &file = std::cout) const;

      /** displayLicenseFile
      *   \brief Display the license file.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayLicenseFile(std::ostream &file = std::cout) const;

      /** displayTraceFile
      *   \brief Display the trace file.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayTraceFile(std::ostream &file = std::cout) const;

      /** displayMeteringFile
      *   \brief Display the value of the metering file.
      *   \param[in] file is the stream to use for the data display.
      *   \return Returns the error code produced by the read/write register function.
      **/
      unsigned int displayMeteringFile(std::ostream &file = std::cout) const;

      /** printHeaderHwReport
      *   \brief Print the header of the hardware report to output stream.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printHeaderHwReport(std::ostream &outputStream = std::cout) const;

      /** printComponentNameHwReport
      *   \brief Print the component name of the hardware report to output stream.
      *   \param[in] componentName is the name of the component to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printComponentNameHwReport(const std::string &componentName,
                                      std::ostream &outputStream = std::cout) const;

      /** printStringNameHwReport
      *   \brief Print the string to output stream.
      *   \param[in] name is the name to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printStringNameHwReport(const std::string &name,
                                   std::ostream &outputStream = std::cout) const;

      /** printRegisterNameHwReport
      *   \brief Print the register name of the hardware report to output stream.
      *   \param[in] registerName is the name of the register to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printRegisterNameHwReport(const std::string &registerName,
                                      std::ostream &outputStream = std::cout) const;

      /** printVersionNameHwReport
      *   \brief Print the version name of the hardware report to output stream.
      *   \param[in] versionName is the name of the version to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printVersionNameHwReport(const std::string &versionName,
                                    std::ostream &outputStream = std::cout) const;

      /** printFileNameHwReport
      *   \brief Print the file name of the hardware report to output stream.
      *   \param[in] fileName is the name of the file to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printFileNameHwReport(const std::string &fileName,
                                 std::ostream &outputStream = std::cout) const;

      /** printStringValueHwReport
      *   \brief Print the value of the string register of the hardware report to output stream.
      *   \param[in] value is the string value to write.
      *   \param[in] desc is the description to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printStringValueHwReport(const std::string &value,
                                    const std::string &desc,
                                    std::ostream &outputStream = std::cout) const;

      /** printStringRegisterValueHwReport
      *   \brief Print the value of the string register of the hardware report to output stream.
      *   \param[in] registerValue is the string value of the register to write.
      *   \param[in] registerDesc is the description of the register to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printStringRegisterValueHwReport(const std::string &registerValue,
                                            const std::string &registerDesc,
                                            std::ostream &outputStream = std::cout) const;

      /** printIntRegisterValueHwReport
      *   \brief Print the value of the integer register of the hardware report to output stream.
      *   \param[in] registerValue is the int value of the register to write.
      *   \param[in] registerDesc is the description of the register to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printIntRegisterValueHwReport(const unsigned int &registerValue,
                                         const std::string &registerDesc,
                                         std::ostream &outputStream = std::cout) const;

      /** printBitRegisterValueHwReport
      *   \brief Print the value of the bit register of the hardware report to output stream.
      *   \param[in] registerValue is the bit value of the register to write.
      *   \param[in] registerDesc is the description of the register to write.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printBitRegisterValueHwReport(const bool &registerValue,
                                         const std::string &registerDesc,
                                         std::ostream &outputStream = std::cout) const;

      /** printWordRegisterValueHwReport
      *   \brief Print the value of the N bits word register of the hardware report to output stream.
      *   \param[in] registerValue is the N bits word value of the register to write.
      *   \param[in] registerDesc is the description of the register to write.
      *   \param[in] numberOfNibble is the number of nibble to display.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printWordRegisterValueHwReport(const unsigned int &registerValue,
                                          const std::string &registerDesc,
                                          const unsigned int &numberOfNibble,
                                          std::ostream &outputStream = std::cout) const;

      /** printStringArrayRegisterValueHwReport
      *   \brief Print the value of the array of string register of the hardware report to output stream.
      *   \param[in] stringArrayValue is the array of string value of the register to write.
      *   \param[in] stringDesc is the description of the register to write.
      *   \param[in] startIndex is the value of index of the first word to display.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printStringArrayRegisterValueHwReport(const std::vector<std::string> &stringArrayValue,
                                                 const std::string &stringDesc,
                                                 const int &startIndex,
                                                 std::ostream &outputStream = std::cout) const;

      /** printStringArrayRegisterValueHwReport
      *   \brief Print the value of the array of string register of the hardware report to output stream.
      *   \param[in] stringValue is the string value of the register to write.
      *   \param[in] stringDesc is the description of the register to write.
      *   \param[in] stringWordSize is the size of a word.
      *   \param[in] startIndex is the value of index of the first word to display.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printStringArrayRegisterValueHwReport(const std::string &stringValue,
                                                 const std::string &stringDesc,
                                                 const unsigned int &stringWordSize,
                                                 const int &startIndex,
                                                 std::ostream &outputStream = std::cout) const;

      /** printFooterHwReport
      *   \brief Print the footer of the hardware report to output stream.
      *   \param[inout] outputStream is the stream to write to. Set to std::cout by default.
      **/
      void printFooterHwReport(std::ostream &outputStream = std::cout) const;

      /** operator<<
      *   \brief Declaration of friend function for output stream operator.
      *   \param[in] file is the stream to use for the data display.
      *   \param[in] DrmControllerOperations is a reference to this object.
      *   \return Returns the output stream used
      **/
      friend std::ostream& operator<<(std::ostream &file, const DrmControllerOperations &DrmControllerOperations);

  }; // class DrmControllerOperations

  /** operator<<
  *   \brief Declaration of function for output stream operator.
  *   \param[in] file is the stream to use for the data display.
  *   \param[in] DrmControllerOperations is a reference to this object.
  *   \return Returns the output stream used
  **/
  std::ostream& operator<<(std::ostream &file, const DrmControllerOperations &DrmControllerOperations);

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_OPERATIONS_HPP__
