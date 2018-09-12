/**
*  \file      DrmControllerTypes.hpp
*  \version   3.0.0.1
*  \date      September 2018
*  \brief     Types definitions.
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/


#ifndef __DRM_CONTROLLER_TYPES_HPP__
#define __DRM_CONTROLLER_TYPES_HPP__

#include <functional>

/**
*   \namespace DrmControllerLibrary
**/
namespace DrmControllerLibrary {

  /** \typedef t_drmReadRegisterFunction
  *   \brief   Read register function prototype.
  *   \remark  The read register function shall return 0 for no error.
  **/
  typedef std::function<unsigned int(const std::string&, unsigned int&)> t_drmReadRegisterFunction;

  /** \typedef t_drmWriteRegisterFunction
  *   \brief   Write register function prototype.
  *   \remark  The write register function shall return 0 for no error.
  **/
  typedef std::function<unsigned int(const std::string&, unsigned int)>  t_drmWriteRegisterFunction;

  /**
  *   \enum  t_drmPageRegisterEnumValues
  *   \brief Enumeration for page codes.
  **/
  typedef enum t_drmPageRegisterEnumValues {
    mDrmPageRegisters    = 0x00,  /**<Value for the registers page.**/
    mDrmPageVlnvFile     = 0x01,  /**<Value for the vlnv file page.**/
    mDrmPageLicenseFile  = 0x02,  /**<Value for the license file page.**/
    mDrmPageTraceFile    = 0x03,  /**<Value for the trace file page.**/
    mDrmPageMeteringFile = 0x04   /**<Value for the metering file page.**/
  } t_drmPageRegisterEnumValues;

  /**
  *   \enum  t_drmCommandRegisterEnumValues
  *   \brief Enumeration for command codes.
  **/
  typedef enum t_drmCommandRegisterEnumValues {
    mDrmCommandNop                       = 0x20,  /**<Value for command NOP.**/
    mDrmCommandExtractDna                = 0x01,  /**<Value for command Extract DNA.**/
    mDrmCommandExtractVlnv               = 0x02,  /**<Value for command Extract VLNV.**/
    mDrmCommandActivate                  = 0x04,  /**<Value for command Activate.**/
    mDrmCommandHeartBeatInit             = 0x08,  /**<Value for command Heart Beat Initialization (unsued).**/
    mDrmCommandHeartBeatFinish           = 0x10,  /**<Value for command Heart Beat Finalization (unused).**/
    mDrmCommandEndSessionExtractMetering = 0x40   /**<Value for command End Session Extract Metering.**/
  } t_drmCommandRegisterEnumValues;

  /**
  *   \enum  t_drmStatusRegisterEnumValues
  *   \brief Enumeration for status bit positions.
  **/
  typedef enum t_drmStatusRegisterEnumValues {
    mDrmStatusDnaReady                = 0,  /**<Position of the status DNA Ready.**/
    mDrmStatusVlnvReady               = 1,  /**<Position of the status VLNV Ready.**/
    mDrmStatusActivationDone          = 2,  /**<Position of the status Activation Done.**/
    mDrmStatusAutoEnabled             = 3,  /**<Position of the status Auto Controller Enabled.**/
    mDrmStatusAutoBusy                = 4,  /**<Position of the status Auto Controller Busy.**/
    mDrmStatusMeteringEnabled         = 5,  /**<Position of the status Metering Operations Enabled.**/
    mDrmStatusMeteringReady           = 6,  /**<Position of the status Metering Ready.**/
    mDrmStatusSaasChallengeReady      = 7,  /**<Position of the status Saas Challenge Ready.**/
    mDrmStatusLicenseTimerEnabled     = 8,  /**<Position of the status License Timer Operations Enabled.**/
    mDrmStatusLicenseTimerInitLoaded  = 9,  /**<Position of the status License Timer Init Loaded.**/
    mDrmStatusEndSessionMeteringReady = 10, /**<Position of the status End session metering ready.**/
    mDrmStatusIpNmbrLsb               = 11, /**<LSB position of the status IP Number.**/
    mDrmStatusIpNmbrMsb               = 31  /**<MSB position of the status IP Number.**/
  } t_drmStatusRegisterEnumValues;

  /**
  *   \enum  t_drmStatusRegisterMaskEnumValues
  *   \brief Enumeration for status bit masks.
  **/
  typedef enum t_drmStatusRegisterMaskEnumValues {
    mDrmStatusMaskDnaReady                = 0x00000001,   /**<Mask for the status bit DNA Ready.**/
    mDrmStatusMaskVlnvReady               = 0x00000002,   /**<Mask for the status bit VLNV Ready.**/
    mDrmStatusMaskActivationDone          = 0x00000004,   /**<Mask for the status bit Activation Done.**/
    mDrmStatusMaskAutoEnabled             = 0x00000008,   /**<Mask for the status bit Auto Controller Enabled.**/
    mDrmStatusMaskAutoBusy                = 0x00000010,   /**<Mask for the status bit Auto Controller Busy.**/
    mDrmStatusMaskMeteringEnabled         = 0x00000020,   /**<Mask for the status bit Metering Operations Enabled.**/
    mDrmStatusMaskMeteringReady           = 0x00000040,   /**<Mask for the status bit Metering Ready.**/
    mDrmStatusMaskSaasChallengeReady      = 0x00000080,   /**<Mask for the status bit Saas Challenge Ready.**/
    mDrmStatusMaskLicenseTimerEnabled     = 0x00000100,   /**<Mask for the status bit License Timer Operations Enabled.**/
    mDrmStatusMaskLicenseTimerInitLoaded  = 0x00000200,   /**<Mask for the status bit License Timer Load Done.**/
    mDrmStatusMaskEndSessionMeteringReady = 0x00000400,   /**<Mask for the status End session metering ready.**/
    mDrmStatusMaskIpNmbr                  = 0xFFFFF800    /**<Mask for the status IP Number.**/
  } t_drmStatusRegisterMaskEnumValues;

  /**
  *   \enum  t_drmErrorRegisterEnumValues
  *   \brief Enumeration for error codes.
  **/
  typedef enum t_drmErrorRegisterEnumValues {
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
  } t_drmErrorRegisterEnumValues;

  /**
  *   \enum  t_drmErrorRegisterBytePositionEnumValues
  *   \brief Enumeration for error byte positions.
  **/
  typedef enum t_drmErrorRegisterBytePositionEnumValues {
    mDrmActivationErrorPosition       = 0,  /**<Error register position for Activation.**/
    mDrmDnaExtractErrorPosition       = 1,  /**<Error register position for DNA Extraction.**/
    mDrmVlnvExtractErrorPosition      = 2,  /**<Error register position for VLNV Extraction.**/
    mDrmLicenseTimerLoadErrorPosition = 3   /**<Error register position for License Timer Load.**/
  } t_drmErrorRegisterBytePositionEnumValues;

  /**
  *   \enum  t_drmErrorRegisterByteMaskEnumValues
  *   \brief Enumeration for error byte masks.
  **/
  typedef enum t_drmErrorRegisterByteMaskEnumValues {
    mDrmActivationErrorMask       = 0x000000FF, /**<Error register mask for Activation.**/
    mDrmDnaExtractErrorMask       = 0x0000FF00, /**<Error register mask for DNA Extraction.**/
    mDrmVlnvExtractErrorMask      = 0x00FF0000, /**<Error register mask for VLNV Extraction.**/
    mDrmLicenseTimerLoadErrorMask = 0xFF000000  /**<Error register mask for License Timer Load.**/
  } t_drmErrorRegisterByteMaskEnumValues;

  /**
  *   \struct t_drmError
  *   \brief  Structure containing the error code enumeration and the error message.
  **/
  typedef struct t_drmError {
    t_drmErrorRegisterEnumValues mDrmErrorEnum; /**<Error code value.**/
    std::string                  mDrmErrorText; /**<Error message value.**/
  } t_drmError;

  /** \var   mDrmErrorArraySize
  *   \brief Size of the array describing all error codes enumeration and attached error text.
  **/
  const unsigned int mDrmErrorArraySize = 22;

  /** \var   mDrmErrorArray
  *   \brief Array describing all error codes enumeration and attached error text.
  **/
  const t_drmError mDrmErrorArray[] = {
                        { mDrmErrorNotReady,                                   "Not ready" },                                        /**<Error message for operation not ready.**/
                        { mDrmErrorNoError,                                    "No error"  },                                        /**<Error message for succesful operation.**/
                        { mDrmErrorBusReadAuthenticatorDrmVersionTimeOutError, "Bus read authenticator drm version timeout error" }, /**<Error message for timeout during drm bus read authenticator version.**/
                        { mDrmErrorAuthenticatorDrmVersionError,               "Authenticator drm version error" },                  /**<Error message for authenticator version mismatch.**/
                        { mDrmErrorDnaAuthenticationError,                     "Authenticator authentication error" },               /**<Error message for authenticator authentication failure.**/
                        { mDrmErrorBusWriteAuthenticatorCommandTimeOutError,   "Bus write authenticator command timeout error" },    /**<Error message for timeout during drm bus write authenticator command.**/
                        { mDrmErrorBusReadAuthenticatorStatusTimeOutError,     "Bus read authenticator status timeout error" },      /**<Error message for timeout during drm bus read authenticator status.**/
                        { mDrmErrorBusWriteAuthenticatorChallengeTimeOutError, "Bus write authenticator challenge timeout error" },  /**<Error message for timeout during drm bus write authenticator challenge.**/
                        { mDrmErrorBusReadAuthenticatorResponseTimeOutError,   "Bus read authenticator response timeout error" },    /**<Error message for timeout during drm bus read authenticator response.**/
                        { mDrmErrorBusReadAuthenticatorDnaTimeOutError,        "Bus read authenticator dna timeout error" },         /**<Error message for timeout during drm bus read authenticator dna.**/
                        { mDrmErrorBusReadActivatorDrmVersionTimeOutError,     "Bus read activator drm version timeout error" },     /**<Error message for timeout during drm bus read activator version.**/
                        { mDrmErrorActivatorDrmVersionError,                   "Activator drm version error" },                      /**<Error message for activator version mismatch.**/
                        { mDrmErrorLicenseHeaderCheckError,                    "License header check error" },                       /**<Error message for license header mismatch.**/
                        { mDrmErrorLicenseDrmVersionError,                     "License drm version error" },                        /**<Error message for license version mismatch.**/
                        { mDrmErrorLicenseDnaDeltaError,                       "License dna delta error" },                          /**<Error message for license dna mismatch.**/
                        { mDrmErrorLicenseMacCheckError,                       "License MAC check error" },                          /**<Error message for license mac mismatch.**/
                        { mDrmErrorBusWriteActivatorCommandTimeOutError,       "Bus write activator command timeout error" },        /**<Error message for timeout during drm bus write activator command.**/
                        { mDrmErrorBusReadActivatorStatusTimeOutError,         "Bus read activator status timeout error" },          /**<Error message for timeout during drm bus read activator status.**/
                        { mDrmErrorBusReadActivatorChallengeTimeOutError,      "Bus read activator challenge timeout error" },       /**<Error message for timeout during drm bus read activator challenge.**/
                        { mDrmErrorBusWriteActivatorResponseTimeOutError,      "Bus write activator response timeout error" },       /**<Error message for timeout during drm bus write activator response.**/
                        { mDrmErrorBusReadInterruptTimeOutError,               "Bus read interrupt timeout error" },                 /**<Error message for timeout during drm bus read interupt.**/
                        { mDrmErrorBusReadExpectedStatusError,                 "Bus read expected status error" }                    /**<Error message for drm bus read unexpected status.**/
                    };

} // DrmControllerLibrary

#endif // __DRM_CONTROLLER_TYPES_HPP__
