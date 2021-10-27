/**
*  \file      DrmControllerRegistersStrategyInterface.hpp
*  \version   7.0.0.0
*  \date      October 2021
*  \brief     Class DrmControllerRegistersStrategyInterface defines strategy interface for register access.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_REGISTERS_STRATEGY_INTERFACE_HPP__
#define __DRM_CONTROLLER_REGISTERS_STRATEGY_INTERFACE_HPP__

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <DrmControllerCommon.hpp>
#include <DrmControllerVersion.hpp>
#include <HAL/DrmControllerRegistersBase.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerRegistersStrategyInterface DrmControllerRegistersStrategyInterface.hpp "include/HAL/DrmControllerRegistersStrategyInterface.hpp"
  *   \brief    Class DrmControllerRegistersStrategyInterface defines strategy interface for register access.
  **/
  class DrmControllerRegistersStrategyInterface: public DrmControllerRegistersBase {

    // public members, functions ...
    public:

      /** DrmControllerRegistersStrategyInterface
      *   \brief Class constructor.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerRegistersStrategyInterface(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction);

      /** ~DrmControllerRegistersStrategyInterface
      *   \brief Class destructor.
      **/
      virtual ~DrmControllerRegistersStrategyInterface();

      /** writeRegistersPageRegister
      *   \brief Write the page register to select the registers page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeRegistersPageRegister() const = 0;

      /** writeVlnvFilePageRegister
      *   \brief Write the page register to select the vlnv file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeVlnvFilePageRegister() const = 0;

      /** writeLicenseFilePageRegister
      *   \brief Write the page register to select the license file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseFilePageRegister() const = 0;

      /** writeTraceFilePageRegister
      *   \brief Write the page register to select the trace file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeTraceFilePageRegister() const = 0;

      /** writeMeteringFilePageRegister
      *   \brief Write the page register to select the metering file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMeteringFilePageRegister() const = 0;

      /** writeMailBoxFilePageRegister
      *   \brief Write the page register to select the mailbox file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMailBoxFilePageRegister() const = 0;

      /** writeNopCommandRegister
      *   \brief Write the command register to the NOP Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeNopCommandRegister() const = 0;

      /** writeDnaExtractCommandRegister
      *   \brief Write the command register to the DNA Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeDnaExtractCommandRegister() const = 0;

      /** writeVlnvExtractCommandRegister
      *   \brief Write the command register to the VLNV Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeVlnvExtractCommandRegister() const = 0;

      /** writeActivateCommandRegister
      *   \brief Write the command register to the Activate Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeActivateCommandRegister() const = 0;

      /** writeEndSessionMeteringExtractCommandRegister
      *   \brief Write the command register to the end session extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeEndSessionMeteringExtractCommandRegister() const = 0;

      /** writeMeteringExtractCommandRegister
      *   \brief Write the command register to the extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMeteringExtractCommandRegister() const = 0;

      /** writeSampleLicenseTimerCounterCommandRegister
      *   \brief Write the command register to the sample license timer counter Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeSampleLicenseTimerCounterCommandRegister() const = 0;

      /** writeLicenseTimerInitSemaphoreRequestCommandRegister
      *   \brief Write the LicenseTimerInitSemaphoreRequest bit in the command register to the given value (do not modify the other bits of the command register).
      *   This method will access to the system bus to write into the command register.
      *   \param[in] licenseTimerInitSemaphoreRequest is the value of the LicenseTimerInitSemaphoreRequest bit to write in the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseTimerInitSemaphoreRequestCommandRegister(bool &licenseTimerInitSemaphoreRequest) const = 0;

      /** readLicenseStartAddressRegister
      *   \brief Read the license start address register.
      *   This method will access to the system bus to read the license start address.
      *   \param[out] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseStartAddressRegister(std::vector<unsigned int> &licenseStartAddress) const = 0;

      /** readLicenseStartAddressRegister
      *   \brief Read the value of the license start address.
      *   This method will access to the system bus to read the license start address register.
      *   \param[in] licenseStartAddress is the license start address value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseStartAddressRegister(std::string &licenseStartAddress) const;

      /** writeLicenseStartAddressRegister
      *   \brief Write the license start address register.
      *   This method will access to the system bus to write the license start address.
      *   \param[in] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseStartAddressRegister(const std::vector<unsigned int> &licenseStartAddress) const = 0;

      /** writeLicenseStartAddressRegister
      *   \brief Write the value of the license start address.
      *   This method will access to the system bus to write the license start address register.
      *   \param[in] licenseStartAddress is the license start address value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeLicenseStartAddressRegister(const std::string &licenseStartAddress) const;

      /** readLicenseTimerInitRegister
      *   \brief Read the license timer register.
      *   This method will access to the system bus to read the license timer.
      *   \param[out] licenseTimerInit is a list of binary values for the license timer register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerInitRegister(std::vector<unsigned int> &licenseTimerInit) const = 0;

      /** readLicenseTimerInitRegister
      *   \brief Read the value of the license timer.
      *   This method will access to the system bus to read the license timer register.
      *   \param[out] licenseTimerInit is the license timer value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseTimerInitRegister(std::string &licenseTimerInit) const;

      /** writeLicenseTimerInitRegister
      *   \brief Write the license start address register.
      *   This method will access to the system bus to write the license timer.
      *   \param[in] licenseTimerInit is a list of binary values for the license timer register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseTimerInitRegister(const std::vector<unsigned int> &licenseTimerInit) const = 0;

      /** writeLicenseTimerInitRegister
      *   \brief Write the value of the license timer.
      *   This method will access to the system bus to write the license timer register.
      *   \param[in] licenseTimerInit is the license timer value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeLicenseTimerInitRegister(const std::string &licenseTimerInit) const;

      /** readDnaReadyStatusRegister
      *   \brief Read the status register and get the dna ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] dnaReady is the value of the status bit DNA Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readDnaReadyStatusRegister(bool &dnaReady) const = 0;

      /** waitDnaReadyStatusRegister
      *   \brief Wait dna ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitDnaReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readVlnvReadyStatusRegister
      *   \brief Read the status register and get the vlnv ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] vlnvReady is the value of the status bit VLNV Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readVlnvReadyStatusRegister(bool &vlnvReady) const = 0;

      /** waitVlnvReadyStatusRegister
      *   \brief Wait vlnv ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitVlnvReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readActivationDoneStatusRegister
      *   \brief Read the status register and get the activation done status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationDone is the value of the status bit Activation Done.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readActivationDoneStatusRegister(bool &activationDone) const = 0;

      /** waitActivationDoneStatusRegister
      *   \brief Wait activation done status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitActivationDoneStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readAutonomousControllerEnabledStatusRegister
      *   \brief Read the status register and get the autonomous controller enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoEnabled is the value of the status bit autonomous controller enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAutonomousControllerEnabledStatusRegister(bool &autoEnabled) const = 0;

      /** readAutonomousControllerBusyStatusRegister
      *   \brief Read the status register and get the autonomous controller busy status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoBusy is the value of the status bit autonomous controller busy.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAutonomousControllerBusyStatusRegister(bool &autoBusy) const = 0;

      /** waitAutonomousControllerBusyStatusRegister
      *   \brief Wait autonomous controller busy status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitAutonomousControllerBusyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readMeteringEnabledStatusRegister
      *   \brief Read the status register and get the metering enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringEnabled is the value of the status bit metering enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringEnabledStatusRegister(bool &meteringEnabled) const = 0;

      /** checkMeteringEnabledStatusRegister
      *   \brief Read the status register and check the metering enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringEnabled is the value of the status bit metering enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerFunctionalityDisabledException if the metering is disabled. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
      **/
      unsigned int checkMeteringEnabledStatusRegister(bool &meteringEnabled) const;

      /** readMeteringReadyStatusRegister
      *   \brief Read the status register and get the metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringReady is the value of the status bit metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringReadyStatusRegister(bool &meteringReady) const = 0;

      /** waitMeteringReadyStatusRegister
      *   \brief Wait metering ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readSaasChallengeReadyStatusRegister
      *   \brief Read the status register and get the saas challenge ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSaasChallengeReadyStatusRegister(bool &saasChallengeReady) const = 0;

      /** waitSaasChallengeReadyStatusRegister
      *   \brief Wait saas challenge ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitSaasChallengeReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readLicenseTimerEnabledStatusRegister
      *   \brief Read the status register and get the license timer enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const = 0;

      /** checkLicenseTimerEnabledStatusRegister
      *   \brief Read the status register and check license timer enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerFunctionalityDisabledException if the metering is disabled. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
      **/
      unsigned int checkLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const;

      /** readLicenseTimerInitLoadedStatusRegister
      *   \brief Read the status register and get the license timer init loaded status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init load.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerInitLoadedStatusRegister(bool &licenseTimerInitLoaded) const = 0;

      /** waitLicenseTimerInitLoadedStatusRegister
      *   \brief Wait license timer init loaded status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerInitLoadedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readEndSessionMeteringReadyStatusRegister
      *   \brief Read the status register and get the end session metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] endSessionMeteringReady is the value of the status bit end session metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readEndSessionMeteringReadyStatusRegister(bool &endSessionMeteringReady) const = 0;

      /** waitEndSessionMeteringReadyStatusRegister
      *   \brief Wait end session metering ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitEndSessionMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readHeartBeatModeEnabledStatusRegister
      *   \brief Read the status register and get the heart beat mode enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] heartBeatModeEnabled is the value of the status bit heart beat mode enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readHeartBeatModeEnabledStatusRegister(bool &heartBeatModeEnabled) const = 0;

      /** readAsynchronousMeteringReadyStatusRegister
      *   \brief Read the status register and get the asynchronous metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] asynchronousMeteringReady is the value of the status bit asynchronous metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAsynchronousMeteringReadyStatusRegister(bool &asynchronousMeteringReady) const = 0;
    
      /** waitAsynchronousMeteringReadyStatusRegister
      *   \brief Wait asynchronous metering ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitAsynchronousMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readLicenseTimerSampleReadyStatusRegister
      *   \brief Read the status register and get the license timer sample ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerSampleReady is the value of the status bit license timer sample ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerSampleReadyStatusRegister(bool &licenseTimerSampleReady) const = 0;

      /** waitLicenseTimerSampleReadyStatusRegister
      *   \brief Wait license timer sample ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerSampleReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readLicenseTimerCountEmptyStatusRegister
      *   \brief Read the status register and get the license timer count empty status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerCounterEmpty is the value of the status bit license timer count empty.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerCountEmptyStatusRegister(bool &licenseTimerCounterEmpty) const = 0;

      /** waitLicenseTimerCountEmptyStatusRegister
      *   \brief Wait license timer count empty status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerCountEmptyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;
    
      /** readSessionRunningStatusRegister
      *   \brief Read the status register and get the session running status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] sessionRunning is the value of the status bit session running.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSessionRunningStatusRegister(bool &sessionRunning) const = 0;
    
      /** waitSessionRunningStatusRegister
      *   \brief Wait session running status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitSessionRunningStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readActivationCodesTransmittedStatusRegister
      *   \brief Read the status register and get the activation codes transmitted status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationCodeTransmitted is the value of the status bit activation codes transmitted.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readActivationCodesTransmittedStatusRegister(bool &activationCodeTransmitted) const = 0;

      /** waitActivationCodesTransmittedStatusRegister
      *   \brief Wait activation codes transmitted status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitActivationCodesTransmittedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readLicenseNodeLockStatusRegister
      *   \brief Read the status register and get the license node lock status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseNodeLock is the value of the status bit license node lock.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseNodeLockStatusRegister(bool &licenseNodeLock) const = 0;

      /** waitLicenseNodeLockStatusRegister
      *   \brief Wait license node lock status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseNodeLockStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readLicenseMeteringStatusRegister
      *   \brief Read the status register and get the license metering status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseMetering is the value of the status bit license metering.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseMeteringStatusRegister(bool &licenseMetering) const = 0;

      /** waitLicenseMeteringStatusRegister
      *   \brief Wait license metering status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseMeteringStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readSecurityAlertStatusRegister
      *   \brief Read the status register and get the Security Alert status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] securityAlert is the value of the status bit Security Alert.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSecurityAlertStatusRegister(bool &securityAlert) const = 0;

      /** waitSecurityAlertStatusRegister
      *   \brief Wait Security Alert status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitSecurityAlertStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readLicenseTimerInitSemaphoreAcknowledgeStatusRegister
      *   \brief Read the status register and get the License Timer Init Semaphore Acknowledge status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerInitSemaphoreAcknowledge is the value of the status bit License Timer Init Semaphore Acknowledge.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerInitSemaphoreAcknowledgeStatusRegister(bool &licenseTimerInitSemaphoreAcknowledge) const = 0;

      /** waitLicenseTimerInitSemaphoreAcknowledgeStatusRegister
      *   \brief Wait License Timer Init Semaphore Acknowledge status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occurred, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerInitSemaphoreAcknowledgeStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const = 0;

      /** readNumberOfLicenseTimerLoadedStatusRegister
      *   \brief Read the status register and get the number of license timer loaded.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfLicenseTimerLoaded is the number of license timer loaded retrieved from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readNumberOfLicenseTimerLoadedStatusRegister(unsigned int &numberOfLicenseTimerLoaded) const = 0;

      /** waitNumberOfLicenseTimerLoadedStatusRegister
      *   \brief Wait number of license timer loaded status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitNumberOfLicenseTimerLoadedStatusRegister(const unsigned int &timeout, const unsigned int &expected, unsigned int &actual) const = 0;

      /** readNumberOfDetectedIpsStatusRegister
      *   \brief Read the status register and get the number of detected IPs.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfDetectedIps is the number of detected ips retrieved from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readNumberOfDetectedIpsStatusRegister(unsigned int &numberOfDetectedIps) const = 0;

      /** readExtractDnaErrorRegister
      *   \brief Read the error register and get the error code related to dna extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] dnaExtractError is the error code related to dna extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readExtractDnaErrorRegister(unsigned char &dnaExtractError) const = 0;

      /** waitExtractDnaErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitExtractDnaErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const = 0;

      /** readExtractVlnvErrorRegister
      *   \brief Read the error register and get the error code related to vlnv extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] vlnvExtractError is the error code related to vlnv extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readExtractVlnvErrorRegister(unsigned char &vlnvExtractError) const = 0;

      /** waitExtractVlnvErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitExtractVlnvErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const = 0;

      /** readActivationErrorRegister
      *   \brief Read the error register and get the error code related to activation.
      *   This method will access to the system bus to read the error register.
      *   \param[out] activationError is the error code related to activation.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readActivationErrorRegister(unsigned char &activationError) const = 0;

      /** waitActivationErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitActivationErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const = 0;

      /** readLicenseTimerLoadErrorRegister
      *   \brief Read the error register and get the error code related to license timer loading.
      *   This method will access to the system bus to read the error register.
      *   \param[out] licenseTimerLoadError is the error code related to license timer loading.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerLoadErrorRegister(unsigned char &licenseTimerLoadError) const = 0;

      /** waitActivationErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerLoadErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const = 0;

      /** readDnaRegister
      *   \brief Read the dna register and get the value.
      *   This method will access to the system bus to read the dna register.
      *   \param[out] dna is the dna value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readDnaRegister(std::vector<unsigned int> &dna) const = 0;

      /** readDnaRegister
      *   \brief Read the dna register and get the value.
      *   This method will access to the system bus to read the dna register.
      *   \param[out] dna is the dna value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readDnaRegister(std::string &dna) const;

      /** readSaasChallengeRegister
      *   \brief Read the Saas Challenge register and get the value.
      *   This method will access to the system bus to read the Saas Challenge register.
      *   \param[out] saasChallenge is the saas challenge value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSaasChallengeRegister(std::vector<unsigned int> &saasChallenge) const = 0;

      /** readSaasChallengeRegister
      *   \brief Read the saas challenge register and get the value.
      *   This method will access to the system bus to read the saas challenge register.
      *   \param[out] saasChallenge is the saas challenge value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readSaasChallengeRegister(std::string &saasChallenge) const;

      /** readLicenseTimerCounterRegister
      *   \brief Read the License Timer Counter register and get the value.
      *   This method will access to the system bus to read the License Timer Counter register.
      *   \param[out] licenseTimerCounter is the License Timer Counter value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerCounterRegister(std::vector<unsigned int> &licenseTimerCounter) const = 0;

      /** readLicenseTimerCounterRegister
      *   \brief Read the license timer counter register and get the value.
      *   This method will access to the system bus to read the license timer counter register.
      *   \param[out] licenseTimerCounter is the license timer counter value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseTimerCounterRegister(std::string &licenseTimerCounter) const;

      /** readDrmVersionRegister
      *   \brief Read the drm version register and get the value.
      *   This method will access to the system bus to read the drm version register.
      *   \param[out] drmVersion is the drm version value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readDrmVersionRegister(unsigned int &drmVersion) const = 0;

      /** readDrmVersionRegister
      *   \brief Read the drm version register and get the value.
      *   This method will access to the system bus to read the drm version register.
      *   \param[out] drmVersion is the drm version value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readDrmVersionRegister(std::string &drmVersion) const;
      
      /** readAdaptiveProportionTestFailuresRegister
      *   \brief Read the Adaptive Proportion Test Failures register and get the value.
      *   This method will access to the system bus to read the Adaptive Proportion Test Failures register.
      *   \param[out] adaptiveProportionTestFailures is the Adaptive Proportion Test Failures value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAdaptiveProportionTestFailuresRegister(std::vector<unsigned int> &adaptiveProportionTestFailures) const = 0;

      /** readAdaptiveProportionTestFailuresRegister
      *   \brief Read the Adaptive Proportion Test Failures register and get the value.
      *   This method will access to the system bus to read the Adaptive Proportion Test Failures register.
      *   \param[out] adaptiveProportionTestFailures is the Adaptive Proportion Test Failures value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readAdaptiveProportionTestFailuresRegister(std::string &adaptiveProportionTestFailures) const;

      /** readRepetitionCountTestFailuresRegister
      *   \brief Read the Repetition Count Test Failures register and get the value.
      *   This method will access to the system bus to read the Repetition Count Test Failures register.
      *   \param[out] repetitionCountTestFailures is the Repetition Count Test Failures value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readRepetitionCountTestFailuresRegister(std::vector<unsigned int> &repetitionCountTestFailures) const = 0;

      /** readRepetitionCountTestFailuresRegister
      *   \brief Read the Repetition Count Test Failures register and get the value.
      *   This method will access to the system bus to read the Repetition Count Test Failures register.
      *   \param[out] repetitionCountTestFailures is the Repetition Count Test Failures value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readRepetitionCountTestFailuresRegister(std::string &repetitionCountTestFailures) const;

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLogsRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &logs) const = 0;

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLogsRegister(const unsigned int &numberOfIPs, std::string &logs) const;

      /** readVlnvFileRegister
      *   \brief Read the vlnv file and get the value.
      *   This method will access to the system bus to read the vlnv file.
      *   The vlnv file will contains numberOfIps+1 words. The first one
      *   is dedicated to the drm controller, the others correspond for
      *   the IPs connected to the drm controller.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] vlnvFile is the vlnv file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readVlnvFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &vlnvFile) const = 0;

      /** readVlnvFileRegister
      *   \brief Read the vlnv file and get the value.
      *   This method will access to the system bus to read the vlnv file.
      *   The vlnv file will contains numberOfIps+1 elements. The first one
      *   is dedicated to the drm controller, the others correspond for
      *   the IPs connected to the drm controller.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] vlnvFile is the vlnv file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readVlnvFileRegister(const unsigned int &numberOfIPs, std::vector<std::string> &vlnvFile) const = 0;

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize, std::vector<unsigned int> &licenseFile) const = 0;

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   The license file is a string using a hexadecimal representation.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize, std::string &licenseFile) const = 0;

      /** writeLicenseFile
      *   \brief Write the license file.
      *   This method will access to the system bus to write the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[in] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseFileRegister(const unsigned int &licenseFileSize, const std::vector<unsigned int> &licenseFile) const = 0;

      /** writeLicenseFileRegister
      *   \brief Write the license file.
      *   This method will access to the system bus to write the license file.
      *   The license file is a string using a hexadecimal representation.
      *   \param[in] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error,
      *           mDrmApi_LICENSE_FILE_SIZE_ERROR if the license file size is lower than the minimum required,
      *           or the error code produced by the read/write register function.
      *   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what()
      *          should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseFileRegister(const std::string &licenseFile) const = 0;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readTraceFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &traceFile) const = 0;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readTraceFileRegister(const unsigned int &numberOfIPs, std::vector<std::string> &traceFile) const = 0;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &meteringFile) const = 0;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringFileRegister(const unsigned int &numberOfIPs, std::vector<std::string> &meteringFile) const = 0;

      /** readMailboxFileSizeRegister
      *   \brief Read the mailbox file word numbers.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMailboxFileSizeRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber) const = 0;

      /** readMailboxFileRegister
      *   \brief Read the mailbox file.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \param[out] readOnlyMailboxData is the data read from the read-only mailbox.
      *   \param[out] readWriteMailboxData is the data read from the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                                   std::vector<unsigned int> &readOnlyMailboxData, std::vector<unsigned int> &readWriteMailboxData) const = 0;

      /** readMailboxFileRegister
      *   \brief Read the mailbox file.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \param[out] readOnlyMailboxData is the data read from the read-only mailbox.
      *   \param[out] readWriteMailboxData is the data read from the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                                   std::vector<std::string> &readOnlyMailboxData, std::vector<std::string> &readWriteMailboxData) const = 0;

      /** writeMailboxFileRegister
      *   \brief Write the mailbox file.
      *   This method will access to the system bus to write the mailbox file.
      *   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMailboxFileRegister(const std::vector <unsigned int> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const = 0;

      /** writeMailboxFileRegister
      *   \brief Write the mailbox file.
      *   This method will access to the system bus to write the mailbox file.
      *   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMailboxFileRegister(const std::vector<std::string> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const = 0;

      /** printHwReport
      * \brief Print all register content accessible through AXI-4 Lite Control channel.
      * \param[in] file: Reference to output file where register contents are saved. By default print on standard output
      */
      void printHwReport(std::ostream &file = std::cout) const;

      /** printMeteringFile
      *   \brief Display the value of the metering file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printMeteringFileHwReport(std::ostream &file) const = 0;

      /** getDrmErrorRegisterMessage
      *   \brief Get the error message from the error register value.
      *   \param[in] errorRegister is the value of the error register.
      *   \return Returns the error message.
      **/
      virtual const char* getDrmErrorRegisterMessage(const unsigned char &errorRegister) const = 0;

    // protected members, functions ...
    protected:

      /** readModifyWriteCommandRegister
      *   \brief Read, modify then write the command register to the given Command value. The value of the licenseTimerInitSemaphoreRequest bit is not modified
      *   This method will access to the system bus to read and write into the command register.
      *   \param[in] commandValue is the value of the command to write
      *   \param[in] licenseTimerInitSemaphoreRequestMask is the mask of the licenseTimerInitSemaphoreRequest bit which must be kept.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readModifyWriteCommandRegister(const unsigned int &commandValue, const unsigned int &licenseTimerInitSemaphoreRequestMask) const;

      /** readPageRegister
      *   \brief Read and get the value of the page register from the hardware.
      *   This method will access to the system bus to read the page register.
      *   \param[out] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readPageRegister(unsigned int &page) const = 0;

      /** writePageRegister
      *   \brief Write the value of the page register into the hardware.
      *   This method will access to the system bus to write the page register.
      *   \param[in] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writePageRegister(const unsigned int &page) const = 0;

      /** writeCommandRegister
      *   \brief Write the value of the command register into the hardware.
      *   This method will access to the system bus to write the command register.
      *   \param[in] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeCommandRegister(const unsigned int &command) const = 0;

      /** readCommandRegister
      *   \brief Read and get the value of the command register from the hardware.
      *   This method will access to the system bus to read the command register.
      *   \param[out] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readCommandRegister(unsigned int &command) const = 0;

      /** readStatusRegister
      *   \brief Read the status register.
      *   This method will access to the system bus to read the status register.
      *   \param[out] status is the binary value of the status register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readStatusRegister(unsigned int &status) const = 0;

      /** readStatusRegister
      *   \brief Read the value of a specific status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[in] bitPosition is the position of the status bit.
      *   \param[in] mask is the mask of the status bit.
      *   \param[out] value is the value of the status bit.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readStatusRegister(const unsigned int &bitPosition, const unsigned int &mask, bool &value) const;

      /** readStatusRegister
      *   \brief Read the value of a several and contigous status bits.
      *   This method will access to the system bus to read the status register.
      *   \param[in] bitPosition is the lsb position of the status bits.
      *   \param[in] mask is the mask of the status bits.
      *   \param[out] value is the value of the status bit.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readStatusRegister(const unsigned int &bitPosition, const unsigned int &mask, unsigned int &value) const;

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
      unsigned int waitStatusRegister(const unsigned int &timeout, const unsigned int &bitPosition, const unsigned int &mask, const bool &expected, bool &actual) const;

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
      unsigned int waitStatusRegister(const unsigned int &timeout, const unsigned int &bitPosition, const unsigned int &mask, const unsigned int &expected, unsigned int &actual) const;

      /** readErrorRegister
      *   \brief Read the error register.
      *   This method will access to the system bus to read the error register.
      *   \param[out] error is the binary value of the error register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readErrorRegister(unsigned int &error) const = 0;

      /** readErrorRegister
      *   \brief Get the value of a specific error byte.
      *   This method will access to the system bus to read the error register.
      *   \param[in] position is the position of the error byte.
      *   \param[in] mask is the mask of the error byte.
      *   \param[out] error is the value of the error byte.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readErrorRegister(const unsigned int &position, const unsigned int &mask, unsigned char &error) const;

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
      unsigned int waitErrorRegister(const unsigned int &timeout, const unsigned int &position, const unsigned int &mask, const unsigned char &expected, unsigned char &actual) const;

      
    // private members, functions ...
    private:

      unsigned int mSleepInMicroSeconds; /**<Store the sleep in microseconds to complete internal operation.**/

      /** printPage
      *   \brief Display the value of the page register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printPageHwReport(std::ostream &file) const = 0;

      /** printCommand
      *   \brief Display the value of the command register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printCommandHwReport(std::ostream &file) const = 0;

      /** printLicenseStartAddress
      *   \brief Display the value of the license start address register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseStartAddressHwReport(std::ostream &file) const = 0;

      /** printLicenseTimer
      *   \brief Display the value of the license timer register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseTimerHwReport(std::ostream &file) const = 0;

      /** printStatus
      *   \brief Display the value of the status register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printStatusHwReport(std::ostream &file) const = 0;

      /** printError
      *   \brief Display the value of the error register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printErrorHwReport(std::ostream &file) const = 0;

      /** printDrmVersion
      *   \brief Display the value of the drm version.
      **/
      virtual void printDrmVersionHwReport(std::ostream &file) const = 0;

      /** printDna
      *   \brief Display the value of the dna.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printDnaHwReport(std::ostream &file) const = 0;

      /** printSaasChallenge
      *   \brief Display the value of the saas challenge.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printSaasChallengeHwReport(std::ostream &file) const = 0;

      /** printLicenseTimerCounter
      *   \brief Display the value of the license timer counter.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseTimerCounterHwReport(std::ostream &file) const = 0;

      /** printLogs
      *   \brief Display the value of the logs register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLogsHwReport(std::ostream &file) const = 0;

      /** printVlnvFile
      *   \brief Display the value of the vlnv file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printVlnvFileHwReport(std::ostream &file) const = 0;

      /** printLicenseFile
      *   \brief Display the license file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseFileHwReport(std::ostream &file) const = 0;

      /** printTraceFile
      *   \brief Display the trace file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printTraceFileHwReport(std::ostream &file) const = 0;

      /** printMailBoxFile
      *   \brief Display the value of the mailbox file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printMailBoxFileHwReport(std::ostream &file) const = 0;
      
      /** printAdaptiveProportionTestFailures
      *   \brief Display the value of the Adaptive Proportion Test Failures.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printAdaptiveProportionTestFailuresHwReport(std::ostream &file) const = 0;
      
      /** printRepetitionCountTestFailuresHwReport;
      *   \brief Display the value of the Repetition Count Test Failures.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printRepetitionCountTestFailuresHwReport(std::ostream &file) const = 0;
      
  }; // class DrmControllerRegistersStrategyInterface

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_REGISTERS_STRATEGY_INTERFACE_HPP__
