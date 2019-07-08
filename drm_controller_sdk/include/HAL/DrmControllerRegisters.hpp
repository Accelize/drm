/**
*  \file      DrmControllerRegisters.hpp
*  \version   3.2.2.0
*  \date      May 2019
*  \brief     Class DrmControllerRegisters defines low level procedures
*             for access to all registers.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_REGISTERS_HPP__
#define __DRM_CONTROLLER_REGISTERS_HPP__

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>

#include <DrmControllerCommon.hpp>
#include <DrmControllerVersion.hpp>
#include <HAL/DrmControllerRegistersStrategyInterface.hpp>
#include <HAL/DrmControllerRegistersStrategy_v3_0_0.hpp>
#include <HAL/DrmControllerRegistersStrategy_v3_1_0.hpp>
#include <HAL/DrmControllerRegistersStrategy_v3_2_0.hpp>
#include <HAL/DrmControllerRegistersStrategy_v3_2_1.hpp>
#include <HAL/DrmControllerRegistersStrategy_v3_2_2.hpp>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerRegisters DrmControllerRegisters.hpp "include/DrmControllerRegisters.hpp"
  *   \brief    Class DrmControllerRegisters defines low level procedures
  **/
  class DrmControllerRegisters {

    // public members, functions ...
    public:

      /** DrmControllerRegisters
      *   \brief Class constructor.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerRegisters(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction);

      /** ~DrmControllerRegisters
      *   \brief Class destructor.
      **/
      ~DrmControllerRegisters();

      /** writeRegistersPageRegister
      *   \brief Write the page register to select the registers page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeRegistersPageRegister() const;

      /** writeVlnvFilePageRegister
      *   \brief Write the page register to select the vlnv file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeVlnvFilePageRegister() const;

      /** writeLicenseFilePageRegister
      *   \brief Write the page register to select the license file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeLicenseFilePageRegister() const;

      /** writeTraceFilePageRegister
      *   \brief Write the page register to select the trace file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeTraceFilePageRegister() const;

      /** writeMeteringFilePageRegister
      *   \brief Write the page register to select the metering file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeMeteringFilePageRegister() const;

      /** writeMailBoxFilePageRegister
      *   \brief Write the page register to select the mailbox file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeMailBoxFilePageRegister() const;

      /** writeNopCommandRegister
      *   \brief Write the command register to the NOP Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeNopCommandRegister() const;

      /** writeDnaExtractCommandRegister
      *   \brief Write the command register to the DNA Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeDnaExtractCommandRegister() const;

      /** writeVlnvExtractCommandRegister
      *   \brief Write the command register to the VLNV Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeVlnvExtractCommandRegister() const;

      /** writeActivateCommandRegister
      *   \brief Write the command register to the Activate Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeActivateCommandRegister() const;

      /** writeEndSessionMeteringExtractCommandRegister
      *   \brief Write the command register to the end session extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeEndSessionMeteringExtractCommandRegister() const;

      /** writeMeteringExtractCommandRegister
      *   \brief Write the command register to the extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeMeteringExtractCommandRegister() const;

      /** writeSampleLicenseTimerCounterCommandRegister
      *   \brief Write the command register to the sample license timer counter Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeSampleLicenseTimerCounterCommandRegister() const;

      /** readLicenseStartAddressRegister
      *   \brief Read the license start address register.
      *   This method will access to the system bus to read the license start address.
      *   \param[out] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseStartAddressRegister(std::vector<unsigned int> &licenseStartAddress) const;

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
      unsigned int writeLicenseStartAddressRegister(const std::vector<unsigned int> &licenseStartAddress) const;

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
      unsigned int readLicenseTimerInitRegister(std::vector<unsigned int> &licenseTimerInit) const;

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
      unsigned int writeLicenseTimerInitRegister(const std::vector<unsigned int> &licenseTimerInit) const;

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
      unsigned int readDnaReadyStatusRegister(bool &dnaReady) const;

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
      unsigned int waitDnaReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readVlnvReadyStatusRegister
      *   \brief Read the status register and get the vlnv ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] vlnvReady is the value of the status bit VLNV Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readVlnvReadyStatusRegister(bool &vlnvReady) const;

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
      unsigned int waitVlnvReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readActivationDoneStatusRegister
      *   \brief Read the status register and get the activation done status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationDone is the value of the status bit Activation Done.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readActivationDoneStatusRegister(bool &activationDone) const;

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
      unsigned int waitActivationDoneStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readAutonomousControllerEnabledStatusRegister
      *   \brief Read the status register and get the autonomous controller enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoEnabled is the value of the status bit autonomous controller enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readAutonomousControllerEnabledStatusRegister(bool &autoEnabled) const;

      /** readAutonomousControllerBusyStatusRegister
      *   \brief Read the status register and get the autonomous controller busy status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoBusy is the value of the status bit autonomous controller busy.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readAutonomousControllerBusyStatusRegister(bool &autoBusy) const;

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
      unsigned int waitAutonomousControllerBusyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readMeteringEnabledStatusRegister
      *   \brief Read the status register and get the metering enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringEnabled is the value of the status bit metering enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readMeteringEnabledStatusRegister(bool &meteringEnabled) const;

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
      unsigned int readMeteringReadyStatusRegister(bool &meteringReady) const;

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
      unsigned int waitMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readSaasChallengeReadyStatusRegister
      *   \brief Read the status register and get the saas challenge ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readSaasChallengeReadyStatusRegister(bool &saasChallengeReady) const;

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
      unsigned int waitSaasChallengeReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerEnabledStatusRegister
      *   \brief Read the status register and get the license timer enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const;

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
      unsigned int readLicenseTimerInitLoadedStatusRegister(bool &licenseTimerInitLoaded) const;

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
      unsigned int waitLicenseTimerInitLoadedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readEndSessionMeteringReadyStatusRegister
      *   \brief Read the status register and get the end session metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] endSessionMeteringReady is the value of the status bit end session metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readEndSessionMeteringReadyStatusRegister(bool &endSessionMeteringReady) const;

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
      unsigned int waitEndSessionMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readHeartBeatModeEnabledStatusRegister
      *   \brief Read the status register and get the heart beat mode enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] heartBeatModeEnabled is the value of the status bit heart beat mode enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readHeartBeatModeEnabledStatusRegister(bool &heartBeatModeEnabled) const;

      /** readAsynchronousMeteringReadyStatusRegister
      *   \brief Read the status register and get the asynchronous metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] asynchronousMeteringReady is the value of the status bit asynchronous metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readAsynchronousMeteringReadyStatusRegister(bool &asynchronousMeteringReady) const;
    
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
      unsigned int waitAsynchronousMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerSampleReadyStatusRegister
      *   \brief Read the status register and get the license timer sample ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerSampleReady is the value of the status bit license timer sample ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseTimerSampleReadyStatusRegister(bool &licenseTimerSampleReady) const;

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
      unsigned int waitLicenseTimerSampleReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerCountEmptyStatusRegister
      *   \brief Read the status register and get the license timer count empty status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerCounterEmpty is the value of the status bit license timer count empty.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseTimerCountEmptyStatusRegister(bool &licenseTimerCounterEmpty) const;

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
      unsigned int waitLicenseTimerCountEmptyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;
    
      /** readSessionRunningStatusRegister
      *   \brief Read the status register and get the session running status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] sessionRunning is the value of the status bit session running.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readSessionRunningStatusRegister(bool &sessionRunning) const;
    
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
      unsigned int waitSessionRunningStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readActivationCodesTransmittedStatusRegister
      *   \brief Read the status register and get the activation codes transmitted status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationCodeTransmitted is the value of the status bit activation codes transmitted.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readActivationCodesTransmittedStatusRegister(bool &activationCodeTransmitted) const;

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
      unsigned int waitActivationCodesTransmittedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseNodeLockStatusRegister
      *   \brief Read the status register and get the license node lock status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseNodeLock is the value of the status bit license node lock.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseNodeLockStatusRegister(bool &licenseNodeLock) const;

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
      unsigned int waitLicenseNodeLockStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseMeteringStatusRegister
      *   \brief Read the status register and get the license metering status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseMetering is the value of the status bit license metering.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseMeteringStatusRegister(bool &licenseMetering) const;

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
      unsigned int waitLicenseMeteringStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readNumberOfLicenseTimerLoadedStatusRegister
      *   \brief Read the status register and get the number of license timer loaded.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfLicenseTimerLoaded is the number of license timer loaded retrieved from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readNumberOfLicenseTimerLoadedStatusRegister(unsigned int &numberOfLicenseTimerLoaded) const;

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
      unsigned int waitNumberOfLicenseTimerLoadedStatusRegister(const unsigned int &timeout, const unsigned int &expected, unsigned int &actual) const;

      /** readNumberOfDetectedIpsStatusRegister
      *   \brief Read the status register and get the number of detected IPs.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfDetectedIps is the number of detected ips retrieved from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readNumberOfDetectedIpsStatusRegister(unsigned int &numberOfDetectedIps) const;

      /** readExtractDnaErrorRegister
      *   \brief Read the error register and get the error code related to dna extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] dnaExtractError is the error code related to dna extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readExtractDnaErrorRegister(unsigned char &dnaExtractError) const;

      /** waitExtractDnaErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      unsigned int waitExtractDnaErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readExtractVlnvErrorRegister
      *   \brief Read the error register and get the error code related to vlnv extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] vlnvExtractError is the error code related to vlnv extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readExtractVlnvErrorRegister(unsigned char &vlnvExtractError) const;

      /** waitExtractVlnvErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      unsigned int waitExtractVlnvErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readActivationErrorRegister
      *   \brief Read the error register and get the error code related to activation.
      *   This method will access to the system bus to read the error register.
      *   \param[out] activationError is the error code related to activation.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readActivationErrorRegister(unsigned char &activationError) const;

      /** waitActivationErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      unsigned int waitActivationErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readLicenseTimerLoadErrorRegister
      *   \brief Read the error register and get the error code related to license timer loading.
      *   This method will access to the system bus to read the error register.
      *   \param[out] licenseTimerLoadError is the error code related to license timer loading.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseTimerLoadErrorRegister(unsigned char &licenseTimerLoadError) const;

      /** waitActivationErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      unsigned int waitLicenseTimerLoadErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readDnaRegister
      *   \brief Read the dna register and get the value.
      *   This method will access to the system bus to read the dna register.
      *   \param[out] dna is the dna value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readDnaRegister(std::vector<unsigned int> &dna) const;

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
      unsigned int readSaasChallengeRegister(std::vector<unsigned int> &saasChallenge) const;

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
      unsigned int readLicenseTimerCounterRegister(std::vector<unsigned int> &licenseTimerCounter) const;

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
      unsigned int readDrmVersionRegister(unsigned int &drmVersion) const;

      /** readDrmVersionRegister
      *   \brief Read the drm version register and get the value.
      *   This method will access to the system bus to read the drm version register.
      *   \param[out] drmVersion is the drm version value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readDrmVersionRegister(std::string &drmVersion) const;

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLogsRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &logs) const;

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLogsRegister(const unsigned int &numberOfIps, std::string &logs) const;

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
      unsigned int readVlnvFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &vlnvFile) const;

      /** readVlnvFileRegister
      *   \brief Read the vlnv file and get the value.
      *   This method will access to the system bus to read the vlnv file.
      *   The vlnv file will contains numberOfIps+1 elements. The first one
      *   is dedicated to the drm controller, the others correspond for
      *   the IPs connected to the drm controller.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] vlnvFile is the vlnv file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readVlnvFileRegister(const unsigned int &numberOfIps, std::vector<std::string> &vlnvFile) const;

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize, std::vector<unsigned int> &licenseFile) const;

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   The license file is a string using a hexadecimal representation.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize, std::string &licenseFile) const;

      /** writeLicenseFile
      *   \brief Write the license file.
      *   This method will access to the system bus to write the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[in] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeLicenseFileRegister(const unsigned int &licenseFileSize, const std::vector<unsigned int> &licenseFile) const;

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
      unsigned int writeLicenseFileRegister(const std::string &licenseFile) const;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readTraceFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &traceFile) const;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readTraceFileRegister(const unsigned int &numberOfIps, std::vector<std::string> &traceFile) const;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readMeteringFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &meteringFile) const;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readMeteringFileRegister(const unsigned int &numberOfIps, std::vector<std::string> &meteringFile) const;

      /** readMailboxFileSizeRegister
      *   \brief Read the mailbox file word numbers.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int readMailboxFileSizeRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber) const;

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
      unsigned int readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                           std::vector<unsigned int> &readOnlyMailboxData, std::vector<unsigned int> &readWriteMailboxData) const;

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
      unsigned int readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                                   std::vector<std::string> &readOnlyMailboxData, std::vector<std::string> &readWriteMailboxData) const;

      /** writeMailboxFileRegister
      *   \brief Write the mailbox file.
      *   This method will access to the system bus to write the mailbox file.
      *   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeMailboxFileRegister(const std::vector <unsigned int> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const;

      /** writeMailboxFileRegister
      *   \brief Write the mailbox file.
      *   This method will access to the system bus to write the mailbox file.
      *   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      unsigned int writeMailboxFileRegister(const std::vector<std::string> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const;

      /** throwFunctionalityDisabledException
      *   \param[in]  expectedStatus is the value of the status to be expected.
      *   \param[in]  actualStatus is the value of the status read.
      *   \param[in]  expectedError is the value of the error to be expected.
      *   \param[in]  actualError is the value of the error read.
      *   \throw Throw DrmControllerLicenseTimerResetedException. DrmControllerLicenseTimerResetedException::what() should be called to get the exception description.
      **/
      void throwLicenseTimerResetedException(const bool &expectedStatus, const bool &actualStatus, const unsigned char &expectedError, const unsigned char &actualError) const;

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
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                 const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                 const unsigned int &expectedStatus, const unsigned int &actualStatus,
                                 const unsigned char &expectedError, const unsigned char &actualError) const;

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
      *   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
      */
      void throwTimeoutException(const std::string &headerMsg,
                                 const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                 const std::string &expectedErrorMsg, const std::string &actualErrorMsg,
                                 const bool &expectedStatus, const bool &actualStatus,
                                 const unsigned char &expectedError, const unsigned char &actualError) const;

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

      /** printHwReport
      * \brief Print all register content accessible through AXI-4 Lite Control channel.
      * \param[in] file: Reference to output file where register contents are saved. By default print on standard output
      */
      void printHwReport(std::ostream &file = std::cout) const;

      /** printMeteringFile
      *   \brief Display the value of the metering file.
      *   \param[in] file is the stream to use for the data print. By default print on standard output.
      **/
      void printMeteringFileHwReport(std::ostream &file = std::cout) const;

      /** getDrmErrorRegisterMessage
      *   \brief Get the error message from the error register value.
      *   \param[in] errorRegister is the value of the error register.
      *   \return Returns the error message.
      **/
      const char* getDrmErrorRegisterMessage(const unsigned char &errorRegister) const;

    // protected members, functions ...
    protected:

    // private members, functions ...
    private:

      DrmControllerRegistersStrategyInterface *mDrmControllerRegistersStrategyInterface;

      typedef std::map<std::string, DrmControllerRegistersStrategyInterface*>                         tDrmControllerRegistersStrategyDictionary;    /*<! Dictionary of register strategies. */
      typedef std::map<std::string, DrmControllerRegistersStrategyInterface*>::reverse_iterator       tDrmControllerRegistersStrategyIterator;      /*<! Dictionary iterator of register strategies. */
      typedef std::map<std::string, DrmControllerRegistersStrategyInterface*>::const_reverse_iterator tDrmControllerRegistersStrategyConstIterator; /*<! Dictionary const iterator of register strategies. */

      typedef std::map<std::string, std::string>                         tDrmControllerRegistersVersionDictionary;    /*<! Dictionary of version strategies. */
      typedef std::map<std::string, std::string>::const_reverse_iterator tDrmControllerRegistersVersionConstIterator; /*<! Dictionary const iterator of version strategies. */

      /** selectRegistersStrategy
      *   \brief Select the register strategy that fits the most with the hardware.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      *   \return Returns the address of the selected DrmControllerRegistersStrategyInterface.
      *   \throw DrmControllerVersionCheckException whenever an error occured. DrmControllerVersionCheckException::what() should be called to get the exception description.
      **/
      DrmControllerRegistersStrategyInterface* selectRegistersStrategy(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction) const;

      /** createRegistersStrategies
      *   \brief Create the register strategies.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      *   \return Returns a dictionary of supported drm version and DrmControllerRegistersStrategyInterface.
      **/
      tDrmControllerRegistersStrategyDictionary createRegistersStrategies(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction) const;

      /** getRegistersStrategy
      *   \brief Get the selected register strategy and free the unselected strategies.
      *   \param[in] strategies is the dictionary of register strategies.
      *   \param[in] selectedVersion is the selected drm version.
      *   \return Returns the address of the selected DrmControllerRegistersStrategyInterface.
      **/
      DrmControllerRegistersStrategyInterface* getRegistersStrategy(tDrmControllerRegistersStrategyDictionary &strategies, const std::string &selectedVersion) const;

      /** readStrategiesDrmVersion
      *   \brief Read the drm version of each strategy.
      *   \param[in] strategies is the dictionary of register strategies.
      *   \return Returns a dictionary of supported drm version and read drm version.
      **/
      tDrmControllerRegistersVersionDictionary readStrategiesDrmVersion(const tDrmControllerRegistersStrategyDictionary &strategies) const;

      /** filterStrategiesDrmVersion
      *   \brief Filter the strategies drm version dictionary.
      *   \param[in] strategiesVersion is the dictionary of strategies version.
      *   \return Returns an updated dictionary of supported drm version and read drm version.
      **/
      tDrmControllerRegistersVersionDictionary filterStrategiesDrmVersion(const tDrmControllerRegistersVersionDictionary &strategiesVersion) const;

      /** parseStrategiesDrmVersion
      *   \brief Parse the drm version of each strategy.
      *   \param[in] strategiesVersion is the dictionary of strategies version.
      *   \return Returns a string of the supported drm version.
      **/
      std::string parseStrategiesDrmVersion(const tDrmControllerRegistersVersionDictionary &strategiesVersion) const;

      /** checkSupportedDrmVersion
      *   \brief Check the drm version is supported.
      *   \param[in] supportedVersion is the supported version of the DRM Controller.
      *   \param[in] drmVersion is the read drm version of the DRM Controller.
      *   \return Returns true if the version is supported, false otherwize.
      **/
      bool checkSupportedDrmVersion(const std::string &supportedVersion, const std::string &drmVersion) const;

      /** operator<<
      *   \brief Declaration of friend function for output stream operator.
      *   \param[in] file is the stream to use for the data display.
      *   \param[in] drmControllerRegisters is a reference to this object.
      *   \return Returns the output stream used
      **/
      friend std::ostream& operator<<(std::ostream &file, const DrmControllerRegisters &drmControllerRegisters);

  }; // class DrmControllerRegisters

  /** operator<<
  *   \brief Declaration of function for output stream operator.
  *   \param[in] file is the stream to use for the data display.
  *   \param[in] drmControllerRegisters is a reference to this object.
  *   \return Returns the output stream used
  **/
  std::ostream& operator<<(std::ostream &file, const DrmControllerRegisters &drmControllerRegisters);

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_REGISTERS_HPP__
