/**
*  \file      DrmControllerRegisters.hpp
*  \version   3.0.0.1
*  \date      September 2018
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

#include <HAL/DrmControllerTypes.hpp>
#include <DrmControllerCommon.hpp>

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
    public :

      /** DrmControllerRegisters
      *   \brief Class constructor.
      *   \param[in] f_read_reg32 function pointer to read 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
      *   \param[in] f_write_reg32 function pointer to write 32 bits register.
      *              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
      **/
      DrmControllerRegisters(t_drmReadRegisterFunction f_read_reg32,
                             t_drmWriteRegisterFunction f_write_reg32);

      /** ~DrmControllerRegisters
      *   \brief Class destructor.
      **/
      virtual ~DrmControllerRegisters();

      /** readPageRegister
      *   \brief Read and get the value of the page register from the hardware.
      *   This method will access to the system bus to read the page register.
      *   \param[out] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readPageRegister(unsigned int &page) const {
        // read page register at page index
        return f_read_reg32(DRM_CONTROLLER_PAGE_REGISTER_NAME, page);
      }

      /** readPageRegister
      *   \brief Read and get the value of the page register from the hardware.
      *   This method will access to the system bus to read the page register.
      *   \param[out] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readPageRegister(t_drmPageRegisterEnumValues &page) const {
        // read page register
        unsigned int readPage;
        unsigned int errorCode = DrmControllerRegisters::readPageRegister(readPage);
        page = (t_drmPageRegisterEnumValues)readPage;
        return errorCode;
      }

      /** writePageRegister
      *   \brief Write the value of the page register into the hardware.
      *   This method will access to the system bus to write the page register.
      *   \param[in] page is the value of the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writePageRegister(const unsigned int &page) const {
        // write page register at page index
        return f_write_reg32(DRM_CONTROLLER_PAGE_REGISTER_NAME, page);
      }

      /** writePageRegister
      *   \brief Write the page register value.
      *   This method will access to the system bus to read and write the page.
      *   \param[in] page is the value of the page register to write.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writePageRegister(const t_drmPageRegisterEnumValues &page) const {
        // write page register
        return DrmControllerRegisters::writePageRegister((unsigned int)page);
      }

      /** writeRegistersPageRegister
      *   \brief Write the page register to select the registers page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeRegistersPageRegister() const {
        // write registers page
        return DrmControllerRegisters::writePageRegister(mDrmPageRegisters);
      }

      /** writeVlnvFilePageRegister
      *   \brief Write the page register to select the vlnv file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeVlnvFilePageRegister() const {
        // write vlnv file page
        return DrmControllerRegisters::writePageRegister(mDrmPageVlnvFile);
      }

      /** writeLicenseFilePageRegister
      *   \brief Write the page register to select the license file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeLicenseFilePageRegister() const {
        // write license file page
        return DrmControllerRegisters::writePageRegister(mDrmPageLicenseFile);
      }

      /** writeTraceFilePageRegister
      *   \brief Write the page register to select the trace file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeTraceFilePageRegister() const {
        // write trace file page
        return DrmControllerRegisters::writePageRegister(mDrmPageTraceFile);
      }

      /** writeMeteringFilePageRegister
      *   \brief Write the page register to select the metering file page.
      *   This method will access to the system bus to write into the page register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeMeteringFilePageRegister() const {
        // write metering file page
        return DrmControllerRegisters::writePageRegister(mDrmPageMeteringFile);
      }

      /** readCommandRegister
      *   \brief Read and get the value of the command register from the hardware.
      *   This method will access to the system bus to read the command register.
      *   \param[out] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readCommandRegister(unsigned int &command) const {
        // read register at command register index
        return DrmControllerRegisters::readRegisterAtIndex(DRM_CONTROLLER_COMMAND_REGISTER_INDEX, command);
      }

      /** readCommandRegister
      *   \brief Read and get the value of the command register from the hardware.
      *   This method will access to the system bus to read the command register.
      *   \param[out] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readCommandRegister(t_drmCommandRegisterEnumValues &command) const {
        // read command register
        unsigned int readCommand;
        unsigned int errorCode = DrmControllerRegisters::readCommandRegister(readCommand);
        command = (t_drmCommandRegisterEnumValues)readCommand;
        return errorCode;
      }

      /** writeCommandRegister
      *   \brief Write the value of the command register into the hardware.
      *   This method will access to the system bus to write the command register.
      *   \param[in] command is the value of the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeCommandRegister(const unsigned int &command) const {
        // write register at command register index
        return DrmControllerRegisters::writeRegisterAtIndex(DRM_CONTROLLER_COMMAND_REGISTER_INDEX, command);
      }

      /** writeCommandRegister
      *   \brief Write the command register value.
      *   This method will access to the system bus to read and write the command.
      *   \param[in] command is the value of the command register to write.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeCommandRegister(const t_drmCommandRegisterEnumValues &command) const {
        // write command register
        return DrmControllerRegisters::writeCommandRegister((unsigned int)command);
      }

      /** writeNopCommandRegister
      *   \brief Write the command register to the NOP Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeNopCommandRegister() const {
        // write nop command
        return DrmControllerRegisters::writeCommandRegister(mDrmCommandNop);
      }

      /** writeDnaExtractCommandRegister
      *   \brief Write the command register to the DNA Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeDnaExtractCommandRegister() const {
        // write extract dna command
        return DrmControllerRegisters::writeCommandRegister(mDrmCommandExtractDna);
      }

      /** writeVlnvExtractCommandRegister
      *   \brief Write the command register to the VLNV Extract Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeVlnvExtractCommandRegister() const {
        // write vlnv extract command
        return DrmControllerRegisters::writeCommandRegister(mDrmCommandExtractVlnv);
      }

      /** writeActivateCommandRegister
      *   \brief Write the command register to the Activate Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeActivateCommandRegister() const {
        // write activation command
        return DrmControllerRegisters::writeCommandRegister(mDrmCommandActivate);
      }

      /** writeEndSessionMeteringExtractCommandRegister
      *   \brief Write the command register to the end session extract metering Command.
      *   This method will access to the system bus to write into the command register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeEndSessionMeteringExtractCommandRegister() const {
        // write end session extract metering command
        return DrmControllerRegisters::writeCommandRegister(mDrmCommandEndSessionExtractMetering);
      }

      /** readLicenseStartAddressRegister
      *   \brief Read the license start address register.
      *   This method will access to the system bus to read the license start address.
      *   \param[out] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLicenseStartAddressRegister(std::vector<unsigned int> &licenseStartAddress) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_LIC_START_ADR_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_LIC_START_ADDRESS_WORD_NBR,
                                                                     licenseStartAddress);
      }

      /** writeLicenseStartAddressRegister
      *   \brief Write the license start address register.
      *   This method will access to the system bus to write the license start address.
      *   \param[in] licenseStartAddress is a list of binary values for the license start address register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeLicenseStartAddressRegister(const std::vector<unsigned int> &licenseStartAddress) const {
        // write register list from index
        return DrmControllerRegisters::writeRegisterListFromIndex(DRM_CONTROLLER_LIC_START_ADR_REGISTER_START_INDEX,
                                                                      DRM_CONTROLLER_LIC_START_ADDRESS_WORD_NBR,
                                                                      licenseStartAddress);
      }

      /** readLicenseTimerInitRegister
      *   \brief Read the license timer register.
      *   This method will access to the system bus to read the license timer.
      *   \param[out] licenseTimerInit is a list of binary values for the license timer register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLicenseTimerInitRegister(std::vector<unsigned int> &licenseTimerInit) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_LIC_TIMER_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_LIC_TIMER_WORD_NBR,
                                                                     licenseTimerInit);
      }

      /** writeLicenseTimerInitRegister
      *   \brief Write the license start address register.
      *   This method will access to the system bus to write the license timer.
      *   \param[in] licenseTimerInit is a list of binary values for the license timer register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeLicenseTimerInitRegister(const std::vector<unsigned int> &licenseTimerInit) const {
        // write register list from index
        return DrmControllerRegisters::writeRegisterListFromIndex(DRM_CONTROLLER_LIC_TIMER_REGISTER_START_INDEX,
                                                                      DRM_CONTROLLER_LIC_TIMER_WORD_NBR,
                                                                      licenseTimerInit);
      }

      /** readStatusRegister
      *   \brief Read the status register.
      *   This method will access to the system bus to read the status register.
      *   \param[out] status is the binary value of the status register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readStatusRegister(unsigned int &status) const {
        // read register at index
        return DrmControllerRegisters::readRegisterAtIndex(DRM_CONTROLLER_STATUS_REGISTER_INDEX, status);
      }

      /** readStatusRegister
      *   \brief Read the value of a specific status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[in] bitPosition is the position of the status bit.
      *   \param[in] mask is the mask of the status bit.
      *   \param[out] value is the value of the status bit.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readStatusRegister(const t_drmStatusRegisterEnumValues &bitPosition,
                                             const t_drmStatusRegisterMaskEnumValues &mask,
                                             bool &value) const {
        // status value
        unsigned int status;
        // read status register
        unsigned int errorCode = DrmControllerRegisters::readStatusRegister(status);
        // check result
        if (errorCode != mDrmApi_NO_ERROR)
          // return error result
          return errorCode;
        // check status bit
        if (((status & mask) >> bitPosition) == 1) {
          value = true;
        }
        else {
          value = false;
        }
        // return error result
        return errorCode;
      }

      /** readDnaReadyStatusRegister
      *   \brief Read the status register and get the dna ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] dnaReady is the value of the status bit DNA Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readDnaReadyStatusRegister(bool &dnaReady) const {
        // read status bit register for dna ready
        return DrmControllerRegisters::readStatusRegister(mDrmStatusDnaReady,
                                                              mDrmStatusMaskDnaReady,
                                                              dnaReady);
      }

      /** readVlnvReadyStatusRegister
      *   \brief Read the status register and get the vlnv ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] vlnvReady is the value of the status bit VLNV Ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readVlnvReadyStatusRegister(bool &vlnvReady) const {
        // read status bit register for vlnv ready
        return DrmControllerRegisters::readStatusRegister(mDrmStatusVlnvReady,
                                                              mDrmStatusMaskVlnvReady,
                                                              vlnvReady);
      }

      /** readActivationDoneStatusRegister
      *   \brief Read the status register and get the activation done status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] activationDone is the value of the status bit Activation Done.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readActivationDoneStatusRegister(bool &activationDone) const {
        // read status bit register for activation done
        return DrmControllerRegisters::readStatusRegister(mDrmStatusActivationDone,
                                                              mDrmStatusMaskActivationDone,
                                                              activationDone);
      }

      /** readAutonomousControllerEnabledStatusRegister
      *   \brief Read the status register and get the autonomous controller enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoEnabled is the value of the status bit autonomous controller enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readAutonomousControllerEnabledStatusRegister(bool &autoEnabled) const {
        // read status bit register for auto controller enabled
        return DrmControllerRegisters::readStatusRegister(mDrmStatusAutoEnabled,
                                                              mDrmStatusMaskAutoEnabled,
                                                              autoEnabled);
      }

      /** readAutonomousControllerBusyStatusRegister
      *   \brief Read the status register and get the autonomous controller busy status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] autoBusy is the value of the status bit autonomous controller busy.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readAutonomousControllerBusyStatusRegister(bool &autoBusy) const {
        // read status bit register for auto controller busy
        return DrmControllerRegisters::readStatusRegister(mDrmStatusAutoBusy,
                                                              mDrmStatusMaskAutoBusy,
                                                              autoBusy);
      }

      /** readMeteringEnabledStatusRegister
      *   \brief Read the status register and get the metering enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringEnabled is the value of the status bit metering enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readMeteringEnabledStatusRegister(bool &meteringEnabled) const {
        // read status bit register for metering enabled
        return DrmControllerRegisters::readStatusRegister(mDrmStatusMeteringEnabled,
                                                              mDrmStatusMaskMeteringEnabled,
                                                              meteringEnabled);
      }

      /** readMeteringReadyStatusRegister
      *   \brief Read the status register and get the metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] meteringReady is the value of the status bit metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readMeteringReadyStatusRegister(bool &meteringReady) const {
        // read status bit register for metering ready
        return DrmControllerRegisters::readStatusRegister(mDrmStatusMeteringReady,
                                                              mDrmStatusMaskMeteringReady,
                                                              meteringReady);
      }

      /** readSaasChallengeReadyStatusRegister
      *   \brief Read the status register and get the saas challenge ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readSaasChallengeReadyStatusRegister(bool &saasChallengeReady) const {
        // read status bit register for metering ready
        return DrmControllerRegisters::readStatusRegister(mDrmStatusSaasChallengeReady,
                                                              mDrmStatusMaskSaasChallengeReady,
                                                              saasChallengeReady);
      }

      /** readLicenseTimerEnabledStatusRegister
      *   \brief Read the status register and get the license timer enabled status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const {
        // read status bit register for license timer enabled
        return DrmControllerRegisters::readStatusRegister(mDrmStatusLicenseTimerEnabled,
                                                              mDrmStatusMaskLicenseTimerEnabled,
                                                              licenseTimerEnabled);
      }

      /** readLicenseTimerInitLoadedStatusRegister
      *   \brief Read the status register and get the license timer init loaded status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init load.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLicenseTimerInitLoadedStatusRegister(bool &licenseTimerInitLoaded) const {
        // read status bit register for license timer load done
        return DrmControllerRegisters::readStatusRegister(mDrmStatusLicenseTimerInitLoaded,
                                                              mDrmStatusMaskLicenseTimerInitLoaded,
                                                              licenseTimerInitLoaded);
      }

      /** readEndSessionMeteringReadyStatusRegister
      *   \brief Read the status register and get the end session metering ready status bit.
      *   This method will access to the system bus to read the status register.
      *   \param[out] endSessionMeteringReady is the value of the status bit end session metering ready.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readEndSessionMeteringReadyStatusRegister(bool &endSessionMeteringReady) const {
        // read status bit register for license timer load done
        return DrmControllerRegisters::readStatusRegister(mDrmStatusEndSessionMeteringReady,
                                                              mDrmStatusMaskEndSessionMeteringReady,
                                                              endSessionMeteringReady);
      }

      /** readNumberOfDetectedIpsStatusRegister
      *   \brief Read the status register and get the number of detected IPs.
      *   This method will access to the system bus to read the status register.
      *   \param[out] numberOfDetectedIps is the number of detected ips from the status.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readNumberOfDetectedIpsStatusRegister(unsigned int &numberOfDetectedIps) const {
        // status value
        unsigned int status;
        // read status register
        unsigned int errorCode = DrmControllerRegisters::readStatusRegister(status);
        // check result
        if (errorCode != mDrmApi_NO_ERROR)
          // return error result
          return errorCode;
        // get number of detected ips
        numberOfDetectedIps = (unsigned int)((status & mDrmStatusMaskIpNmbr) >> mDrmStatusIpNmbrLsb);
        // return error result
        return errorCode;
      }

      /** readErrorRegister
      *   \brief Read the error register.
      *   This method will access to the system bus to read the error register.
      *   \param[out] error is the binary value of the error register.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readErrorRegister(unsigned int &error) const {
        // read register at index
        return DrmControllerRegisters::readRegisterAtIndex(DRM_CONTROLLER_ERROR_REGISTER_INDEX, error);
      }

      /** readErrorRegister
      *   \brief Get the value of a specific error byte.
      *   This method will access to the system bus to read the error register.
      *   \param[in] position is the position of the error byte.
      *   \param[in] mask is the mask of the error byte.
      *   \param[out] error is the value of the error byte.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readErrorRegister(const t_drmErrorRegisterBytePositionEnumValues &position,
                                            const t_drmErrorRegisterByteMaskEnumValues &mask,
                                            unsigned char &error) const {
        // error value
        unsigned int readError;
        // read error register
        unsigned int errorCode = DrmControllerRegisters::readErrorRegister(readError);
        // check result
        if (errorCode != mDrmApi_NO_ERROR)
          // return error result
          return errorCode;
        error = (unsigned char)((readError & mask) >> position*DRM_CONTROLLER_SINGLE_OPERATION_ERROR_REGISTER_SIZE);
        // return error result
        return errorCode;
      }

      /** readExtractDnaErrorRegister
      *   \brief Read the error register and get the error code related to dna extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] dnaExtractError is the error code related to dna extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readExtractDnaErrorRegister(unsigned char &dnaExtractError) const {
        // read dna extract error
        return DrmControllerRegisters::readErrorRegister(mDrmDnaExtractErrorPosition,
                                                             mDrmDnaExtractErrorMask,
                                                             dnaExtractError);
      }

      /** readExtractVlnvErrorRegister
      *   \brief Read the error register and get the error code related to vlnv extraction.
      *   This method will access to the system bus to read the error register.
      *   \param[out] vlnvExtractError is the error code related to vlnv extraction.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readExtractVlnvErrorRegister(unsigned char &vlnvExtractError) const {
        // read vlnv extract error
        return DrmControllerRegisters::readErrorRegister(mDrmVlnvExtractErrorPosition,
                                                             mDrmVlnvExtractErrorMask,
                                                             vlnvExtractError);
      }

      /** readActivationErrorRegister
      *   \brief Read the error register and get the error code related to activation.
      *   This method will access to the system bus to read the error register.
      *   \param[out] activationError is the error code related to activation.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readActivationErrorRegister(unsigned char &activationError) const {
        // read activation error
        return DrmControllerRegisters::readErrorRegister(mDrmActivationErrorPosition,
                                                             mDrmActivationErrorMask,
                                                             activationError);
      }

      /** readLicenseTimerLoadErrorRegister
      *   \brief Read the error register and get the error code related to license timer loading.
      *   This method will access to the system bus to read the error register.
      *   \param[out] licenseTimerLoadError is the error code related to license timer loading.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLicenseTimerLoadErrorRegister(unsigned char &licenseTimerLoadError) const {
        // read license timer load error
        return DrmControllerRegisters::readErrorRegister(mDrmLicenseTimerLoadErrorPosition,
                                                             mDrmLicenseTimerLoadErrorMask,
                                                             licenseTimerLoadError);
      }

      /** readDrmVersionRegister
      *   \brief Read the drm version register and get the value.
      *   This method will access to the system bus to read the drm version register.
      *   \param[out] drmVersion is the drm version value.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readDrmVersionRegister(unsigned int &drmVersion) const {
        // read register at drm version index
        return DrmControllerRegisters::readRegisterAtIndex(DRM_CONTROLLER_VERSION_REGISTER_INDEX, drmVersion);
      }

      /** readDnaRegister
      *   \brief Read the dna register and get the value.
      *   This method will access to the system bus to read the dna register.
      *   \param[out] dna is the dna value.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readDnaRegister(std::vector<unsigned int> &dna) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_DNA_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_DNA_WORD_NBR,
                                                                     dna);
      }

      /** readSaasChallengeRegister
      *   \brief Read the Saas Challenge register and get the value.
      *   This method will access to the system bus to read the Saas Challenge register.
      *   \param[out] saasChallenge is the saas challenge value.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readSaasChallengeRegister(std::vector<unsigned int> &saasChallenge) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_SAAS_CHALLENGE_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_SAAS_CHALLENGE_WORD_NBR,
                                                                     saasChallenge);
      }

      /** readLogsRegister
      *   \brief Read the logs register and get the value.
      *   This method will access to the system bus to read the logs register.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] logs is the logs value.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLogsRegister(const unsigned int &numberOfIps,
                                           std::vector<unsigned int> &logs) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_LOGS_REGISTER_START_INDEX,
                                                                     DrmControllerRegisters::getNumberOfWords(numberOfIps),
                                                                     logs);
      }

      /** readVlnvFileRegister
      *   \brief Read the vlnv file and get the value.
      *   This method will access to the system bus to read the vlnv file.
      *   The vlnv file will contains numberOfIps+1 words. The first one
      *   is dedicated to the drm controller, the others correspond for
      *   the IPs connected to the drm controller.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] vlnvFile is the vlnv file.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readVlnvFileRegister(const unsigned int &numberOfIps,
                                               std::vector<unsigned int> &vlnvFile) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_FILES_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_VLNV_WORD_WORD_NBR*(numberOfIps+1),
                                                                     vlnvFile);
      }

      /** readLicenseFileRegister
      *   \brief Read the license file and get the value.
      *   This method will access to the system bus to read the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[out] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readLicenseFileRegister(const unsigned int &licenseFileSize,
                                                  std::vector<unsigned int> &licenseFile) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_FILES_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_LICENSE_WORD_WORD_NBR*licenseFileSize,
                                                                     licenseFile);
      }

      /** writeLicenseFile
      *   \brief Write the license file.
      *   This method will access to the system bus to write the license file.
      *   \param[in] licenseFileSize is the number of 128 bits words to read.
      *   \param[in] licenseFile is the license file.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeLicenseFileRegister(const unsigned int &licenseFileSize,
                                                   const std::vector<unsigned int> &licenseFile) const {
        // write register list from index
        return DrmControllerRegisters::writeRegisterListFromIndex(DRM_CONTROLLER_FILES_REGISTER_START_INDEX,
                                                                      DRM_CONTROLLER_LICENSE_WORD_WORD_NBR*licenseFileSize,
                                                                      licenseFile);
      }

      /** readTraceFileRegister
      *   \brief Read the trace file and get the value.
      *   This method will access to the system bus to read the trace file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] traceFile is the trace file.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readTraceFileRegister(const unsigned int &numberOfIps,
                                                std::vector<unsigned int> &traceFile) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_FILES_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_TRACE_WORD_WORD_NBR*DRM_CONTROLLER_NUMBER_OF_TRACES_PER_IP*numberOfIps,
                                                                     traceFile);
      }

      /** readMeteringFileRegister
      *   \brief Read the metering file and get the value.
      *   This method will access to the system bus to read the metering file.
      *   \param[in] numberOfIps is the total number of IPs.
      *   \param[out] meteringFile is the metering file.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readMeteringFileRegister(const unsigned int &numberOfIps,
                                                  std::vector<unsigned int> &meteringFile) const {
        // read register list from index
        return DrmControllerRegisters::readRegisterListFromIndex(DRM_CONTROLLER_FILES_REGISTER_START_INDEX,
                                                                     DRM_CONTROLLER_METERING_WORD_WORD_NBR*(numberOfIps+2),
                                                                     meteringFile);
      }

    // protected members, functions ...
    protected :

    // private members, functions ...
    private :
      t_drmReadRegisterFunction f_read_reg32;
      t_drmWriteRegisterFunction f_write_reg32;

      /** readRegisterListFromIndex
      *   \brief Read a list of register starting from a specified index.
      *   This method will access to the system bus to read several registers.
      *   \param[in]  registerWordStartIndex is the start index of the first register word to read.
      *   \param[in]  numberOfRegisterWords is the number of register words to read.
      *   \param[out] registerList is the value of registers read.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readRegisterListFromIndex(const unsigned int &registerWordStartIndex,
                                                    const unsigned int &numberOfRegisterWords,
                                                    std::vector<unsigned int> &registerList) const {
        // clear list
        registerList.clear();
        // register word
        unsigned int registerWord(0);
        // error code
        unsigned int errorCode(mDrmApi_NO_ERROR);
        // read each element
        for (unsigned int ii = 0; ii < numberOfRegisterWords; ii++) {
          // read saas challenge word
          errorCode = DrmControllerRegisters::readRegisterAtIndex(registerWordStartIndex+ii, registerWord);
          // check result
          if (errorCode != mDrmApi_NO_ERROR)
            // return error result
            return errorCode;
          // push element
          registerList.push_back(registerWord);
        }
        // return error result
        return errorCode;
      }

      /** readRegisterAtIndex
      *   \brief Read a register at a specified index.
      *   This method will access to the system bus to read a register.
      *   \param[in]  registerWordIndex is the index of the register word to read.
      *   \param[out] registerValue is the value of the register read.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int readRegisterAtIndex(const unsigned int &registerWordIndex,
                                              unsigned int &registerValue) const {
        // read register at index
        return f_read_reg32(getRegisterNameFromIndex(registerWordIndex), registerValue);
      }

      /** writeRegisterListFromIndex
      *   \brief Write a list of register starting from a specified index.
      *   This method will access to the system bus to write several registers.
      *   \param[in]  registerWordStartIndex is the start index of the first register word to write.
      *   \param[in]  numberOfRegisterWords is the number of register words to write.
      *   \param[out] registerList is the value of registers write.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeRegisterListFromIndex(const unsigned int &registerWordStartIndex,
                                                     const unsigned int &numberOfRegisterWords,
                                                     const std::vector<unsigned int> &registerList) const {
        // register word index
        unsigned int registerWordIndex = registerWordStartIndex;
        // error code
        unsigned int errorCode(mDrmApi_NO_ERROR);
        for (std::vector<unsigned int>::const_iterator it = registerList.cbegin(); it != registerList.cend(); it++) {
          // write register word
          errorCode = DrmControllerRegisters::writeRegisterAtIndex(registerWordIndex++, *it);
          // check result
          if (errorCode != mDrmApi_NO_ERROR)
            // return error result
            return errorCode;
        }
        return errorCode;
      }

      /** writeRegisterAtIndex
      *   \brief Write a register at a specified index.
      *   This method will access to the system bus to write a register.
      *   \param[in]  registerWordIndex is the index of the register word to write.
      *   \param[out] registerValue is the value of the register write.
      *   \return Returns mDrmApi_NO_ERROR if no error, errors from read/write register functions otherwize.
      **/
      inline unsigned int writeRegisterAtIndex(const unsigned int &registerWordIndex,
                                               const unsigned int &registerValue) const {
        // write register at index
        return f_write_reg32(getRegisterNameFromIndex(registerWordIndex), registerValue);
      }

      /** getRegisterNameFromIndex
      *   \brief Get the register name from index
      *   \param[in] index is the register index.
      *   \return Returns the name of the register at the specified index.
      **/
      inline const std::string getRegisterNameFromIndex(const unsigned int &index) const {
        // create a string stream
        std::ostringstream stringStream;
        // insert indexed register name
        stringStream << DRM_CONTROLLER_INDEXED_REGISTER_NAME;
        // insert index
        stringStream << index;
        // return the string from the stream
        return stringStream.str();
      }

      /** getNumberOfWords
      *   \brief Get the number of words used by a register.
      *   \param[in] registerSize is the size of the register in bits.
      *   \return Returns the number of words used for the given register size.
      **/
      inline unsigned int getNumberOfWords(const unsigned int& registerSize) const {
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

  }; // class DrmControllerRegisters

} // namespace DrmControllerLibrary

#endif // __DRM_CONTROLLER_REGISTERS_HPP__
