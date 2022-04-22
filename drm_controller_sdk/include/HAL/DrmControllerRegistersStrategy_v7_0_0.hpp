/**
*  \file      DrmControllerRegistersStrategy_v7_0_0.hpp
*  \version   7.1.0.0
*  \date      January 2022
*  \brief     Class DrmControllerRegistersStrategy_v7_0_0 defines strategy for register access of drm controller v7.0.0.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_REGISTERS_STRATEGY_V7_0_0_HPP__
#define __DRM_CONTROLLER_REGISTERS_STRATEGY_V7_0_0_HPP__

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <time.h>
#include <sys/time.h>

#include <HAL/DrmControllerRegistersStrategyInterface.hpp>

// version of the DRM Controller supported by this class
#define DRM_CONTROLLER_V7_0_0_SUPPORTED_VERSION "7.0.0" /**<Definition of the version of the supported DRM Controller.**/

// Name of the registers.
#define DRM_CONTROLLER_V7_0_0_PAGE_REGISTER_NAME    "DrmPageRegister" /**<Definition of the name of the page register.**/
#define DRM_CONTROLLER_V7_0_0_INDEXED_REGISTER_NAME "DrmRegisterLine" /**<Definition of the base name of indexed registers.**/

// Size of registers
#define DRM_CONTROLLER_V7_0_0_COMMAND_SIZE               32   /**<Definition of the register size for the command register.**/
#define DRM_CONTROLLER_V7_0_0_LICENSE_START_ADDRESS_SIZE 64  /**<Definition of the register size for the license start address register.**/
#define DRM_CONTROLLER_V7_0_0_LICENSE_TIMER_SIZE         384 /**<Definition of the register size for the license timer register.**/
#define DRM_CONTROLLER_V7_0_0_STATUS_SIZE                32  /**<Definition of the register size for the status register.**/
#define DRM_CONTROLLER_V7_0_0_ERROR_SIZE                 32  /**<Definition of the register size for the error register.**/
#define DRM_CONTROLLER_V7_0_0_DEVICE_DNA_SIZE            128 /**<Definition of the register size for the device dna register.**/
#define DRM_CONTROLLER_V7_0_0_SAAS_CHALLENGE_SIZE        128 /**<Definition of the register size for the saas challenge register.**/
#define DRM_CONTROLLER_V7_0_0_LICENSE_TIMER_COUNTER_SIZE 64  /**<Definition of the register size for the license timer counter register.**/
#define DRM_CONTROLLER_V7_0_0_VERSION_SIZE               24  /**<Definition of the register size for the version register.**/
#define DRM_CONTROLLER_V7_0_0_ADAPTIVE_PROPORTION_TEST_FAILURES_SIZE                  16  /**<Definition of the register size for the Adaptive Proportion Test Failures register.**/
#define DRM_CONTROLLER_V7_0_0_REPETITION_COUNT_TEST_FAILURES_SIZE                  16  /**<Definition of the register size for the Repetition Count Test Failures register.**/
#define DRM_CONTROLLER_V7_0_0_VLNV_WORD_SIZE             64  /**<Definition of the register size for the vlnv word.**/
#define DRM_CONTROLLER_V7_0_0_LICENSE_WORD_SIZE          128 /**<Definition of the register size for the license word.**/
#define DRM_CONTROLLER_V7_0_0_TRACE_WORD_SIZE            64  /**<Definition of the register size for the trace word.**/
#define DRM_CONTROLLER_V7_0_0_METERING_WORD_SIZE         128 /**<Definition of the register size for the metering word.**/
#define DRM_CONTROLLER_V7_0_0_MAILBOX_WORD_SIZE          32  /**<Definition of the register size for the mailbox word.**/

#define DRM_CONTROLLER_V7_0_0_NUMBER_OF_TRACES_PER_IP 3 /**<Definition of the number of traces per ip.**/

#define DRM_CONTROLLER_V7_0_0_VLNV_NUMBER_OF_ADDITIONAL_WORDS     1 /**<Definition of the number of additional words for the vlnv file.**/
#define DRM_CONTROLLER_V7_0_0_METERING_NUMBER_OF_ADDITIONAL_WORDS 3 /**<Definition of the number of additional words for the metering file.**/
#define DRM_CONTROLLER_V7_0_0_MAILBOX_NUMBER_OF_ADDITIONAL_WORDS  1 /**<Definition of the number of additional words for the mailbox file.**/

#define DRM_CONTROLLER_V7_0_0_LICENSE_HEADER_BLOCK_SIZE 7 /**<Definition of the number of license word used for the header block.**/
#define DRM_CONTROLLER_V7_0_0_LICENSE_IP_BLOCK_SIZE     4 /**<Definition of the number of license word used for an ip block.**/

#define DRM_CONTROLLER_V7_0_0_NUMBER_OF_ERROR_CODES     22 /**<Definition of the number of error codes.**/

#define DRM_CONTROLLER_V7_0_0_METERING_FILE_HEADER_POSITION                 0 /**<Definition of the position of the metering file header.**/
#define DRM_CONTROLLER_V7_0_0_METERING_FILE_LICENSE_TIMER_COUNT_POSITION    1 /**<Definition of the position of the license timer in the metering file.**/
#define DRM_CONTROLLER_V7_0_0_METERING_FILE_FIRST_IP_METERING_DATA_POSITION 2 /**<Definition of the position of the first metering data in the metering file.**/
#define DRM_CONTROLLER_V7_0_0_METERING_FILE_MAC_FROM_END_POSITION           0 /**<Definition of the position of the mac from the end of the metering file.**/

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /**
  *   \class    DrmControllerRegistersStrategy_v7_0_0 DrmControllerRegistersStrategy_v7_0_0.hpp "include/HAL/DrmControllerRegistersStrategy_v7_0_0.hpp"
  *   \brief    Class DrmControllerRegistersStrategy_v7_0_0 defines strategy for register access of drm controller v7.0.0.
  **/
  class DrmControllerRegistersStrategy_v7_0_0: public DrmControllerRegistersStrategyInterface {

    // public members, functions ...
    public:

      /** DrmControllerRegistersStrategy_v7_0_0
      *   \brief Class constructor.
      *   \param[in] readRegisterFunction function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] writeRegisterFunction function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerRegistersStrategy_v7_0_0(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction);

      /** ~DrmControllerRegistersStrategy_v7_0_0
      *   \brief Class destructor.
      **/
      virtual ~DrmControllerRegistersStrategy_v7_0_0();

      /** writeRegistersPageRegister
      *   \brief Write the page register to select the registers page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeRegistersPageRegister() const;

      /** writeVlnvFilePageRegister
      *   \brief Write the page register to select the vlnv file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeVlnvFilePageRegister() const;

      /** writeLicenseFilePageRegister
      *   \brief Write the page register to select the license file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseFilePageRegister() const;

      /** writeTraceFilePageRegister
      *   \brief Write the page register to select the trace file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeTraceFilePageRegister() const;

      /** writeMeteringFilePageRegister
      *   \brief Write the page register to select the metering file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMeteringFilePageRegister() const;

      /** writeMailBoxFilePageRegister
      *   \brief Write the page register to select the mailbox file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMailBoxFilePageRegister() const;

      /** writeNopCommandRegister
      *   \brief Write the command register to the NOP Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeNopCommandRegister() const;

      /** writeDnaExtractCommandRegister
      *   \brief Write the command register to the DNA Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeDnaExtractCommandRegister() const;

      /** writeVlnvExtractCommandRegister
      *   \brief Write the command register to the VLNV Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeVlnvExtractCommandRegister() const;

      /** writeActivateCommandRegister
      *   \brief Write the command register to the Activate Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeActivateCommandRegister() const;

      /** writeEndSessionMeteringExtractCommandRegister
      *   \brief Write the command register to the end session extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeEndSessionMeteringExtractCommandRegister() const;

      /** writeMeteringExtractCommandRegister
      *   \brief Write the command register to the extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMeteringExtractCommandRegister() const;

      /** writeSampleLicenseTimerCounterCommandRegister
      *   \brief Write the command register to the sample license timer counter Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeSampleLicenseTimerCounterCommandRegister() const;

      /** writeLicenseTimerInitSemaphoreRequestCommandRegister
      *   \brief Write the LicenseTimerInitSemaphoreRequest bit in the command register to the given value (do not modify the other bits of the command register).
      *   This method will access to the system bus to write into the command register.
      *   \param[in] licenseTimerInitSemaphoreRequest is the value of the LicenseTimerInitSemaphoreRequest bit to write in the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseTimerInitSemaphoreRequestCommandRegister(bool &licenseTimerInitSemaphoreRequest) const;

      /** readLicenseStartAddressRegister
      *   \brief Read the license start address register.
      *   This method will access to the system bus to read the license start address.
      *   \param[out] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseStartAddressRegister(std::vector<unsigned int> &licenseStartAddress) const;

      /** writeLicenseStartAddressRegister
      *   \brief Write the license start address register.
      *   This method will access to the system bus to write the license start address.
      *   \param[in] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseStartAddressRegister(const std::vector<unsigned int> &licenseStartAddress) const;

      /** readLicenseTimerInitRegister
      *   \brief Read the license timer register.
      *   This method will access to the system bus to read the license timer.
      *   \param[out] licenseTimerInit is a list of binary values for the license timer register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerInitRegister(std::vector<unsigned int> &licenseTimerInit) const;

      /** writeLicenseTimerInitRegister
      *   \brief Write the license start address register.
      *   This method will access to the system bus to write the license timer.
      *   \param[in] licenseTimerInit is a list of binary values for the license timer register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseTimerInitRegister(const std::vector<unsigned int> &licenseTimerInit) const;

      /** readDnaReadyStatusRegister
      *   \brief Read the status register and get the dna ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] dnaReady is the value of the status bit DNA Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readDnaReadyStatusRegister(bool &dnaReady) const;

      /** waitDnaReadyStatusRegister
      *   \brief Wait dna ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitDnaReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readVlnvReadyStatusRegister
      *   \brief Read the status register and get the vlnv ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] vlnvReady is the value of the status bit VLNV Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readVlnvReadyStatusRegister(bool &vlnvReady) const;

      /** waitVlnvReadyStatusRegister
      *   \brief Wait vlnv ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitVlnvReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readActivationDoneStatusRegister
      *   \brief Read the status register and get the activation done status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationDone is the value of the status bit Activation Done.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readActivationDoneStatusRegister(bool &activationDone) const;

      /** waitActivationDoneStatusRegister
      *   \brief Wait activation done status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitActivationDoneStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readAutonomousControllerEnabledStatusRegister
      *   \brief Read the status register and get the autonomous controller enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoEnabled is the value of the status bit autonomous controller enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAutonomousControllerEnabledStatusRegister(bool &autoEnabled) const;

      /** readAutonomousControllerBusyStatusRegister
      *   \brief Read the status register and get the autonomous controller busy status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoBusy is the value of the status bit autonomous controller busy.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAutonomousControllerBusyStatusRegister(bool &autoBusy) const;

      /** waitAutonomousControllerBusyStatusRegister
      *   \brief Wait autonomous controller busy status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitAutonomousControllerBusyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readMeteringEnabledStatusRegister
      *   \brief Read the status register and get the metering enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringEnabled is the value of the status bit metering enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringEnabledStatusRegister(bool &meteringEnabled) const;

      /** readMeteringReadyStatusRegister
      *   \brief Read the status register and get the metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringReady is the value of the status bit metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringReadyStatusRegister(bool &meteringReady) const;

      /** waitMeteringReadyStatusRegister
      *   \brief Wait metering ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readSaasChallengeReadyStatusRegister
      *   \brief Read the status register and get the saas challenge ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSaasChallengeReadyStatusRegister(bool &saasChallengeReady) const;

      /** waitSaasChallengeReadyStatusRegister
      *   \brief Wait saas challenge ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitSaasChallengeReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerEnabledStatusRegister
      *   \brief Read the status register and get the license timer enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const;

      /** readLicenseTimerInitLoadedStatusRegister
      *   \brief Read the status register and get the license timer init loaded status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init load.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerInitLoadedStatusRegister(bool &licenseTimerInitLoaded) const;

      /** waitLicenseTimerInitLoadedStatusRegister
      *   \brief Wait license timer init loaded status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerInitLoadedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readEndSessionMeteringReadyStatusRegister
      *   \brief Read the status register and get the end session metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] endSessionMeteringReady is the value of the status bit end session metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readEndSessionMeteringReadyStatusRegister(bool &endSessionMeteringReady) const;

      /** waitEndSessionMeteringReadyStatusRegister
      *   \brief Wait end session metering ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitEndSessionMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readHeartBeatModeEnabledStatusRegister
      *   \brief Read the status register and get the heart beat mode enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] heartBeatModeEnabled is the value of the status bit heart beat mode enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readHeartBeatModeEnabledStatusRegister(bool &heartBeatModeEnabled) const;

      /** readAsynchronousMeteringReadyStatusRegister
      *   \brief Read the status register and get the asynchronous metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] asynchronousMeteringReady is the value of the status bit asynchronous metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAsynchronousMeteringReadyStatusRegister(bool &asynchronousMeteringReady) const;
    
      /** waitAsynchronousMeteringReadyStatusRegister
      *   \brief Wait asynchronous metering ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitAsynchronousMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerSampleReadyStatusRegister
      *   \brief Read the status register and get the license timer sample ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerSampleReady is the value of the status bit license timer sample ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerSampleReadyStatusRegister(bool &licenseTimerSampleReady) const;

      /** waitLicenseTimerSampleReadyStatusRegister
      *   \brief Wait license timer sample ready status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerSampleReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerCountEmptyStatusRegister
      *   \brief Read the status register and get the license timer count empty status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerCounterEmpty is the value of the status bit license timer count empty.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerCountEmptyStatusRegister(bool &licenseTimerCounterEmpty) const;

      /** waitLicenseTimerCountEmptyStatusRegister
      *   \brief Wait license timer count empty status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerCountEmptyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;
    
      /** readSessionRunningStatusRegister
      *   \brief Read the status register and get the session running status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] sessionRunning is the value of the status bit session running.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSessionRunningStatusRegister(bool &sessionRunning) const;
    
      /** waitSessionRunningStatusRegister
      *   \brief Wait session running status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitSessionRunningStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readActivationCodesTransmittedStatusRegister
      *   \brief Read the status register and get the activation codes transmitted status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationCodeTransmitted is the value of the status bit activation codes transmitted.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readActivationCodesTransmittedStatusRegister(bool &activationCodeTransmitted) const;

      /** waitActivationCodesTransmittedStatusRegister
      *   \brief Wait activation codes transmitted status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitActivationCodesTransmittedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseNodeLockStatusRegister
      *   \brief Read the status register and get the license node lock status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseNodeLock is the value of the status bit license node lock.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseNodeLockStatusRegister(bool &licenseNodeLock) const;

      /** waitLicenseNodeLockStatusRegister
      *   \brief Wait license node lock status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseNodeLockStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseMeteringStatusRegister
      *   \brief Read the status register and get the license metering status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseMetering is the value of the status bit license metering.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseMeteringStatusRegister(bool &licenseMetering) const;

      /** waitLicenseMeteringStatusRegister
      *   \brief Wait license metering status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseMeteringStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readSecurityAlertStatusRegister
      *   \brief Read the status register and get the Security Alert bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] securityAlert is the value of the status bit Security Alert.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSecurityAlertStatusRegister(bool &securityAlert) const;

      /** waitSecurityAlertgStatusRegister
      *   \brief Wait Security Alert status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status bit read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitSecurityAlertStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readLicenseTimerInitSemaphoreAcknowledgeStatusRegister
      *   \brief Read the status register and get the License Timer Init Semaphore Acknowledge status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerInitSemaphoreAcknowledge is the value of the status bit License Timer Init Semaphore Acknowledge.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerInitSemaphoreAcknowledgeStatusRegister(bool &licenseTimerInitSemaphoreAcknowledge) const;

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
      virtual unsigned int waitLicenseTimerInitSemaphoreAcknowledgeStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const;

      /** readNumberOfLicenseTimerLoadedStatusRegister
      *   \brief Read the status register and get the number of license timer loaded.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfLicenseTimerLoaded is the number of license timer loaded retrieved from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readNumberOfLicenseTimerLoadedStatusRegister(unsigned int &numberOfLicenseTimerLoaded) const;

      /** waitNumberOfLicenseTimerLoadedStatusRegister
      *   \brief Wait number of license timer loaded status register to reach specified value.
      *   This method will access to the system bus to read the status register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the status to be expected.
      *   \param[out] actual is the value of the status read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitNumberOfLicenseTimerLoadedStatusRegister(const unsigned int &timeout, const unsigned int &expected, unsigned int &actual) const;

      /** readNumberOfDetectedIpsStatusRegister
      *   \brief Read the status register and get the number of detected IPs.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfDetectedIps is the number of detected ips retrieved from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readNumberOfDetectedIpsStatusRegister(unsigned int &numberOfDetectedIps) const;

      /** readExtractDnaErrorRegister
      *   \brief Read the error register and get the error code related to dna extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] dnaExtractError is the error code related to dna extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readExtractDnaErrorRegister(unsigned char &dnaExtractError) const;

      /** waitExtractDnaErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitExtractDnaErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readExtractVlnvErrorRegister
      *   \brief Read the error register and get the error code related to vlnv extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] vlnvExtractError is the error code related to vlnv extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readExtractVlnvErrorRegister(unsigned char &vlnvExtractError) const;

      /** waitExtractVlnvErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitExtractVlnvErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readActivationErrorRegister
      *   \brief Read the error register and get the error code related to activation.
      *   This method will access to the system bus to read the error register.
      *   \param[out] activationError is the error code related to activation.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readActivationErrorRegister(unsigned char &activationError) const;

      /** waitActivationErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitActivationErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readLicenseTimerLoadErrorRegister
      *   \brief Read the error register and get the error code related to license timer loading.
      *   This method will access to the system bus to read the error register.
      *   \param[out] licenseTimerLoadError is the error code related to license timer loading.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerLoadErrorRegister(unsigned char &licenseTimerLoadError) const;

      /** waitActivationErrorRegister
      *   \brief Wait error to reach specified value.
      *   This method will access to the system bus to read the error register.
      *   \param[in]  timeout is the timeout value in micro seconds.
      *   \param[in]  expected is the value of the error to be expected.
      *   \param[out] actual is the value of the error read.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
      *   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
      **/
      virtual unsigned int waitLicenseTimerLoadErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const;

      /** readDnaRegister
      *   \brief Read the dna register and get the value.
      *   This method will access to the system bus to read the dna register.
      *   \param[out] dna is the dna value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readDnaRegister(std::vector<unsigned int> &dna) const;

      /** readSaasChallengeRegister
      *   \brief Read the Saas Challenge register and get the value.
      *   This method will access to the system bus to read the Saas Challenge register.
      *   \param[out] saasChallenge is the saas challenge value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readSaasChallengeRegister(std::vector<unsigned int> &saasChallenge) const;

      /** readLicenseTimerCounterRegister
      *   \brief Read the License Timer Counter register and get the value.
      *   This method will access to the system bus to read the License Timer Counter register.
      *   \param[out] licenseTimerCounter is the License Timer Counter value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseTimerCounterRegister(std::vector<unsigned int> &licenseTimerCounter) const;

      /** readDrmVersionRegister
      *   \brief Read the drm version register and get the value.
      *   This method will access to the system bus to read the drm version register.
      *   \param[out] drmVersion is the drm version value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readDrmVersionRegister(unsigned int &drmVersion) const;

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLogsRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &logs) const;

      /** readVlnvFileRegister
      *   \brief Read the vlnv file and get the value.
      *   This method will access to the system bus to read the vlnv file.
      *   The vlnv file will contains numberOfIps+1 words. The first one
      *   is dedicated to the drm controller, the others correspond for
      *   the IPs connected to the drm controller.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] vlnvFile is the vlnv file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readVlnvFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &vlnvFile) const;

      /** readVlnvFileRegister
      *   \brief Read the vlnv file and get the value.
      *   This method will access to the system bus to read the vlnv file.
      *   The vlnv file will contains numberOfIps+1 elements. The first one
      *   is dedicated to the drm controller, the others correspond for
      *   the IPs connected to the drm controller.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] vlnvFile is the vlnv file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readVlnvFileRegister(const unsigned int &numberOfIPs, std::vector<std::string> &vlnvFile) const;

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize, std::vector<unsigned int> &licenseFile) const;

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   The license file is a string using a hexadecimal representation.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize, std::string &licenseFile) const;

      /** writeLicenseFile
      *   \brief Write the license file.
      *   This method will access to the system bus to write the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[in] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeLicenseFileRegister(const unsigned int &licenseFileSize, const std::vector<unsigned int> &licenseFile) const;

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
      virtual unsigned int writeLicenseFileRegister(const std::string &licenseFile) const;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readTraceFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &traceFile) const;

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readTraceFileRegister(const unsigned int &numberOfIPs, std::vector<std::string> &traceFile) const;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &meteringFile) const;

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIPs is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMeteringFileRegister(const unsigned int &numberOfIPs, std::vector<std::string> &meteringFile) const;

      /** readMailboxFileSizeRegister
      *   \brief Read the mailbox file word numbers.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMailboxFileSizeRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber) const;

      /** readMailboxFileRegister
      *   \brief Read the mailbox file.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \param[out] readOnlyMailboxData is the data read from the read-only mailbox.
      *   \param[out] readWriteMailboxData is the data read from the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                                   std::vector<unsigned int> &readOnlyMailboxData, std::vector<unsigned int> &readWriteMailboxData) const;

      /** readMailboxFileRegister
      *   \brief Read the mailbox file.
      *   This method will access to the system bus to read the mailbox file.
      *   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \param[out] readOnlyMailboxData is the data read from the read-only mailbox.
      *   \param[out] readWriteMailboxData is the data read from the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                                   std::vector<std::string> &readOnlyMailboxData, std::vector<std::string> &readWriteMailboxData) const;

      /** writeMailboxFileRegister
      *   \brief Write the mailbox file.
      *   This method will access to the system bus to write the mailbox file.
      *   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMailboxFileRegister(const std::vector <unsigned int> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const;

      /** writeMailboxFileRegister
      *   \brief Write the mailbox file.
      *   This method will access to the system bus to write the mailbox file.
      *   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
      *   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeMailboxFileRegister(const std::vector<std::string> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const;

      /** printMeteringFile
      *   \brief Display the value of the metering file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printMeteringFileHwReport(std::ostream &file) const;

      /** getDrmErrorRegisterMessage
      *   \brief Get the error message from the error register value.
      *   \param[in] errorRegister is the value of the error register.
      *   \return Returns the error message.
      **/
      virtual const char* getDrmErrorRegisterMessage(const unsigned char &errorRegister) const;
      
      /** readAdaptiveProportionTestFailuresRegister
      *   \brief Read the Adaptive Proportion Test Failures register and get the value.
      *   This method will access to the system bus to read the Adaptive Proportion Test Failures register.
      *   \param[out] adaptiveProportionTestFailures is the Adaptive Proportion Test Failures value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readAdaptiveProportionTestFailuresRegister(std::vector<unsigned int> &adaptiveProportionTestFailures) const;
      
      /** readRepetitionCountTestFailuresRegister
      *   \brief Read the Repetition Count Test Failures register and get the value.
      *   This method will access to the system bus to read the Repetition Count Test Failures register.
      *   \param[out] repetitionCountTestFailures is the Repetition Count Test Failures value.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readRepetitionCountTestFailuresRegister(std::vector<unsigned int> &repetitionCountTestFailures) const;

    // protected members, functions ...
    protected:

      /** readPageRegister
      *   \brief Read and get the value of the page register from the hardware.
      *   This method will access to the system bus to read the page register.
      *   \param[out] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readPageRegister(unsigned int &page) const;

      /** writePageRegister
      *   \brief Write the value of the page register into the hardware.
      *   This method will access to the system bus to write the page register.
      *   \param[in] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writePageRegister(const unsigned int &page) const;

      /** readCommandRegister
      *   \brief Read and get the value of the command register from the hardware.
      *   This method will access to the system bus to read the command register.
      *   \param[out] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readCommandRegister(unsigned int &command) const;

      /** writeCommandRegister
      *   \brief Write the value of the command register into the hardware.
      *   This method will access to the system bus to write the command register.
      *   \param[in] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int writeCommandRegister(const unsigned int &command) const;

      /** readStatusRegister
      *   \brief Read the status register.
      *   This method will access to the system bus to read the status register.
      *   \param[out] status is the binary value of the status register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readStatusRegister(unsigned int &status) const;

      /** readErrorRegister
      *   \brief Read the error register.
      *   This method will access to the system bus to read the error register.
      *   \param[out] error is the binary value of the error register.
      *   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwize.
      *   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
      **/
      virtual unsigned int readErrorRegister(unsigned int &error) const;

    // private members, functions ...
    private:

      /** checkLicenseFileSize
      *   \brief Verify that the size of the license file is correct.
      *   \param[in] licenseFile is the license file.
      *   \return Returns true if license file size is correct, false otherwize.
      *   \throw DrmControllerLicenseFileSizeException whenever a check on license file size is bad. DrmControllerLicenseFileSizeException::what() should be called to get the exception description.
      **/
      bool checkLicenseFileSize(const std::vector<unsigned int> &licenseFile) const;

      /** printPage
      *   \brief Display the value of the page register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printPageHwReport(std::ostream &file) const;

      /** printCommand
      *   \brief Display the value of the command register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printCommandHwReport(std::ostream &file) const;

      /** printLicenseStartAddress
      *   \brief Display the value of the license start address register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseStartAddressHwReport(std::ostream &file) const;

      /** printLicenseTimer
      *   \brief Display the value of the license timer register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseTimerHwReport(std::ostream &file) const;

      /** printStatus
      *   \brief Display the value of the status register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printStatusHwReport(std::ostream &file) const;

      /** printError
      *   \brief Display the value of the error register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printErrorHwReport(std::ostream &file) const;

      /** printDrmVersion
      *   \brief Display the value of the drm version.
      **/
      virtual void printDrmVersionHwReport(std::ostream &file) const;

      /** printDna
      *   \brief Display the value of the dna.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printDnaHwReport(std::ostream &file) const;

      /** printSaasChallenge
      *   \brief Display the value of the saas challenge.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printSaasChallengeHwReport(std::ostream &file) const;

      /** printLicenseTimerCounter
      *   \brief Display the value of the license timer counter.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseTimerCounterHwReport(std::ostream &file) const;

      /** printLogs
      *   \brief Display the value of the logs register.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLogsHwReport(std::ostream &file) const;

      /** printVlnvFile
      *   \brief Display the value of the vlnv file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printVlnvFileHwReport(std::ostream &file) const;

      /** printLicenseFile
      *   \brief Display the license file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printLicenseFileHwReport(std::ostream &file) const;

      /** printTraceFile
      *   \brief Display the trace file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printTraceFileHwReport(std::ostream &file) const;

      /** printMailBoxFile
      *   \brief Display the value of the mailbox file.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printMailBoxFileHwReport(std::ostream &file) const;
      
      /** printAdaptiveProportionTestFailures
      *   \brief Display the value of the Adaptive Proportion Test Failures.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printAdaptiveProportionTestFailuresHwReport(std::ostream &file) const;
      
      /** printRepetitionCountTestFailures
      *   \brief Display the value of the Repetition Count Test Failures.
      *   \param[in] file is the stream to use for the data print.
      **/
      virtual void printRepetitionCountTestFailuresHwReport(std::ostream &file) const;

      /** getMeteringFileHeader
      *   \brief Get the header of the metering file.
      *   \param[in] meteringFile is a list containing the metering file to retrieve the header from.
      *   \return Returns a list containing the metering file header.
      **/
      std::vector<unsigned int> getMeteringFileHeader(const std::vector<unsigned int> &meteringFile) const;

      /** getMeteringFileHeaderSessionId
      *   \brief Get the session id from the metering file header.
      *   \param[in] meteringFileHeader is a list containing the metering file header to retrieve the session id from.
      *   \return Returns a list containing the metering file session id.
      **/
      std::vector<unsigned int> getMeteringFileHeaderSessionId(const std::vector<unsigned int> &meteringFileHeader) const;

      /** getMeteringFileHeaderEncryptedMeteringFlag
      *   \brief Get the encrypted metering flag from the metering file header.
      *   \param[in] meteringFileHeader is a list containing the metering file header to retrieve the encrypted metering flag from.
      *   \return Returns true if the metering file is encrypted, false otherwize.
      **/
      bool getMeteringFileHeaderEncryptedMeteringFlag(const std::vector<unsigned int> &meteringFileHeader) const;

      /** getMeteringFileHeaderEndSessionMeteringFlag
      *   \brief Get the end session metering flag from the metering file header.
      *   \param[in] meteringFileHeader is a list containing the metering file header to retrieve the end session metering flag from.
      *   \return Returns true if the metering file is for an end session, false otherwize.
      **/
      bool getMeteringFileHeaderEndSessionMeteringFlag(const std::vector<unsigned int> &meteringFileHeader) const;

      /** getMeteringFileHeaderEnvironmentId
      *   \brief Get the environment id from the metering file header.
      *   \param[in] meteringFileHeader is a list containing the metering file header to retrieve the environment id from.
      *   \return Returns the environment id.
      **/
      unsigned int getMeteringFileHeaderEnvironmentId(const std::vector<unsigned int> &meteringFileHeader) const;

      /** getMeteringFileHeaderSegmentIndex
      *   \brief Get the segment index from the metering file header.
      *   \param[in] meteringFileHeader is a list containing the metering file header to retrieve the segment index from.
      *   \return Returns the segment index.
      **/
      unsigned int getMeteringFileHeaderSegmentIndex(const std::vector<unsigned int> &meteringFileHeader) const;

      /** getMeteringFileLicenseTimer
      *   \brief Get the license timer from the metering file.
      *   \param[in] meteringFile is a list containing the metering file to retrieve the license timer from.
      *   \return Returns a list containing the license timer retrieved from metering file.
      **/
      std::vector<unsigned int> getMeteringFileLicenseTimer(const std::vector<unsigned int> &meteringFile) const;

      /** getMeteringFileIpMeteringData
      *   \brief Get the ip metering data from the metering file.
      *   \param[in] meteringFile is a list containing the metering file to retrieve the ip metering data from.
      *   \return Returns a list containing the ip metering retrieved from metering file.
      **/
      std::vector<unsigned int> getMeteringFileIpMeteringData(const std::vector<unsigned int> &meteringFile) const;

      /** getMeteringFileMac
      *   \brief Get the mac from the metering file.
      *   \param[in] meteringFile is a list containing the metering file to retrieve the mac from.
      *   \return Returns a list containing the mac retrieved from metering file.
      **/
      std::vector<unsigned int> getMeteringFileMac(const std::vector<unsigned int> &meteringFile) const;

      // number of words per registers
      const unsigned int mCommandRegisterWordNumber;
      const unsigned int mLicenseStartAddressRegisterWordNumber;
      const unsigned int mLicenseTimerRegisterWordNumber;
      const unsigned int mStatusRegisterWordNumber;
      const unsigned int mErrorRegisterWordNumber;
      const unsigned int mDnaRegisterWordNumber;
      const unsigned int mSaasChallengeRegisterWordNumber;
      const unsigned int mSampledLicenseTimerCountRegisterWordNumber;
      const unsigned int mVersionRegisterWordNumber;
      const unsigned int mAdaptiveProportionTestFailuresRegisterWordNumber;
      const unsigned int mRepetitionCountTestFailuresRegisterWordNumber;
      const unsigned int mVlnvWordRegisterWordNumber;
      const unsigned int mLicenseWordRegisterWordNumber;
      const unsigned int mTraceWordRegisterWordNumber;
      const unsigned int mMeteringWordRegisterWordNumber;
      const unsigned int mMailboxWordRegisterWordNumber;
      

      // start index of each registers
      const unsigned int mCommandRegisterStartIndex;
      const unsigned int mLicenseStartAddressRegisterStartIndex;
      const unsigned int mLicenseTimerRegisterStartIndex;
      const unsigned int mStatusRegisterStartIndex;
      const unsigned int mErrorRegisterStartIndex;
      const unsigned int mDnaRegisterStartIndex;
      const unsigned int mSaasChallengeRegisterStartIndex;
      const unsigned int mSampledLicenseTimerCountRegisterStartIndex;
      const unsigned int mVersionRegisterStartIndex;
      const unsigned int mAdaptiveProportionTestFailuresRegisterStartIndex;
      const unsigned int mRepetitionCountTestFailuresRegisterStartIndex;
      const unsigned int mLogsRegisterStartIndex;
      const unsigned int mVlnvWordRegisterStartIndex;
      const unsigned int mLicenseWordRegisterStartIndex;
      const unsigned int mTraceWordRegisterStartIndex;
      const unsigned int mMeteringWordRegisterStartIndex;
      const unsigned int mMailboxWordRegisterStartIndex;
      
      // number of traces per ip
      const unsigned int mNumberOfTracesPerIp;

      // number of additional words
      const unsigned int mVlnvNumberOfAdditionalWords;
      const unsigned int mMeteringNumberOfAdditionalWords;
      const unsigned int mMailboxNumberOfAdditionalWords;

      // number of words in license 
      const unsigned int mLicenseFileHeaderWordNumber;
      const unsigned int mLicenseFileIpBlockWordNumber;
      const unsigned int mLicenseFileMinimumWordNumber;

      // metering file words positions
      const unsigned int mMeteringFileHeaderWordPosition;
      const unsigned int mMeteringFileLicenseTimerCountWordPosition;
      const unsigned int mMeteringFileFirstIpMeteringDataWordPosition;
      const unsigned int mMeteringFileMacWordFromEndPosition;

      /**
      *   \enum  tDrmPageRegisterEnumValues
      *   \brief Enumeration for page codes.
      **/
      typedef enum tDrmPageRegisterEnumValues {
        mDrmPageRegisters    = 0x00,  /**<Value for the registers page.**/
        mDrmPageVlnvFile     = 0x01,  /**<Value for the vlnv file page.**/
        mDrmPageLicenseFile  = 0x02,  /**<Value for the license file page.**/
        mDrmPageTraceFile    = 0x03,  /**<Value for the trace file page.**/
        mDrmPageMeteringFile = 0x04,  /**<Value for the metering file page.**/
        mDrmPageMailboxFile  = 0x05   /**<Value for the mailbox file page.**/
      } tDrmPageRegisterEnumValues;

      /**
      *   \enum  tDrmCommandRegisterEnumValues
      *   \brief Enumeration for command codes.
      **/
      typedef enum tDrmCommandRegisterEnumValues {
        mDrmCommandNop                              = 0x00000020,  /**<Value for command NOP.**/
        mDrmCommandExtractDna                       = 0x00000001,  /**<Value for command Extract DNA.**/
        mDrmCommandExtractVlnv                      = 0x00000002,  /**<Value for command Extract VLNV.**/
        mDrmCommandActivate                         = 0x00000004,  /**<Value for command Activate.**/
        mDrmCommandHeartBeatInit                    = 0x00000008,  /**<Value for command Heart Beat Initialization (unsued).**/
        mDrmCommandHeartBeatFinish                  = 0x00000010,  /**<Value for command Heart Beat Finalization (unused).**/
        mDrmCommandEndSessionExtractMetering        = 0x00000040,  /**<Value for command End Session Extract Metering.**/
        mDrmCommandExtractMetering                  = 0x00000080,  /**<Value for command Extract Metering.**/
        mDrmCommandSampleLicenseTimerCounter        = 0x00000100,  /**<Value for command Sample License Timer Counter.**/
        mDrmCommandLicenseTimerInitSemaphoreRequest = 0x80000000   /**<Value for the License Timer Init Semaphore Request.**/
      } tDrmCommandRegisterEnumValues;

      /**
      *   \enum  tDrmStatusRegisterEnumValues
      *   \brief Enumeration for status bit positions.
      **/
      typedef enum tDrmStatusRegisterEnumValues {
        mDrmStatusDnaReady                             = 0,  /**<Position of the status DNA Ready.**/
        mDrmStatusVlnvReady                            = 1,  /**<Position of the status VLNV Ready.**/
        mDrmStatusActivationDone                       = 2,  /**<Position of the status Activation Done.**/
        mDrmStatusAutoEnabled                          = 3,  /**<Position of the status Auto Controller Enabled.**/
        mDrmStatusAutoBusy                             = 4,  /**<Position of the status Auto Controller Busy.**/
        mDrmStatusMeteringEnabled                      = 5,  /**<Position of the status Metering Operations Enabled.**/
        mDrmStatusMeteringReady                        = 6,  /**<Position of the status Metering Ready.**/
        mDrmStatusSaasChallengeReady                   = 7,  /**<Position of the status Saas Challenge Ready.**/
        mDrmStatusLicenseTimerEnabled                  = 8,  /**<Position of the status License Timer Operations Enabled.**/
        mDrmStatusLicenseTimerInitLoaded               = 9,  /**<Position of the status License Timer Init Loaded.**/
        mDrmStatusEndSessionMeteringReady              = 10, /**<Position of the status End session metering ready.**/
        mDrmStatusHeartBeatModeEnabled                 = 11, /**<Position of the status Heart beat mode enabled.**/
        mDrmStatusAsynchronousMeteringReady            = 12, /**<Position of the status Asynchronous metering ready.**/
        mDrmStatusLicenseTimerSampleReady              = 13, /**<Position of the status License Timer sample ready.**/
        mDrmStatusLicenseTimerCountEmpty               = 14, /**<Position of the status License Timer Count empty.**/
        mDrmStatusSessionRunning                       = 15, /**<Position of the status Session Running.**/
        mDrmStatusActivationCodesTransmitted           = 16, /**<Position of the status Activation Code Transmitted.**/
        mDrmStatusLicenseNodeLock                      = 17, /**<Position of the status License Node Lock.**/
        mDrmStatusLicenseMetering                      = 18, /**<Position of the status License Metering.**/
        mDrmStatusLicenseTimerLoadedNumberLsb          = 19, /**<LSB position of the status License Timer Loaded Number.**/
        mDrmStatusLicenseTimerLoadedNumberMsb          = 20, /**<MSB position of the status License Timer Loaded Number.**/
        mDrmStatusSecurityAlert                        = 21, /**<Position of the status Security Alert.**/
        mDrmStatusLicenseTimerInitSemaphoreAcknowledge = 22, /**<Position of the status License Timer Init Semaphore Acknowledge.**/
        mDrmStatusIpActivatorNumberLsb                 = 23, /**<LSB position of the status IP Activator Number.**/
        mDrmStatusIpActivatorNumberMsb                 = 31  /**<MSB position of the status IP Activator Number.**/
      } tDrmStatusRegisterEnumValues;

      /**
      *   \enum  tDrmStatusRegisterMaskEnumValues
      *   \brief Enumeration for status bit masks.
      **/
      typedef enum tDrmStatusRegisterMaskEnumValues {
        mDrmStatusMaskDnaReady                             = 0x00000001,   /**<Mask for the status bit DNA Ready.**/
        mDrmStatusMaskVlnvReady                            = 0x00000002,   /**<Mask for the status bit VLNV Ready.**/
        mDrmStatusMaskActivationDone                       = 0x00000004,   /**<Mask for the status bit Activation Done.**/
        mDrmStatusMaskAutoEnabled                          = 0x00000008,   /**<Mask for the status bit Auto Controller Enabled.**/
        mDrmStatusMaskAutoBusy                             = 0x00000010,   /**<Mask for the status bit Auto Controller Busy.**/
        mDrmStatusMaskMeteringEnabled                      = 0x00000020,   /**<Mask for the status bit Metering Operations Enabled.**/
        mDrmStatusMaskMeteringReady                        = 0x00000040,   /**<Mask for the status bit Metering Ready.**/
        mDrmStatusMaskSaasChallengeReady                   = 0x00000080,   /**<Mask for the status bit Saas Challenge Ready.**/
        mDrmStatusMaskLicenseTimerEnabled                  = 0x00000100,   /**<Mask for the status bit License Timer Operations Enabled.**/
        mDrmStatusMaskLicenseTimerInitLoaded               = 0x00000200,   /**<Mask for the status bit License Timer Load Done.**/
        mDrmStatusMaskEndSessionMeteringReady              = 0x00000400,   /**<Mask for the status End session metering ready.**/
        mDrmStatusMaskHeartBeatModeEnabled                 = 0x00000800,   /**<Mask for the status Heart beat mode enabled.**/
        mDrmStatusMaskAsynchronousMeteringReady            = 0x00001000,   /**<Mask for the status Asynchronous metering ready.**/
        mDrmStatusMaskLicenseTimerSampleReady              = 0x00002000,   /**<Mask for the status License Timer sample ready.**/
        mDrmStatusMaskLicenseTimerCountEmpty               = 0x00004000,   /**<Mask for the status License Timer Count empty.**/
        mDrmStatusMaskSessionRunning                       = 0x00008000,   /**<Mask for the status Session Running.**/
        mDrmStatusMaskActivationCodesTransmitted           = 0x00010000,   /**<Mask for the status Activation Code Transmitted.**/
        mDrmStatusMaskLicenseNodeLock                      = 0x00020000,   /**<Mask for the status License Node Lock.**/
        mDrmStatusMaskLicenseMetering                      = 0x00040000,   /**<Mask for the status License Metering.**/
        mDrmStatusMaskLicenseTimerLoadedNumber             = 0x00180000,   /**<Mask for the status License Timer Loaded Number.**/
        mDrmStatusMaskSecurityAlert                        = 0x00200000,   /**<Mask for the status Security Alert.**/
        mDrmStatusMaskLicenseTimerInitSemaphoreAcknowledge = 0x00400000,   /**<Mask for the status License Timer Init Semaphore Acknowledge.**/
        mDrmStatusMaskIpActivatorNumber                    = 0xFF800000    /**<Mask for the status IP Activator Number.**/
      } tDrmStatusRegisterMaskEnumValues;

      /**
      *   \enum  tDrmErrorRegisterEnumValues
      *   \brief Enumeration for error codes.
      **/
      typedef enum tDrmErrorRegisterEnumValues {
        mDrmErrorNotReady                                   = 0xFF,  /**<Error code for operation not ready.**/
        mDrmErrorNoError                                    = 0x00,  /**<Error code for succesful operation.**/
        mDrmErrorBusReadAuthenticatorDrmVersionTimeOutError = 0x01,  /**<Error code for timeout during drm bus read authenticator version.**/
        mDrmErrorAuthenticatorDrmVersionError               = 0x02,  /**<Error code for authenticator version mismatch.**/
        mDrmErrorDnaAuthenticationError                     = 0x03,  /**<Error code for authenticator authentication failure.**/
        mDrmErrorBusWriteAuthenticatorCommandTimeOutError   = 0x04,  /**<Error code for timeout during drm bus write authenticator command.**/
        mDrmErrorBusReadAuthenticatorStatusTimeOutError     = 0x05,  /**<Error code for timeout during drm bus read authenticator status.**/
        mDrmErrorBusWriteAuthenticatorChallengeTimeOutError = 0x06,  /**<Error code for timeout during drm bus write authenticator challenge.**/
        mDrmErrorBusReadAuthenticatorResponseTimeOutError   = 0x07,  /**<Error code for timeout during drm bus read authenticator response.**/
        mDrmErrorBusReadAuthenticatorDnaTimeOutError        = 0x08,  /**<Error code for timeout during drm bus read authenticator dna.**/
        mDrmErrorBusReadActivatorDrmVersionTimeOutError     = 0x09,  /**<Error code for timeout during drm bus read activator version.**/
        mDrmErrorActivatorDrmVersionError                   = 0x0A,  /**<Error code for activator version mismatch.**/
        mDrmErrorLicenseHeaderCheckError                    = 0x0B,  /**<Error code for license header mismatch.**/
        mDrmErrorLicenseDrmVersionError                     = 0x0C,  /**<Error code for license version mismatch.**/
        mDrmErrorLicenseDnaDeltaError                       = 0x0D,  /**<Error code for license dna mismatch.**/
        mDrmErrorLicenseMacCheckError                       = 0x0E,  /**<Error code for license mac mismatch.**/
        mDrmErrorBusWriteActivatorCommandTimeOutError       = 0x0F,  /**<Error code for timeout during drm bus write activator command.**/
        mDrmErrorBusReadActivatorStatusTimeOutError         = 0x10,  /**<Error code for timeout during drm bus read activator status.**/
        mDrmErrorBusReadActivatorChallengeTimeOutError      = 0x11,  /**<Error code for timeout during drm bus read activator challenge.**/
        mDrmErrorBusWriteActivatorResponseTimeOutError      = 0x12,  /**<Error code for timeout during drm bus write activator response.**/
        mDrmErrorBusReadInterruptTimeOutError               = 0x13,  /**<Error code for timeout during drm bus read interupt.**/
        mDrmErrorBusReadExpectedStatusError                 = 0x14   /**<Error code for drm bus read unexpected status.**/
      } tDrmErrorRegisterEnumValues;

      /**
      *   \enum  tDrmErrorRegisterBytePositionEnumValues
      *   \brief Enumeration for error byte positions.
      **/
      typedef enum tDrmErrorRegisterBytePositionEnumValues {
        mDrmActivationErrorPosition       = 0,  /**<Error register position for Activation.**/
        mDrmDnaExtractErrorPosition       = 8,  /**<Error register position for DNA Extraction.**/
        mDrmVlnvExtractErrorPosition      = 16, /**<Error register position for VLNV Extraction.**/
        mDrmLicenseTimerLoadErrorPosition = 24  /**<Error register position for License Timer Load.**/
      } tDrmErrorRegisterBytePositionEnumValues;

      /**
      *   \enum  tDrmErrorRegisterByteMaskEnumValues
      *   \brief Enumeration for error byte masks.
      **/
      typedef enum tDrmErrorRegisterByteMaskEnumValues {
        mDrmActivationErrorMask       = 0x000000FF, /**<Error register mask for Activation.**/
        mDrmDnaExtractErrorMask       = 0x0000FF00, /**<Error register mask for DNA Extraction.**/
        mDrmVlnvExtractErrorMask      = 0x00FF0000, /**<Error register mask for VLNV Extraction.**/
        mDrmLicenseTimerLoadErrorMask = 0xFF000000  /**<Error register mask for License Timer Load.**/
      } tDrmErrorRegisterByteMaskEnumValues;

      /**
      *   \enum  tDrmMailBoxRegisterEnumValues
      *   \brief Enumeration for mailbox register bit positions.
      **/
      typedef enum tDrmMailBoxRegisterEnumValues {
        mDrmMailboxFileReadWriteMailboxWordNumberLsb = 0,   /**<LSB position of the read write mailbox word number bit.**/
        mDrmMailboxFileReadWriteMailboxWordNumberMsb = 15,  /**<MSB position of the read write mailbox word number bit.**/
        mDrmMailboxFileReadOnlyMailboxWordNumberLsb  = 16,  /**<LSB position of the read only mailbox word number bit.**/
        mDrmMailboxFileReadOnlyMailboxWordNumberMsb  = 31   /**<MSB position of the read only mailbox word number bit.**/
      } tDrmMailBoxRegisterEnumValues;

      /**
      *   \enum  tDrmMailBoxRegisterMaskEnumValues
      *   \brief Enumeration for mailbox register bit masks.
      **/
      typedef enum tDrmMailBoxRegisterMaskEnumValues {
        mDrmMailboxFileMaskReadWriteMailboxWordNumber = 0x0000FFFF,   /**<Mask for the read write mailbox word number.**/
        mDrmMailboxFileMaskReadOnlyMailboxWordNumber  = 0xFFFF0000    /**<Mask for the read only mailbox word number.**/
      } tDrmMailBoxRegisterMaskEnumValues;

      /**
      *   \struct tDrmError
      *   \brief  Structure containing the error code enumeration and the error message.
      **/
      typedef struct tDrmErrorRegisterMessages {
        tDrmErrorRegisterEnumValues mDrmErrorCode;    /**<Error code value.**/
        std::string                 mDrmErrorMessage; /**<Error message value.**/
      } tDrmErrorRegisterMessages;

      const unsigned int mDrmErrorRegisterMessagesArraySize ;
      const tDrmErrorRegisterMessages mDrmErrorRegisterMessagesArray[DRM_CONTROLLER_V7_0_0_NUMBER_OF_ERROR_CODES];

  }; // class DrmControllerRegistersStrategy_v7_0_0

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_REGISTERS_STRATEGY_V7_0_0_HPP__
