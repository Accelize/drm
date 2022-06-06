/**
*  \file      DrmControllerRegisters.cpp
*  \version   8.0.0.0
*  \date      March 2022
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

#include <HAL/DrmControllerRegisters.hpp>

// namespace usage
using namespace DrmControllerLibrary;

/************************************************************/
/**                  PUBLIC MEMBER FUNCTIONS               **/
/************************************************************/

/** DrmControllerRegisters
*   \brief Class constructor.
*   \param[in] readRegisterFunction function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] writeRegisterFunction function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
**/
DrmControllerRegisters::DrmControllerRegisters(tDrmReadRegisterFunction readRegisterFunction, tDrmWriteRegisterFunction writeRegisterFunction)
 : mDrmControllerRegistersStrategyInterface(selectRegistersStrategy(readRegisterFunction, writeRegisterFunction))
{}

/** ~DrmControllerRegisters
*   \brief Class destructor.
**/
DrmControllerRegisters::~DrmControllerRegisters() {
  if (mDrmControllerRegistersStrategyInterface != NULL) {
    delete mDrmControllerRegistersStrategyInterface;
    mDrmControllerRegistersStrategyInterface = NULL;
  }
}

/** readLicenseStartAddressRegister
*   \brief Read the license start address register.
*   This method will access to the system bus to read the license start address.
*   \param[out] licenseStartAddress is a list of binary values for the license start address register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseStartAddressRegister(std::vector<unsigned int> &licenseStartAddress) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseStartAddressRegister(licenseStartAddress);
}

/** readLicenseStartAddressRegister
*   \brief Read the value of the license start address.
*   This method will access to the system bus to read the license start address register.
*   \param[in] licenseStartAddress is the license start address value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseStartAddressRegister(std::string &licenseStartAddress) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseStartAddressRegister(licenseStartAddress);
}

/** readLicenseTimerInitRegister
*   \brief Read the license timer register.
*   This method will access to the system bus to read the license timer.
*   \param[out] licenseTimerInit is a list of binary values for the license timer register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerInitRegister(std::vector<unsigned int> &licenseTimerInit) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerInitRegister(licenseTimerInit);
}

/** readLicenseTimerInitRegister
*   \brief Read the value of the license timer.
*   This method will access to the system bus to read the license timer register.
*   \param[out] licenseTimerInit is the license timer value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerInitRegister(std::string &licenseTimerInit) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerInitRegister(licenseTimerInit);
}

/** writeLicenseTimerInitRegister
*   \brief Write the license start address register.
*   This method will access to the system bus to write the license timer.
*   \param[in] licenseTimerInit is a list of binary values for the license timer register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseTimerInitRegister(const std::vector<unsigned int> &licenseTimerInit) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseTimerInitRegister(licenseTimerInit);
}

/** writeLicenseTimerInitRegister
*   \brief Write the value of the license timer.
*   This method will access to the system bus to write the license timer register.
*   \param[in] licenseTimerInit is the license timer value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseTimerInitRegister(const std::string &licenseTimerInit) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseTimerInitRegister(licenseTimerInit);
}

/** readDnaReadyStatusRegister
*   \brief Read the status register and get the dna ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] dnaReady is the value of the status bit DNA Ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readDnaReadyStatusRegister(bool &dnaReady) const {
  return mDrmControllerRegistersStrategyInterface->readDnaReadyStatusRegister(dnaReady);
}

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
unsigned int DrmControllerRegisters::waitDnaReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitDnaReadyStatusRegister(timeout, expected, actual);
}

/** readVlnvReadyStatusRegister
*   \brief Read the status register and get the vlnv ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] vlnvReady is the value of the status bit VLNV Ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readVlnvReadyStatusRegister(bool &vlnvReady) const {
  return mDrmControllerRegistersStrategyInterface->readVlnvReadyStatusRegister(vlnvReady);
}

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
unsigned int DrmControllerRegisters::waitVlnvReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitVlnvReadyStatusRegister(timeout, expected, actual);
}

/** readActivationDoneStatusRegister
*   \brief Read the status register and get the activation done status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] activationDone is the value of the status bit Activation Done.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readActivationDoneStatusRegister(bool &activationDone) const {
  return mDrmControllerRegistersStrategyInterface->readActivationDoneStatusRegister(activationDone);
}

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
unsigned int DrmControllerRegisters::waitActivationDoneStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitActivationDoneStatusRegister(timeout, expected, actual);
}

/** readAutonomousControllerEnabledStatusRegister
*   \brief Read the status register and get the autonomous controller enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] autoEnabled is the value of the status bit autonomous controller enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readAutonomousControllerEnabledStatusRegister(bool &autoEnabled) const {
  return mDrmControllerRegistersStrategyInterface->readAutonomousControllerEnabledStatusRegister(autoEnabled);
}

/** readAutonomousControllerBusyStatusRegister
*   \brief Read the status register and get the autonomous controller busy status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] autoBusy is the value of the status bit autonomous controller busy.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readAutonomousControllerBusyStatusRegister(bool &autoBusy) const {
  return mDrmControllerRegistersStrategyInterface->readAutonomousControllerBusyStatusRegister(autoBusy);
}

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
unsigned int DrmControllerRegisters::waitAutonomousControllerBusyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitAutonomousControllerBusyStatusRegister(timeout, expected, actual);
}

/** readMeteringEnabledStatusRegister
*   \brief Read the status register and get the metering enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readMeteringEnabledStatusRegister(bool &meteringEnabled) const {
  return mDrmControllerRegistersStrategyInterface->readMeteringEnabledStatusRegister(meteringEnabled);
}

/** checkMeteringEnabledStatusRegister
*   \brief Read the status register and check the metering enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] meteringEnabled is the value of the status bit metering enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_METERING_DISABLED_ERROR if the metering is disabled, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException if the metering is disabled. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::checkMeteringEnabledStatusRegister(bool &meteringEnabled) const {
  return mDrmControllerRegistersStrategyInterface->checkMeteringEnabledStatusRegister(meteringEnabled);
}

/** readMeteringReadyStatusRegister
*   \brief Read the status register and get the metering ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] meteringReady is the value of the status bit metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readMeteringReadyStatusRegister(bool &meteringReady) const {
  return mDrmControllerRegistersStrategyInterface->readMeteringReadyStatusRegister(meteringReady);
}

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
unsigned int DrmControllerRegisters::waitMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitMeteringReadyStatusRegister(timeout, expected, actual);
}

/** readSaasChallengeReadyStatusRegister
*   \brief Read the status register and get the saas challenge ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] saasChallengeReady is the value of the status bit saas challenge ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readSaasChallengeReadyStatusRegister(bool &saasChallengeReady) const {
  return mDrmControllerRegistersStrategyInterface->readSaasChallengeReadyStatusRegister(saasChallengeReady);
}

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
unsigned int DrmControllerRegisters::waitSaasChallengeReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitSaasChallengeReadyStatusRegister(timeout, expected, actual);
}

/** readLicenseTimerEnabledStatusRegister
*   \brief Read the status register and get the license timer enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
}

/** checkLicenseTimerEnabledStatusRegister
*   \brief Read the status register and check license timer enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerEnabled is the value of the status bit license timer enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, mDrmApi_LICENSE_TIMER_DISABLED_ERROR if the license timer is disabled, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
*   \throw DrmControllerFunctionalityDisabledException if the metering is disabled. DrmControllerFunctionalityDisabledException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::checkLicenseTimerEnabledStatusRegister(bool &licenseTimerEnabled) const {
  return mDrmControllerRegistersStrategyInterface->checkLicenseTimerEnabledStatusRegister(licenseTimerEnabled);
}

/** readLicenseTimerInitLoadedStatusRegister
*   \brief Read the status register and get the license timer init loaded status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerInitLoaded is the value of the status bit license timer init load.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerInitLoadedStatusRegister(bool &licenseTimerInitLoaded) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerInitLoadedStatusRegister(licenseTimerInitLoaded);
}

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
unsigned int DrmControllerRegisters::waitLicenseTimerInitLoadedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseTimerInitLoadedStatusRegister(timeout, expected, actual);
}

/** readEndSessionMeteringReadyStatusRegister
*   \brief Read the status register and get the end session metering ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] endSessionMeteringReady is the value of the status bit end session metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readEndSessionMeteringReadyStatusRegister(bool &endSessionMeteringReady) const {
  return mDrmControllerRegistersStrategyInterface->readEndSessionMeteringReadyStatusRegister(endSessionMeteringReady);
}

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
unsigned int DrmControllerRegisters::waitEndSessionMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitEndSessionMeteringReadyStatusRegister(timeout, expected, actual);
}

/** readHeartBeatModeEnabledStatusRegister
*   \brief Read the status register and get the heart beat mode enabled status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] heartBeatModeEnabled is the value of the status bit heart beat mode enabled.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readHeartBeatModeEnabledStatusRegister(bool &heartBeatModeEnabled) const {
  return mDrmControllerRegistersStrategyInterface->readHeartBeatModeEnabledStatusRegister(heartBeatModeEnabled);
}

/** readAsynchronousMeteringReadyStatusRegister
*   \brief Read the status register and get the asynchronous metering ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] asynchronousMeteringReady is the value of the status bit asynchronous metering ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readAsynchronousMeteringReadyStatusRegister(bool &asynchronousMeteringReady) const {
  return mDrmControllerRegistersStrategyInterface->readAsynchronousMeteringReadyStatusRegister(asynchronousMeteringReady);
}

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
unsigned int DrmControllerRegisters::waitAsynchronousMeteringReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitAsynchronousMeteringReadyStatusRegister(timeout, expected, actual);
}

/** readLicenseTimerSampleReadyStatusRegister
*   \brief Read the status register and get the license timer sample ready status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerSampleReady is the value of the status bit license timer sample ready.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerSampleReadyStatusRegister(bool &licenseTimerSampleReady) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerSampleReadyStatusRegister(licenseTimerSampleReady);
}

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
unsigned int DrmControllerRegisters::waitLicenseTimerSampleReadyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseTimerSampleReadyStatusRegister(timeout, expected, actual);
}

/** readLicenseTimerCountEmptyStatusRegister
*   \brief Read the status register and get the license timer count empty status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerCounterEmpty is the value of the status bit license timer count empty.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerCountEmptyStatusRegister(bool &licenseTimerCounterEmpty) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerCountEmptyStatusRegister(licenseTimerCounterEmpty);
}

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
unsigned int DrmControllerRegisters::waitLicenseTimerCountEmptyStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseTimerCountEmptyStatusRegister(timeout, expected, actual);
}

/** readSessionRunningStatusRegister
*   \brief Read the status register and get the session running status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] sessionRunning is the value of the status bit session running.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readSessionRunningStatusRegister(bool &sessionRunning) const {
  return mDrmControllerRegistersStrategyInterface->readSessionRunningStatusRegister(sessionRunning);
}

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
unsigned int DrmControllerRegisters::waitSessionRunningStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitSessionRunningStatusRegister(timeout, expected, actual);
}

/** readActivationCodesTransmittedStatusRegister
*   \brief Read the status register and get the activation codes transmitted status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] activationCodeTransmitted is the value of the status bit activation codes transmitted.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readActivationCodesTransmittedStatusRegister(bool &activationCodeTransmitted) const {
  return mDrmControllerRegistersStrategyInterface->readActivationCodesTransmittedStatusRegister(activationCodeTransmitted);
}

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
unsigned int DrmControllerRegisters::waitActivationCodesTransmittedStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitActivationCodesTransmittedStatusRegister(timeout, expected, actual);
}

/** readLicenseNodeLockStatusRegister
*   \brief Read the status register and get the license node lock status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseNodeLock is the value of the status bit license node lock.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseNodeLockStatusRegister(bool &licenseNodeLock) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseNodeLockStatusRegister(licenseNodeLock);
}

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
unsigned int DrmControllerRegisters::waitLicenseNodeLockStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseNodeLockStatusRegister(timeout, expected, actual);
}

/** readLicenseMeteringStatusRegister
*   \brief Read the status register and get the license metering status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseMetering is the value of the status bit license metering.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseMeteringStatusRegister(bool &licenseMetering) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseMeteringStatusRegister(licenseMetering);
}

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
unsigned int DrmControllerRegisters::waitLicenseMeteringStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseMeteringStatusRegister(timeout, expected, actual);
}

/** readSecurityAlertStatusRegister
*   \brief Read the status register and get the Security Alert status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] securityAlert is the value of the status bit Security Alert.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readSecurityAlertStatusRegister(bool &securityAlert) const {
  return mDrmControllerRegistersStrategyInterface->readSecurityAlertStatusRegister(securityAlert);
}

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
unsigned int DrmControllerRegisters::waitSecurityAlertStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitSecurityAlertStatusRegister(timeout, expected, actual);
}

/** readLicenseTimerInitSemaphoreAcknowledgeStatusRegister
*   \brief Read the status register and get the License Timer Init Semaphore Acknowledge status bit.
*   This method will access to the system bus to read the status register.
*   \param[out] licenseTimerInitSemaphoreAcknowledge is the value of the status bit License Timer Init Semaphore Acknowledge.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerInitSemaphoreAcknowledgeStatusRegister(bool &licenseTimerInitSemaphoreAcknowledge) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerInitSemaphoreAcknowledgeStatusRegister(licenseTimerInitSemaphoreAcknowledge);
}

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
unsigned int DrmControllerRegisters::waitLicenseTimerInitSemaphoreAcknowledgeStatusRegister(const unsigned int &timeout, const bool &expected, bool &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseTimerInitSemaphoreAcknowledgeStatusRegister(timeout, expected, actual);
}

/** readNumberOfLicenseTimerLoadedStatusRegister
*   \brief Read the status register and get the number of license timer loaded.
*   This method will access to the system bus to read the status register.
*   \param[out] numberOfLicenseTimerLoaded is the number of license timer loaded retrieved from the status.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readNumberOfLicenseTimerLoadedStatusRegister(unsigned int &numberOfLicenseTimerLoaded) const {
  return mDrmControllerRegistersStrategyInterface->readNumberOfLicenseTimerLoadedStatusRegister(numberOfLicenseTimerLoaded);
}

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
unsigned int DrmControllerRegisters::waitNumberOfLicenseTimerLoadedStatusRegister(const unsigned int &timeout, const unsigned int &expected, unsigned int &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitNumberOfLicenseTimerLoadedStatusRegister(timeout, expected, actual);
}

/** readNumberOfDetectedIpsStatusRegister
*   \brief Read the status register and get the number of detected IPs.
*   This method will access to the system bus to read the status register.
*   \param[out] numberOfDetectedIps is the number of detected ips retrieved from the status.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readNumberOfDetectedIpsStatusRegister(unsigned int &numberOfDetectedIps) const {
  return mDrmControllerRegistersStrategyInterface->readNumberOfDetectedIpsStatusRegister(numberOfDetectedIps);
}

/** readExtractDnaErrorRegister
*   \brief Read the error register and get the error code related to dna extraction.
*   This method will access to the system bus to read the error register.
*   \param[out] dnaExtractError is the error code related to dna extraction.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readExtractDnaErrorRegister(unsigned char &dnaExtractError) const {
  return mDrmControllerRegistersStrategyInterface->readExtractDnaErrorRegister(dnaExtractError);
}

/** waitExtractDnaErrorRegister
*   \brief Wait error to reach specified value.
*   This method will access to the system bus to read the error register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expected is the value of the error to be expected.
*   \param[out] actual is the value of the error read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
*   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::waitExtractDnaErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitExtractDnaErrorRegister(timeout, expected, actual);
}

/** readExtractVlnvErrorRegister
*   \brief Read the error register and get the error code related to vlnv extraction.
*   This method will access to the system bus to read the error register.
*   \param[out] vlnvExtractError is the error code related to vlnv extraction.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readExtractVlnvErrorRegister(unsigned char &vlnvExtractError) const {
  return mDrmControllerRegistersStrategyInterface->readExtractVlnvErrorRegister(vlnvExtractError);
}

/** waitExtractVlnvErrorRegister
*   \brief Wait error to reach specified value.
*   This method will access to the system bus to read the error register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expected is the value of the error to be expected.
*   \param[out] actual is the value of the error read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
*   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::waitExtractVlnvErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitExtractVlnvErrorRegister(timeout, expected, actual);
}

/** readActivationErrorRegister
*   \brief Read the error register and get the error code related to activation.
*   This method will access to the system bus to read the error register.
*   \param[out] activationError is the error code related to activation.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readActivationErrorRegister(unsigned char &activationError) const {
  return mDrmControllerRegistersStrategyInterface->readActivationErrorRegister(activationError);
}

/** waitActivationErrorRegister
*   \brief Wait error to reach specified value.
*   This method will access to the system bus to read the error register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expected is the value of the error to be expected.
*   \param[out] actual is the value of the error read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
*   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::waitActivationErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitActivationErrorRegister(timeout, expected, actual);
}

/** readLicenseTimerLoadErrorRegister
*   \brief Read the error register and get the error code related to license timer loading.
*   This method will access to the system bus to read the error register.
*   \param[out] licenseTimerLoadError is the error code related to license timer loading.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerLoadErrorRegister(unsigned char &licenseTimerLoadError) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerLoadErrorRegister(licenseTimerLoadError);
}

/** waitActivationErrorRegister
*   \brief Wait error to reach specified value.
*   This method will access to the system bus to read the error register.
*   \param[in]  timeout is the timeout value in micro seconds.
*   \param[in]  expected is the value of the error to be expected.
*   \param[out] actual is the value of the error read.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_HARDWARE_TIMEOUT_ERROR if a timeout occured, errors from read/write register functions otherwize.
*   \throw DrmControllerTimeOutException whenever a timeout error occured. DrmControllerTimeOutException::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::waitLicenseTimerLoadErrorRegister(const unsigned int &timeout, const unsigned char &expected, unsigned char &actual) const {
  return mDrmControllerRegistersStrategyInterface->waitLicenseTimerLoadErrorRegister(timeout, expected, actual);
}

/** readDnaRegister
*   \brief Read the dna register and get the value.
*   This method will access to the system bus to read the dna register.
*   \param[out] dna is the dna value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readDnaRegister(std::vector<unsigned int> &dna) const {
  return mDrmControllerRegistersStrategyInterface->readDnaRegister(dna);
}

/** readDnaRegister
*   \brief Read the dna register and get the value.
*   This method will access to the system bus to read the dna register.
*   \param[out] dna is the dna value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readDnaRegister(std::string &dna) const {
  return mDrmControllerRegistersStrategyInterface->readDnaRegister(dna);
}

/** readSaasChallengeRegister
*   \brief Read the Saas Challenge register and get the value.
*   This method will access to the system bus to read the Saas Challenge register.
*   \param[out] saasChallenge is the saas challenge value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readSaasChallengeRegister(std::vector<unsigned int> &saasChallenge) const {
  return mDrmControllerRegistersStrategyInterface->readSaasChallengeRegister(saasChallenge);
}

/** readSaasChallengeRegister
*   \brief Read the saas challenge register and get the value.
*   This method will access to the system bus to read the saas challenge register.
*   \param[out] saasChallenge is the saas challenge value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readSaasChallengeRegister(std::string &saasChallenge) const {
  return mDrmControllerRegistersStrategyInterface->readSaasChallengeRegister(saasChallenge);
}

/** readLicenseTimerCounterRegister
*   \brief Read the License Timer Counter register and get the value.
*   This method will access to the system bus to read the License Timer Counter register.
*   \param[out] licenseTimerCounter is the License Timer Counter value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerCounterRegister(std::vector<unsigned int> &licenseTimerCounter) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerCounterRegister(licenseTimerCounter);
}

/** readLicenseTimerCounterRegister
*   \brief Read the license timer counter register and get the value.
*   This method will access to the system bus to read the license timer counter register.
*   \param[out] licenseTimerCounter is the license timer counter value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseTimerCounterRegister(std::string &licenseTimerCounter) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseTimerCounterRegister(licenseTimerCounter);
}

/** readDrmVersionRegister
*   \brief Read the drm version register and get the value.
*   This method will access to the system bus to read the drm version register.
*   \param[out] drmVersion is the drm version value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readDrmVersionRegister(unsigned int &drmVersion) const {
  return mDrmControllerRegistersStrategyInterface->readDrmVersionRegister(drmVersion);
}

/** readDrmVersionRegister
*   \brief Read the drm version register and get the value.
*   This method will access to the system bus to read the drm version register.
*   \param[out] drmVersion is the drm version value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readDrmVersionRegister(std::string &drmVersion) const {
  return mDrmControllerRegistersStrategyInterface->readDrmVersionRegister(drmVersion);
}

/** readAdaptiveProportionTestFailuresRegister
*   \brief Read the Adaptive Proportion Test Failures register and get the value.
*   This method will access to the system bus to read the Adaptive Proportion Test Failures register.
*   \param[out] adaptiveProportionTestFailures is the Adaptive Proportion Test Failures value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readAdaptiveProportionTestFailuresRegister(std::vector<unsigned int> &adaptiveProportionTestFailures) const {
  return mDrmControllerRegistersStrategyInterface->readAdaptiveProportionTestFailuresRegister(adaptiveProportionTestFailures);
}

/** readAdaptiveProportionTestFailuresRegister
*   \brief Read the Adaptive Proportion Test Failures register and get the value.
*   This method will access to the system bus to read the Adaptive Proportion Test Failures register.
*   \param[out] adaptiveProportionTestFailures is the Adaptive Proportion Test Failures value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readAdaptiveProportionTestFailuresRegister(std::string &adaptiveProportionTestFailures) const {
  return mDrmControllerRegistersStrategyInterface->readAdaptiveProportionTestFailuresRegister(adaptiveProportionTestFailures);
}

/** readRepetitionCountTestFailuresRegister
*   \brief Read the Repetition CountTest Failures register and get the value.
*   This method will access to the system bus to read the Repetition Count Test Failures register.
*   \param[out] repetitionCountTestFailures is the Repetition Count Test Failures value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readRepetitionCountTestFailuresRegister(std::vector<unsigned int> &repetitionCountTestFailures) const {
  return mDrmControllerRegistersStrategyInterface->readRepetitionCountTestFailuresRegister(repetitionCountTestFailures);
}

/** readRepetitionCountTestFailuresRegister
*   \brief Read the Repetition Count Test Failures register and get the value.
*   This method will access to the system bus to read the Repetition Count Test Failures register.
*   \param[out] repetitionCountTestFailures is the Repetition Count Test Failures value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readRepetitionCountTestFailuresRegister(std::string &repetitionCountTestFailures) const {
  return mDrmControllerRegistersStrategyInterface->readRepetitionCountTestFailuresRegister(repetitionCountTestFailures);
}

/** readLogsRegister
*   \brief Read the logs register and get the value.
*   This method will access to the system bus to read the logs register.
*   \param[in] numberOfIps is the total number of IPs.
*   \param[out] logs is the logs value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLogsRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &logs) const {
  return mDrmControllerRegistersStrategyInterface->readLogsRegister(numberOfIps, logs);
}

/** readLogsRegister
*   \brief Read the logs register and get the value.
*   This method will access to the system bus to read the logs register.
*   \param[in] numberOfIps is the total number of IPs.
*   \param[out] logs is the logs value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLogsRegister(const unsigned int &numberOfIps, std::string &logs) const {
  return mDrmControllerRegistersStrategyInterface->readLogsRegister(numberOfIps, logs);
}

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
unsigned int DrmControllerRegisters::readVlnvFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &vlnvFile) const {
  return mDrmControllerRegistersStrategyInterface->readVlnvFileRegister(numberOfIps, vlnvFile);
}

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
unsigned int DrmControllerRegisters::readVlnvFileRegister(const unsigned int &numberOfIps, std::vector<std::string> &vlnvFile) const {
  return mDrmControllerRegistersStrategyInterface->readVlnvFileRegister(numberOfIps, vlnvFile);
}

/** readLicenseFileRegister
*   \brief Read the license file and get the value.
*   This method will access to the system bus to read the license file.
*   \param[in] licenseFileSize is the number of 128 bits words to read.
*   \param[out] licenseFile is the license file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseFileRegister(const unsigned int &licenseFileSize, std::vector<unsigned int> &licenseFile) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseFileRegister(licenseFileSize, licenseFile);
}

/** readLicenseFileRegister
*   \brief Read the license file and get the value.
*   This method will access to the system bus to read the license file.
*   The license file is a string using a hexadecimal representation.
*   \param[in] licenseFileSize is the number of 128 bits words to read.
*   \param[out] licenseFile is the license file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readLicenseFileRegister(const unsigned int &licenseFileSize, std::string &licenseFile) const {
  return mDrmControllerRegistersStrategyInterface->readLicenseFileRegister(licenseFileSize, licenseFile);
}

/** readTraceFileRegister
*   \brief Read the trace file and get the value.
*   This method will access to the system bus to read the trace file.
*   \param[in] numberOfIps is the total number of IPs.
*   \param[out] traceFile is the trace file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readTraceFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &traceFile) const {
  return mDrmControllerRegistersStrategyInterface->readTraceFileRegister(numberOfIps, traceFile);
}

/** readTraceFileRegister
*   \brief Read the trace file and get the value.
*   This method will access to the system bus to read the trace file.
*   \param[in] numberOfIps is the total number of IPs.
*   \param[out] traceFile is the trace file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readTraceFileRegister(const unsigned int &numberOfIps, std::vector<std::string> &traceFile) const {
  return mDrmControllerRegistersStrategyInterface->readTraceFileRegister(numberOfIps, traceFile);
}

/** readMeteringFileRegister
*   \brief Read the metering file and get the value.
*   This method will access to the system bus to read the metering file.
*   \param[in] numberOfIps is the total number of IPs.
*   \param[out] meteringFile is the metering file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readMeteringFileRegister(const unsigned int &numberOfIps, std::vector<unsigned int> &meteringFile) const {
  return mDrmControllerRegistersStrategyInterface->readMeteringFileRegister(numberOfIps, meteringFile);
}

/** readMeteringFileRegister
*   \brief Read the metering file and get the value.
*   This method will access to the system bus to read the metering file.
*   \param[in] numberOfIps is the total number of IPs.
*   \param[out] meteringFile is the metering file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readMeteringFileRegister(const unsigned int &numberOfIps, std::vector<std::string> &meteringFile) const {
  return mDrmControllerRegistersStrategyInterface->readMeteringFileRegister(numberOfIps, meteringFile);
}

/** readMailboxFileSizeRegister
*   \brief Read the mailbox file word numbers.
*   This method will access to the system bus to read the mailbox file.
*   \param[out] readOnlyMailboxWordNumber is the number of words in the read-only mailbox.
*   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::readMailboxFileSizeRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber) const {
  return mDrmControllerRegistersStrategyInterface->readMailboxFileSizeRegister(readOnlyMailboxWordNumber, readWriteMailboxWordNumber);
}

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
unsigned int DrmControllerRegisters::readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                              std::vector<unsigned int> &readOnlyMailboxData, std::vector<unsigned int> &readWriteMailboxData) const {
  return mDrmControllerRegistersStrategyInterface->readMailboxFileRegister(readOnlyMailboxWordNumber, readWriteMailboxWordNumber, readOnlyMailboxData, readWriteMailboxData);
}

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
unsigned int DrmControllerRegisters::readMailboxFileRegister(unsigned int &readOnlyMailboxWordNumber, unsigned int &readWriteMailboxWordNumber,
                                              std::vector<std::string> &readOnlyMailboxData, std::vector<std::string> &readWriteMailboxData) const {
  return mDrmControllerRegistersStrategyInterface->readMailboxFileRegister(readOnlyMailboxWordNumber, readWriteMailboxWordNumber, readOnlyMailboxData, readWriteMailboxData);
}

/** writeMailboxFileRegister
*   \brief Write the mailbox file.
*   This method will access to the system bus to write the mailbox file.
*   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
*   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeMailboxFileRegister(const std::vector <unsigned int> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const {
  return mDrmControllerRegistersStrategyInterface->writeMailboxFileRegister(readWriteMailboxData, readWriteMailboxWordNumber);
}

/** writeMailboxFileRegister
*   \brief Write the mailbox file.
*   This method will access to the system bus to write the mailbox file.
*   \param[in] readWriteMailboxData is the data to write into the read-write mailbox.
*   \param[out] readWriteMailboxWordNumber is the number of words in the read-write mailbox.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeMailboxFileRegister(const std::vector<std::string> &readWriteMailboxData, unsigned int &readWriteMailboxWordNumber) const {
  return mDrmControllerRegistersStrategyInterface->writeMailboxFileRegister(readWriteMailboxData, readWriteMailboxWordNumber);
}

/** throwLicenseTimerResetedException
*   \param[in]  expectedStatus is the value of the status to be expected.
*   \param[in]  actualStatus is the value of the status read.
*   \param[in]  expectedError is the value of the error to be expected.
*   \param[in]  actualError is the value of the error read.
*   \throw Throw DrmControllerLicenseTimerResetedException. DrmControllerLicenseTimerResetedException::what() should be called to get the exception description.
**/
void DrmControllerRegisters::throwLicenseTimerResetedException(const bool &expectedStatus, const bool &actualStatus,
                                                               const unsigned char &expectedError, const unsigned char &actualError) const {
  mDrmControllerRegistersStrategyInterface->throwLicenseTimerResetedException(expectedStatus, actualStatus, expectedError, actualError,
                                                                              mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(expectedError),
                                                                              mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(actualError));
}

/** throwTimeoutException
*   \brief Throw a timeout exception.
*   \param[in]  headerMsg is the header message to be used when throwing timeout error exception.
*   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
*/
void DrmControllerRegisters::throwTimeoutException(const std::string &headerMsg) const {
  mDrmControllerRegistersStrategyInterface->throwTimeoutException(headerMsg);
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
*   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
*/
void DrmControllerRegisters::throwTimeoutException(const std::string &headerMsg, const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                   const std::string &expectedErrorMsg, const std::string &actualErrorMsg, const unsigned int &expectedStatus, const unsigned int &actualStatus,
                                                   const unsigned char &expectedError, const unsigned char &actualError) const {
  mDrmControllerRegistersStrategyInterface->throwTimeoutException(headerMsg, expectedStatusMsg, actualStatusMsg, expectedErrorMsg, actualErrorMsg,
                                                                  expectedStatus, actualStatus, expectedError, actualError,
                                                                  mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(expectedError),
                                                                  mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(actualError));
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
*   \throw Throw DrmControllerTimeOutException. DrmControllerTimeOutException::what() should be called to get the exception description.
*/
void DrmControllerRegisters::throwTimeoutException(const std::string &headerMsg, const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                   const std::string &expectedErrorMsg, const std::string &actualErrorMsg, const bool &expectedStatus, const bool &actualStatus,
                                                   const unsigned char &expectedError, const unsigned char &actualError) const {
  mDrmControllerRegistersStrategyInterface->throwTimeoutException(headerMsg, expectedStatusMsg, actualStatusMsg, expectedErrorMsg, actualErrorMsg,
                                                                  expectedStatus, actualStatus, expectedError, actualError,
                                                                  mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(expectedError),
                                                                  mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(actualError));
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
void DrmControllerRegisters::throwTimeoutException(const std::string &headerMsg, const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                   const unsigned int &expectedStatus, const unsigned int &actualStatus) const {
  mDrmControllerRegistersStrategyInterface->throwTimeoutException(headerMsg, expectedStatusMsg, actualStatusMsg, expectedStatus, actualStatus);
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
void DrmControllerRegisters::throwTimeoutException(const std::string &headerMsg, const std::string &expectedStatusMsg, const std::string &actualStatusMsg,
                                                   const bool &expectedStatus, const bool &actualStatus) const {
  mDrmControllerRegistersStrategyInterface->throwTimeoutException(headerMsg, expectedStatusMsg, actualStatusMsg, expectedStatus, actualStatus);
}

/** printHwReport
* \brief Print all register content accessible through AXI-4 Lite Control channel.
* \param[in] file: Reference to output file where register contents are saved. By default print on standard output
*/
void DrmControllerRegisters::printHwReport(std::ostream &file) const {
  mDrmControllerRegistersStrategyInterface->printHwReport(file);
}

/** printMeteringFile
*   \brief Display the value of the metering file.
*   \param[in] file is the stream to use for the data print. By default print on standard output.
**/
void DrmControllerRegisters::printMeteringFileHwReport(std::ostream &file) const {
  mDrmControllerRegistersStrategyInterface->printMeteringFileHwReport(file);
}

/** getDrmErrorRegisterMessage
*   \brief Get the error message from the error register value.
*   \param[in] errorRegister is the value of the error register.
*   \return Returns the error message.
**/
const char* DrmControllerRegisters::getDrmErrorRegisterMessage(const unsigned char &errorRegister) const {
  return mDrmControllerRegistersStrategyInterface->getDrmErrorRegisterMessage(errorRegister);
}

/************************************************************/
/**                  PROTECTED MEMBER FUNCTIONS            **/
/************************************************************/

/** writeRegistersPageRegister
*   \brief Write the page register to select the registers page.
*   This method will access to the system bus to write into the page register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeRegistersPageRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeRegistersPageRegister();
}

/** writeVlnvFilePageRegister
*   \brief Write the page register to select the vlnv file page.
*   This method will access to the system bus to write into the page register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeVlnvFilePageRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeVlnvFilePageRegister();
}

/** writeLicenseFilePageRegister
*   \brief Write the page register to select the license file page.
*   This method will access to the system bus to write into the page register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseFilePageRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseFilePageRegister();
}

/** writeTraceFilePageRegister
*   \brief Write the page register to select the trace file page.
*   This method will access to the system bus to write into the page register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeTraceFilePageRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeTraceFilePageRegister();
}

/** writeMeteringFilePageRegister
*   \brief Write the page register to select the metering file page.
*   This method will access to the system bus to write into the page register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeMeteringFilePageRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeMeteringFilePageRegister();
}

/** writeMailBoxFilePageRegister
*   \brief Write the page register to select the mailbox file page.
*   This method will access to the system bus to write into the page register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeMailBoxFilePageRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeMailBoxFilePageRegister();
}

/** writeNopCommandRegister
*   \brief Write the command register to the NOP Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeNopCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeNopCommandRegister();
}

/** writeDnaExtractCommandRegister
*   \brief Write the command register to the DNA Extract Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeDnaExtractCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeDnaExtractCommandRegister();
}

/** writeVlnvExtractCommandRegister
*   \brief Write the command register to the VLNV Extract Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeVlnvExtractCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeVlnvExtractCommandRegister();
}

/** writeActivateCommandRegister
*   \brief Write the command register to the Activate Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeActivateCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeActivateCommandRegister();
}

/** writeEndSessionMeteringExtractCommandRegister
*   \brief Write the command register to the end session extract metering Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeEndSessionMeteringExtractCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeEndSessionMeteringExtractCommandRegister();
}

/** writeMeteringExtractCommandRegister
*   \brief Write the command register to the extract metering Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeMeteringExtractCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeMeteringExtractCommandRegister();
}

/** writeSampleLicenseTimerCounterCommandRegister
*   \brief Write the command register to the sample license timer counter Command.
*   This method will access to the system bus to write into the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeSampleLicenseTimerCounterCommandRegister() const {
  return mDrmControllerRegistersStrategyInterface->writeSampleLicenseTimerCounterCommandRegister();
}

/** writeLicenseTimerInitSemaphoreRequestCommandRegister
*   \brief Write the LicenseTimerInitSemaphoreRequest bit in the command register to the given value (do not modify the other bits of the command register).
*   This method will access to the system bus to write into the command register.
*   \param[in] licenseTimerInitSemaphoreRequest is the value of the LicenseTimerInitSemaphoreRequest bit to write in the command register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_UNSUPPORTED_FEATURE_ERROR if the feature is not supported, errors from read/write register functions otherwise.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseTimerInitSemaphoreRequestCommandRegister(bool &licenseTimerInitSemaphoreRequest) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseTimerInitSemaphoreRequestCommandRegister(licenseTimerInitSemaphoreRequest);
}

/** writeLicenseStartAddressRegister
*   \brief Write the license start address register.
*   This method will access to the system bus to write the license start address.
*   \param[in] licenseStartAddress is a list of binary values for the license start address register.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseStartAddressRegister(const std::vector<unsigned int> &licenseStartAddress) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseStartAddressRegister(licenseStartAddress);
}

/** writeLicenseStartAddressRegister
*   \brief Write the value of the license start address.
*   This method will access to the system bus to write the license start address register.
*   \param[in] licenseStartAddress is the license start address value.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseStartAddressRegister(const std::string &licenseStartAddress) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseStartAddressRegister(licenseStartAddress);
}

/** writeLicenseFile
*   \brief Write the license file.
*   This method will access to the system bus to write the license file.
*   \param[in] licenseFileSize is the number of 128 bits words to read.
*   \param[in] licenseFile is the license file.
*   \return Returns mDrmApi_NO_ERROR if no error, mDrmApi_Unsupported_Feature if the feature is not supported, errors from read/write register functions otherwize.
*   \throw DrmControllerUnsupportedFeature whenever the feature is not supported. DrmControllerUnsupportedFeature::what() should be called to get the exception description.
**/
unsigned int DrmControllerRegisters::writeLicenseFileRegister(const unsigned int &licenseFileSize, const std::vector<unsigned int> &licenseFile) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseFileRegister(licenseFileSize, licenseFile);
}

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
unsigned int DrmControllerRegisters::writeLicenseFileRegister(const std::string &licenseFile) const {
  return mDrmControllerRegistersStrategyInterface->writeLicenseFileRegister(licenseFile);
}

/************************************************************/
/**                  PRIVATE MEMBER FUNCTIONS              **/
/************************************************************/

/** selectRegistersStrategy
*   \brief Select the register strategy that fits the most with the hardware.
*   \param[in] readRegisterFunction function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] writeRegisterFunction function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
*   \return Returns the address of the selected DrmControllerRegistersStrategyInterface.
*   \throw DrmControllerVersionCheckException whenever an error occured. DrmControllerVersionCheckException::what() should be called to get the exception description.
**/
DrmControllerRegistersStrategyInterface* DrmControllerRegisters::selectRegistersStrategy(tDrmReadRegisterFunction readRegisterFunction,
                                                                                         tDrmWriteRegisterFunction writeRegisterFunction) const {
  // dictionaries of strategies
  tDrmControllerRegistersStrategyDictionary strategies(createRegistersStrategies(readRegisterFunction, writeRegisterFunction));
  // get and parse existing version
  std::string parsedStrategiesVersion(parseStrategiesDrmVersion((readStrategiesDrmVersion(strategies))));
  // final check
  if (parsedStrategiesVersion.empty() == true) {
    // unable to find a strategy
    std::ostringstream writter;
    writter << DRM_CONTROLLER_ERROR_HEADER << "Unable to select a register strategy that is compatible with the DRM Controller" << DRM_CONTROLLER_ERROR_FOOTER << std::endl;
    throw DrmControllerVersionCheckException(writter.str());
  }
  // return the strategy found
  return getRegistersStrategy(strategies, parsedStrategiesVersion);
}

/** createRegistersStrategies
*   \brief Create the register strategies.
*   \param[in] readRegisterFunction function pointer to read 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int&)".
*   \param[in] writeRegisterFunction function pointer to write 32 bits register.
*              The function pointer shall have the following prototype "unsigned int f(const std::string&, unsigned int)".
*   \return Returns a dictionary of supported drm version and DrmControllerRegistersStrategyInterface.
**/
DrmControllerRegisters::tDrmControllerRegistersStrategyDictionary DrmControllerRegisters::createRegistersStrategies(tDrmReadRegisterFunction readRegisterFunction,
                                                                                                                    tDrmWriteRegisterFunction writeRegisterFunction) const {
  tDrmControllerRegistersStrategyDictionary strategies;
  strategies[DRM_CONTROLLER_V3_0_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v3_0_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V3_1_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v3_1_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V3_2_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v3_2_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V3_2_1_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v3_2_1(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V3_2_2_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v3_2_2(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V4_0_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v4_0_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V4_0_1_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v4_0_1(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V4_1_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v4_1_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V4_2_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v4_2_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V4_2_1_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v4_2_1(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V6_0_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v6_0_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V6_0_1_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v6_0_1(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V7_0_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v7_0_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V7_1_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v7_1_0(readRegisterFunction, writeRegisterFunction);
  strategies[DRM_CONTROLLER_V8_0_0_SUPPORTED_VERSION] = new DrmControllerRegistersStrategy_v8_0_0(readRegisterFunction, writeRegisterFunction);
  return strategies;
}

/** getRegistersStrategy
*   \brief Get the selected register strategy and free the unselected strategies.
*   \param[in] strategies is the dictionary of register strategies.
*   \param[in] selectedVersion is the selected drm version.
*   \return Returns the address of the selected DrmControllerRegistersStrategyInterface.
**/
DrmControllerRegistersStrategyInterface* DrmControllerRegisters::getRegistersStrategy(tDrmControllerRegistersStrategyDictionary &strategies,
                                                                                      const std::string &selectedVersion) const {
  // create a list of unsupported version
  std::vector<std::string> unsupportedVersion;
  for (tDrmControllerRegistersStrategyConstIterator it = strategies.rbegin(); it != strategies.rend(); it++)
    unsupportedVersion.push_back(it->first);
  unsupportedVersion.erase(std::find(unsupportedVersion.begin(),unsupportedVersion.end(), selectedVersion));
  // delete each unsupported strategy
  for (std::vector<std::string>::const_iterator it = unsupportedVersion.begin(); it != unsupportedVersion.end(); it++) {
    std::map<std::string, DrmControllerRegistersStrategyInterface*>::iterator strategy = strategies.find(*it);
    if (strategy != strategies.end()) {
      delete strategy->second;
      strategy->second = NULL;
      strategies.erase(strategy);
    }
  }
  return strategies.begin()->second;
}

/** readStrategiesDrmVersion
*   \brief Read the drm version of each strategy.
*   \param[in] strategies is the dictionary of register strategies.
*   \return Returns a list of string containing the supported drm version.
**/
std::vector<std::string> DrmControllerRegisters::readStrategiesDrmVersion(const tDrmControllerRegistersStrategyDictionary &strategies) const {
  // get the version of each strategy
  tDrmControllerRegistersVersionDictionary strategiesVersion;
  for (tDrmControllerRegistersStrategyConstIterator it = strategies.rbegin(); it != strategies.rend(); it++) {
    std::string drmVersion;
    if (it->second->readDrmVersionRegister(drmVersion) == mDrmApi_NO_ERROR)
      strategiesVersion[it->first] = DrmControllerDataConverter::binaryToVersionString(DrmControllerDataConverter::hexStringToBinary(drmVersion)[0]);
  }
  return filterStrategiesDrmVersion(strategiesVersion);
}

/** filterStrategiesDrmVersion
*   \brief Filter the strategies drm version dictionary.
*   \param[in] strategiesVersion is the dictionary of strategies version.
*   \return Returns a list of string containing the supported drm version.
**/
std::vector<std::string> DrmControllerRegisters::filterStrategiesDrmVersion(const tDrmControllerRegistersVersionDictionary &strategiesVersion) const {
  std::vector<std::string> filteredVersion;
  for (tDrmControllerRegistersVersionConstIterator it = strategiesVersion.rbegin(); it != strategiesVersion.rend(); it++) {
    if (checkSupportedDrmVersion(it->first, it->second) == true)
      filteredVersion.push_back(it->first);
  }
  return filteredVersion;
}

/** parseStrategiesDrmVersion
*   \brief Parse the drm version of each strategy.
*   \param[in] filteredVersion is list of string containing the supported drm version.
*   \return Returns a string of the supported drm version.
**/
std::string DrmControllerRegisters::parseStrategiesDrmVersion(const std::vector<std::string> &filteredVersion) const {
  // check for supported version
  unsigned int selectedBugFix = 0;
  std::string selectedVersion = "";
  for (std::vector<std::string>::const_iterator it = filteredVersion.begin(); it != filteredVersion.end(); it++) {
    std::string drmVersionMajor, drmVersionMinor, drmVersionBug;
    DrmControllerVersion::getVersionElements(*it, drmVersionMajor, drmVersionMinor, drmVersionBug);
    unsigned int currentBugFix = (unsigned int)strtoul(drmVersionBug.c_str(), NULL, 10);
    if (currentBugFix >= selectedBugFix) {
      selectedBugFix = currentBugFix;
      selectedVersion = *it;
    }
  }
  return selectedVersion;
}

/** checkSupportedDrmVersion
*   \brief Check the drm version is supported.
*   \param[in] supportedVersion is the supported version of the DRM Controller.
*   \param[in] drmVersion is the read drm version of the DRM Controller.
*   \return Returns true if the version is supported, false otherwize.
**/
bool DrmControllerRegisters::checkSupportedDrmVersion(const std::string &supportedVersion, const std::string &drmVersion) const {
  // get supported version elements
  std::string supportedVersionMajor, supportedVersionMinor, supportedVersionBug;
  DrmControllerVersion::getVersionElements(supportedVersion, supportedVersionMajor, supportedVersionMinor, supportedVersionBug);
  // get read drm version elements
  std::string drmVersionMajor, drmVersionMinor, drmVersionBug;
  DrmControllerVersion::getVersionElements(drmVersion, drmVersionMajor, drmVersionMinor, drmVersionBug);
  // check major and minor version matches
  if (supportedVersionMajor == drmVersionMajor && supportedVersionMinor == drmVersionMinor)
    return true;
  return false;
}

/** operator<<
*   \brief Declaration of function for operator <<
*   \param[in] file is the stream to use for the data display.
*   \param[in] DrmControllerOperations is a reference to this object.
*   \return Returns the output stream used
**/
std::ostream& operator<<(std::ostream &file, const DrmControllerRegisters &drmControllerRegisters) {
  drmControllerRegisters.printHwReport(file);
  return file;
}
