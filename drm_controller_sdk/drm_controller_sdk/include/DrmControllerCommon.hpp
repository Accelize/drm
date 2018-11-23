/**
*  \file      DrmControllerCommon.hpp
*  \version   3.0.0.1
*  \date      September 2018
*  \brief     This header file contains the common definitions
*  \copyright Licensed under the Apache License, Version 2.0 (the "License");
*             you may not use this file except in compliance with the License.
*             You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*             Unless required by applicable law or agreed to in writing, software
*             distributed under the License is distributed on an "AS IS" BASIS,
*             WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*             See the License for the specific language governing permissions and
*             limitations under the License.
**/

#ifndef __DRM_CONTROLLER_COMMON_HPP__
#define __DRM_CONTROLLER_COMMON_HPP__

#include <iostream>
#include <bitset>

// version value
#define DRM_CONTROLLER_VERSION "3.0.0"
#define DRM_CONTROLLER_SDK_VERSION "3.0.0.2"

// IP component name
#define DRM_CONTROLLER_COMPONENT_NAME "DRM_CONTROLLER_WITH_DNA"      /**<Definition of the drm controller component name.**/
// IP component instance name
#define DRM_CONTROLLER_INSTANCE_NAME  "DRM_CONTROLLER_WITH_DNA_INST" /**<Definition of the drm controller instance name.**/

// Data size definitions.
#define DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE 32 /**<Definition of the system bus data size.**/
#define DRM_CONTROLLER_BYTE_SIZE 8 /**<Definition of the byte size in bits.**/
#define DRM_CONTROLLER_NIBBLE_SIZE (DRM_CONTROLLER_BYTE_SIZE/2) /**<Definition of the nibble size in bits.**/

// number of element macro
#define NBR_OF_WORDS(BUS_DATA_SIZE, REGISTER_SIZE) ((REGISTER_SIZE <= BUS_DATA_SIZE) ? 1 : \
                                                   ((REGISTER_SIZE % BUS_DATA_SIZE) == 0 ? \
                                                   (REGISTER_SIZE/BUS_DATA_SIZE) : (REGISTER_SIZE/BUS_DATA_SIZE)+1))

// Error registers number and size definitions.
#define DRM_CONTROLLER_NUMBER_OF_ERROR_REGISTERS 4            /**<Definition of the number of error registers.**/
#define DRM_CONTROLLER_SINGLE_OPERATION_ERROR_REGISTER_SIZE 8 /**<Definition of the size of a single operation error register.**/

// Version definitions.
#define DRM_CONTROLLER_VERSION_DIGIT_SIZE           2 /**<Definition of the size of digit.**/
#define DRM_CONTROLLER_VERSION_DIGIT_SIZE_BITS      (DRM_CONTROLLER_VERSION_DIGIT_SIZE*4) /**<Definition of the size of digit in bits.**/
#define DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT      3 /**<Definition of the number of digits in the version.**/
#define DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT_BITS (DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT*DRM_CONTROLLER_VERSION_DIGIT_SIZE_BITS) /**<Definition of the number of digits given in bits in the version.**/

// Number of traces per IP definition.
#define DRM_CONTROLLER_NUMBER_OF_TRACES_PER_IP 3 /**<Definition of the number of traces per IP.**/

// Size of registers
#define DRM_CONTROLLER_PAGE_SIZE                  3 /**<Definition of the register size for the page register.**/
#define DRM_CONTROLLER_COMMAND_SIZE               7 /**<Definition of the register size for the command register.**/
#define DRM_CONTROLLER_LICENSE_START_ADDRESS_SIZE 64 /**<Definition of the register size for the license start address register.**/
#define DRM_CONTROLLER_LICENSE_TIMER_SIZE         384 /**<Definition of the register size for the license timer register.**/
#define DRM_CONTROLLER_STATUS_SIZE                32 /**<Definition of the register size for the status register.**/
#define DRM_CONTROLLER_ERROR_SIZE                 DRM_CONTROLLER_NUMBER_OF_ERROR_REGISTERS*DRM_CONTROLLER_SINGLE_OPERATION_ERROR_REGISTER_SIZE /**<Definition of the register size for the error register.**/
#define DRM_CONTROLLER_DEVICE_DNA_SIZE            128 /**<Definition of the register size for the device dna register.**/
#define DRM_CONTROLLER_SAAS_CHALLENGE_SIZE        128 /**<Definition of the register size for the saas challenge register.**/
#define DRM_CONTROLLER_VERSION_SIZE               DRM_CONTROLLER_VERSION_NUMBER_OF_DIGIT_BITS /**<Definition of the register size for the version register.**/
#define DRM_CONTROLLER_VLNV_WORD_SIZE             64 /**<Definition of the register size for the vlnv word.**/
#define DRM_CONTROLLER_LICENSE_WORD_SIZE          128 /**<Definition of the register size for the license word.**/
#define DRM_CONTROLLER_TRACE_WORD_SIZE            64 /**<Definition of the register size for the trace word.**/
#define DRM_CONTROLLER_METERING_WORD_SIZE         128 /**<Definition of the register size for the metering word.**/

// Number of words per registers.
#define DRM_CONTROLLER_PAGE_WORD_NBR              NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_PAGE_SIZE) /**<Definition of the number of word in the page register.**/
#define DRM_CONTROLLER_COMMAND_WORD_NBR           NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_COMMAND_SIZE) /**<Definition of the number of word in the command register.**/
#define DRM_CONTROLLER_LIC_START_ADDRESS_WORD_NBR NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_LICENSE_START_ADDRESS_SIZE) /**<Definition of the number of word in the license start address register.**/
#define DRM_CONTROLLER_LIC_TIMER_WORD_NBR         NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_LICENSE_TIMER_SIZE) /**<Definition of the number of word in the license timer register.**/
#define DRM_CONTROLLER_STATUS_WORD_NBR            NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_STATUS_SIZE) /**<Definition of the number of word in the status register.**/
#define DRM_CONTROLLER_ERROR_WORD_NBR             NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_ERROR_SIZE) /**<Definition of the number of word in the error register.**/
#define DRM_CONTROLLER_DNA_WORD_NBR               NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_DEVICE_DNA_SIZE) /**<Definition of the number of word in the dna register.**/
#define DRM_CONTROLLER_SAAS_CHALLENGE_WORD_NBR    NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_SAAS_CHALLENGE_SIZE) /**<Definition of the number of word in the saas challenge register.**/
#define DRM_CONTROLLER_VERSION_WORD_NBR           NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_VERSION_SIZE) /**<Definition of the number of word in the version register.**/
#define DRM_CONTROLLER_VLNV_WORD_WORD_NBR         NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_VLNV_WORD_SIZE) /**<Definition of the number of word in a vlnv word.**/
#define DRM_CONTROLLER_LICENSE_WORD_WORD_NBR      NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_LICENSE_WORD_SIZE) /**<Definition of the number of word in a license word.**/
#define DRM_CONTROLLER_TRACE_WORD_WORD_NBR        NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_TRACE_WORD_SIZE) /**<Definition of the number of word in a trace word.**/
#define DRM_CONTROLLER_METERING_WORD_WORD_NBR     NBR_OF_WORDS(DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE, DRM_CONTROLLER_METERING_WORD_SIZE) /**<Definition of the number of word in a metering word.**/

// Name of the registers.
#define DRM_CONTROLLER_PAGE_REGISTER_NAME    "DrmPageRegister" /**<Definition of the name of the page register.**/
#define DRM_CONTROLLER_INDEXED_REGISTER_NAME "DrmRegisterLine" /**<Definition of the base name of indexed registers.**/

// Index of the register lines.
#define DRM_CONTROLLER_FILES_REGISTER_START_INDEX          0 /**<Definition of the start index of the files register.**/
#define DRM_CONTROLLER_COMMAND_REGISTER_INDEX              0 /**<Definition of the index of the command register.**/
#define DRM_CONTROLLER_LIC_START_ADR_REGISTER_START_INDEX  DRM_CONTROLLER_COMMAND_REGISTER_INDEX+DRM_CONTROLLER_COMMAND_WORD_NBR /**<Definition of the start index of the license start address register.**/
#define DRM_CONTROLLER_LIC_TIMER_REGISTER_START_INDEX      DRM_CONTROLLER_LIC_START_ADR_REGISTER_START_INDEX+DRM_CONTROLLER_LIC_START_ADDRESS_WORD_NBR /**<Definition of the start index of the license timer register.**/
#define DRM_CONTROLLER_STATUS_REGISTER_INDEX               DRM_CONTROLLER_LIC_TIMER_REGISTER_START_INDEX+DRM_CONTROLLER_LIC_TIMER_WORD_NBR /**<Definition of the index of the status register.**/
#define DRM_CONTROLLER_ERROR_REGISTER_INDEX                DRM_CONTROLLER_STATUS_REGISTER_INDEX+DRM_CONTROLLER_STATUS_WORD_NBR /**<Definition of the index of the error register.**/
#define DRM_CONTROLLER_DNA_REGISTER_START_INDEX            DRM_CONTROLLER_ERROR_REGISTER_INDEX+DRM_CONTROLLER_ERROR_WORD_NBR /**<Definition of the start index of the dna register.**/
#define DRM_CONTROLLER_SAAS_CHALLENGE_REGISTER_START_INDEX DRM_CONTROLLER_DNA_REGISTER_START_INDEX+DRM_CONTROLLER_DNA_WORD_NBR /**<Definition of the start index of the saas challenge register.**/
#define DRM_CONTROLLER_VERSION_REGISTER_INDEX              DRM_CONTROLLER_SAAS_CHALLENGE_REGISTER_START_INDEX+DRM_CONTROLLER_SAAS_CHALLENGE_WORD_NBR /**<Definition of the start index of the version register.**/
#define DRM_CONTROLLER_LOGS_REGISTER_START_INDEX           DRM_CONTROLLER_VERSION_REGISTER_INDEX+DRM_CONTROLLER_VERSION_WORD_NBR /**<Definition of the start index of the logs register.**/

// license file definitions.
#define DRM_CONTROLLER_LICENSE_HEADER_BLOCK_SIZE 7 /**<Definition of the number of license word used for the header block.**/
#define DRM_CONTROLLER_LICENSE_IP_BLOCK_SIZE     4 /**<Definition of the number of license word used for an ip block.**/

// Minimum size of the license file
#define DRM_CONTROLLER_MINIMUM_LICENSE_FILE_SIZE_128_BITS              (DRM_CONTROLLER_LICENSE_HEADER_BLOCK_SIZE+DRM_CONTROLLER_LICENSE_IP_BLOCK_SIZE) /**<Definition of the minimum size for license file in 128 bits words.**/
#define DRM_CONTROLLER_MINIMUM_LICENSE_FILE_SIZE_SYSTEM_BUS_DATA_SIZE  DRM_CONTROLLER_MINIMUM_LICENSE_FILE_SIZE_128_BITS*DRM_CONTROLLER_LICENSE_WORD_WORD_NBR /**<Definition of the minimum size for license file in DRM_CONTROLLER_SYSTEM_BUS_DATA_SIZE bits words.**/

// error messages
#define DRM_CONTROLLER_ERROR_HEADER       "[ERROR] : "
#define DRM_CONTROLLER_ERROR_HEADER_EMPTY "          "
#define DRM_CONTROLLER_ERROR_FOOTER       "."
// version check error messages
#define DRM_CONTROLLER_VERSION_CHECK_ERROR_HEADER           "Hardware version and software version do not match"
#define DRM_CONTROLLER_VERSION_CHECK_ERROR_HARDWARE_VERSION "Hardware version : "
#define DRM_CONTROLLER_VERSION_CHECK_ERROR_SOFTWARE_VERSION "Software version : "
#define DRM_CONTROLLER_VERSION_CHECK_ERROR_FOOTER           "Please update both hardware and software to the latest version"
// init after reset timeout error messages
#define DRM_CONTROLLER_INIT_AFTER_RESET_TIMEOUT_ERROR "DRM Controller Initialization After Reset is in timeout"
// extract dna timeout error messages
#define DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_ERROR_HEADER  "DRM Controller DNA Extraction is in timeout"
#define DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_STATUS        "DNA Ready Status Bit      : "
#define DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_ERROR_CODE    "DNA Extract Error Code    : 0x"
#define DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_ERROR_MESSAGE "DNA Extract Error Message : "
// extract vlnv timeout error messages
#define DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_ERROR_HEADER  "DRM Controller VLNV Extraction is in timeout"
#define DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_STATUS        "VLNV Ready Status Bit      : "
#define DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_ERROR_CODE    "VLNV Extract Error Code    : 0x"
#define DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_ERROR_MESSAGE "VLNV Extract Error Message : "
// license timer load timeout error messages
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_ERROR_HEADER  "DRM Controller License Timer Load is in timeout"
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_STATUS        "License Timer Loaded Status Bit  : "
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_ERROR_CODE    "License Timer Load Error Code    : 0x"
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_TIMEOUT_ERROR_MESSAGE "License Timer Load Error Message : "
// activation timeout error messages
#define DRM_CONTROLLER_ACTIVATION_TIMEOUT_ERROR_HEADER  "DRM Controller Activation is in timeout"
#define DRM_CONTROLLER_ACTIVATION_TIMEOUT_STATUS        "Activation Done Status Bit : "
#define DRM_CONTROLLER_ACTIVATION_TIMEOUT_ERROR_CODE    "Activation Error Code      : 0x"
#define DRM_CONTROLLER_ACTIVATION_TIMEOUT_ERROR_MESSAGE "Activation Error Message   : "
// license file size error messages
#define DRM_CONTROLLER_LICENSE_FILE_SIZE_ERROR_HEADER "The size of the license file is lower than the minimum required"
#define DRM_CONTROLLER_LICENSE_FILE_SIZE_REQUIRED     "Minimum license file size required (in number of 32 bits words) : "
#define DRM_CONTROLLER_LICENSE_FILE_SIZE_CURRENT      "Current license file size (in number of 32 bits words)          : "
// extract metering timeout error messages
#define DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_ERROR_HEADER "DRM Controller Metering Extraction is in timeout"
#define DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_STATUS       "Metering Ready Status Bit : "
// end session extract metering timeout error messages
#define DRM_CONTROLLER_END_SESSION_EXTRACT_METERING_TIMEOUT_ERROR_HEADER "DRM Controller End Session Metering Extraction is in timeout"
#define DRM_CONTROLLER_END_SESSION_EXTRACT_METERING_TIMEOUT_STATUS       "End Session Metering Ready Status Bit : "
// extract saas challenge timeout error messages
#define DRM_CONTROLLER_EXTRACT_SAAS_CHALLENGE_TIMEOUT_ERROR_HEADER "DRM Controller Saas Challenge Extraction is in timeout"
#define DRM_CONTROLLER_EXTRACT_SAAS_CHALLENGE_TIMEOUT_STATUS       "Saas Challenge Ready Status Bit : "
// license timer load timeout error messages
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_HEADER        "DRM Controller License Timer Load is not ready"
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_DESCRIPTION_1 "The DRM Controller might have been reseted"
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_DESCRIPTION_2 "The initialization step must be redone by calling initialize function"
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_STATUS              "License Timer Loaded Status Bit  : "
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_CODE          "License Timer Load Error Code    : 0x"
#define DRM_CONTROLLER_LICENSE_TIMER_LOAD_NOT_READY_ERROR_MESSAGE       "License Timer Load Error Message : "
// license timer disabled error message
#define DRM_CONTROLLER_LICENSE_TIMER_DISABLED_ERROR_HEADER "DRM Controller License Timer Load is disabled"
#define DRM_CONTROLLER_LICENSE_TIMER_DISABLED_STATUS       "License Timer Enabled Status Bit : "
// metering disabled error message
#define DRM_CONTROLLER_METERING_DISABLED_ERROR_HEADER "DRM Controller Metering is disabled"
#define DRM_CONTROLLER_METERING_DISABLED_STATUS       "Metering Enabled Status Bit : "

// error codes
/**
*   \enum t_drmApiErrorCode
*   \brief Enumeration for error code values.
**/
typedef enum t_drmApiErrorCode {
  mDrmApi_NO_ERROR = 0,                                    /**<No error.**/
  mDrmApi_HARDWARE_TIMEOUT_ERROR,                          /**<Hardware timeout error.**/
  mDrmApi_LICENSE_FILE_SIZE_ERROR,                         /**<License file size error.**/
  mDrmApi_VERSION_CHECK_ERROR,                             /**<Version check error.**/
  mDrmApi_LICENSE_TIMER_RESETED_ERROR,                     /**<License timer reseted error.**/
  mDrmApi_LICENSE_TIMER_DISABLED_ERROR,                    /**<License timer disabled error.**/
  mDrmApi_METERING_DISABLED_ERROR,                         /**<Metering disabled error.**/
  mDrmApi_CLIENT_LICENSE_ATTRIBUTES_ERROR,                 /**<The attributes in the license received do not match the attributes sent. */
  mDrmApi_CLIENT_HTTP_STATUS_NO_CONTENT_ERROR,             /**<An update or delete to an existing resource has been applied successfully. */
  mDrmApi_CLIENT_HTTP_STATUS_BAD_REQUEST_ERROR,            /**<The request contains invalid values. */
  mDrmApi_CLIENT_HTTP_STATUS_UNAUTHORIZED_ERROR,           /**<Authentication is required and has failed or has not been provided. */
  mDrmApi_CLIENT_HTTP_STATUS_FORBIDDEN_ERROR,              /**<The user is not allowed to perform this request. */
  mDrmApi_CLIENT_HTTP_STATUS_NOT_FOUND_ERROR,              /**<The requested resource does not exist. */
  mDrmApi_CLIENT_HTTP_STATUS_METHOD_NOT_ALLOWED_ERROR,     /**<An HTTP method is being requested that isn’t allowed for the resource or the authenticated user. */
  mDrmApi_CLIENT_HTTP_STATUS_NOT_ACCEPTABLE_ERROR,         /**<The requested format is not supported. Please use either JSON or XML depending on the requested endpoint. */
  mDrmApi_CLIENT_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE_ERROR, /**<The "Content-Type" header is incorrect or missing in the request. */
  mDrmApi_CLIENT_HTTP_STATUS_INTERNAL_SERVER_ERROR,        /**<An error occurred on the server side. If the problem persists, please contact Algodone. */
  mDrmApi_CLIENT_CURL_ERROR,                               /**<An error occurred with curl. */
  mDrmApi_CLIENT_HTTP_STATUS_UNKNOWN_ERROR                 /**<An unknow error occured. */
} t_drmApiErrorCode;

  /**
  *   \struct t_drmApiError
  *   \brief Structure containing the api error code enumeration and the api error text.
  **/
  typedef struct t_drmApiError {
    t_drmApiErrorCode mDrmApiErrorCode;
    std::string        mDrmApiErrorText;
  } t_drmApiError;

  /** mDrmApiErrorArraySize
  *   \brief Size of the array describing all api error codes enumeration and attached error text.
  **/
  const unsigned int mDrmApiErrorArraySize = 16;

  /** mDrmApiErrorArray
  *   \brief Array describing all api error codes enumeration and attached error text.
  **/
  const t_drmApiError mDrmApiErrorArray[] = {
    { mDrmApi_NO_ERROR,                                        "NO ERROR"                                        }, /**<No error.**/
    { mDrmApi_HARDWARE_TIMEOUT_ERROR,                          "HARDWARE TIMEOUT ERROR"                          }, /**<Hardware timeout error.**/
    { mDrmApi_LICENSE_FILE_SIZE_ERROR,                         "LICENSE FILE SIZE ERROR"                         }, /**<License file size error.**/
    { mDrmApi_VERSION_CHECK_ERROR,                             "VERSION CHECK ERROR"                             }, /**<Version check error.**/
    { mDrmApi_LICENSE_TIMER_RESETED_ERROR,                     "LICENSE TIMER RESETED ERROR"                     }, /**<License timer reseted error.**/
    { mDrmApi_LICENSE_TIMER_DISABLED_ERROR,                    "LICENSE TIMER DISABLED ERROR"                    }, /**<License timer disabled error.**/
    { mDrmApi_METERING_DISABLED_ERROR,                         "METERING DISABLED ERROR"                         }, /**<Metering disabled error.**/
    { mDrmApi_CLIENT_LICENSE_ATTRIBUTES_ERROR,                 "CLIENT LICENSE ATTRIBUTES ERROR"                 }, /**<The attributes in the license received do not match the attributes sent. */
    { mDrmApi_CLIENT_HTTP_STATUS_NO_CONTENT_ERROR,             "CLIENT HTTP STATUS NO CONTENT ERROR"             }, /**<An update or delete to an existing resource has been applied successfully. */
    { mDrmApi_CLIENT_HTTP_STATUS_BAD_REQUEST_ERROR,            "CLIENT HTTP STATUS BAD REQUEST ERROR"            }, /**<The request contains invalid values. */
    { mDrmApi_CLIENT_HTTP_STATUS_UNAUTHORIZED_ERROR,           "CLIENT HTTP STATUS UNAUTHORIZED ERROR"           }, /**<Authentication is required and has failed or has not been provided. */
    { mDrmApi_CLIENT_HTTP_STATUS_FORBIDDEN_ERROR,              "CLIENT HTTP STATUS FORBIDDEN ERROR"              }, /**<The user is not allowed to perform this request. */
    { mDrmApi_CLIENT_HTTP_STATUS_NOT_FOUND_ERROR,              "CLIENT HTTP STATUS NOT FOUND ERROR"              }, /**<The requested resource does not exist. */
    { mDrmApi_CLIENT_HTTP_STATUS_METHOD_NOT_ALLOWED_ERROR,     "CLIENT HTTP STATUS METHOD NOT ALLOWED ERROR"     }, /**<An HTTP method is being requested that isn’t allowed for the resource or the authenticated user. */
    { mDrmApi_CLIENT_HTTP_STATUS_NOT_ACCEPTABLE_ERROR,         "CLIENT HTTP STATUS NOT ACCEPTABLE ERROR"         }, /**<The requested format is not supported. Please use either JSON or XML depending on the requested endpoint. */
    { mDrmApi_CLIENT_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE_ERROR, "CLIENT HTTP STATUS UNSUPPORTED MEDIA TYPE ERROR" }, /**<The "Content-Type" header is incorrect or missing in the request. */
    { mDrmApi_CLIENT_HTTP_STATUS_INTERNAL_SERVER_ERROR,        "CLIENT HTTP STATUS INTERNAL SERVER ERROR"        }, /**<An error occurred on the server side. If the problem persists, please contact Algodone. */
    { mDrmApi_CLIENT_CURL_ERROR,                               "CLIENT CURL ERROR : "                            }, /**<An error occurred with curl. */
    { mDrmApi_CLIENT_HTTP_STATUS_UNKNOWN_ERROR,                "CLIENT HTTP STATUS UNKNOWN ERROR"                }  /**<An unknow error occured. */
  };

// number of micro seconds in 1 second
#define DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND           1000000 /**<Number of micro seconds in a second.**/
// timeout values
#define DRM_CONTROLLER_AUTONOMOUS_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS  30*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND /**<Timeout max value in microseconds for auto controller done operations.**/
#define DRM_CONTROLLER_EXTRACT_DNA_TIMEOUT_IN_MICRO_SECONDS            30*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND /**<Timeout max value in microseconds for dna extract operations.**/
#define DRM_CONTROLLER_EXTRACT_VLNV_TIMEOUT_IN_MICRO_SECONDS           30*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND /**<Timeout max value in microseconds for vlnv extract operations.**/
#define DRM_CONTROLLER_ACTIVATE_TIMEOUT_IN_MICRO_SECONDS               30*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND /**<Timeout max value in microseconds for activate operations.**/
#define DRM_CONTROLLER_EXTRACT_METERING_TIMEOUT_IN_MICRO_SECONDS       30*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND /**<Timeout max value in microseconds for metering extract operations.**/
#define DRM_CONTROLLER_EXTRACT_SAAS_CHALLENGE_TIMEOUT_IN_MICRO_SECONDS 30*DRM_CONTROLLER_NUMBER_OF_MICRO_SECONDS_IN_ONE_SECOND /**<Timeout max value in microseconds for saas challenge extract operations.**/

// Logs level definitions.
#define DRM_CONTROLLER_LOG_DEBUG   0 /**<Definition of log level for debug messages.**/
#define DRM_CONTROLLER_LOG_INFO    1 /**<Definition of log level for information messages.**/
#define DRM_CONTROLLER_LOG_WARNING 2 /**<Definition of log level for warning messages.**/
#define DRM_CONTROLLER_LOG_ERROR   3 /**<Definition of log level for error messages.**/
#define DRM_CONTROLLER_LOG_NONE    4 /**<Definition of log level for no message.**/

// Logs level string definitions.
#define DRM_CONTROLLER_LOG_DEBUG_STRING   "Debug"   /**<Definition of log string for message.**/
#define DRM_CONTROLLER_LOG_INFO_STRING    "Info"    /**<Definition of log string for message.**/
#define DRM_CONTROLLER_LOG_WARNING_STRING "Warning" /**<Definition of log string for message.**/
#define DRM_CONTROLLER_LOG_ERROR_STRING   "Error"   /**<Definition of log string for message.**/
#define DRM_CONTROLLER_LOG_NONE_STRING    "None"    /**<Definition of log string for message.**/

// Logs level string array.
extern const char *drmLogLevelString[]; /**<Variable for log level strings.**/

// Current log level definitions.
#define DRM_CONTROLLER_LOG_LEVEL DRM_CONTROLLER_LOG_NONE /**<Definition of the current log level.**/

// Log messages definitions.
#define DRM_CONTROLLER_LOG_MESSAGE_HEADER         " API LOG"  /**<Definition of the header message.**/
#define DRM_CONTROLLER_LOG_MESSAGE_SEPARATOR      "  -  "         /**<Definition of the seperator**/
#define DRM_CONTROLLER_LOG_MESSAGE_FILE           "File : "       /**<Definition of the file message**/
#define DRM_CONTROLLER_LOG_MESSAGE_LINE           "Line : "       /**<Definition of the line message**/
#define DRM_CONTROLLER_LOG_MESSAGE_FUNCTION       "Function : "   /**<Definition of the function message**/
#define DRM_CONTROLLER_LOG_MESSAGE_TEXT           "Message : "    /**<Definition of the message**/
#define DRM_CONTROLLER_LOG_MESSAGE_TEXT_VALUE     "Value : "      /**<Definition of the value message**/

/** DRM_CONTROLLER_LOG_MESSAGE_GENERATE_HEADER
*   \brief Generation of the message header.
*   \param[in] level is the log level.
**/
#define DRM_CONTROLLER_LOG_MESSAGE_GENERATE_HEADER(level)                                      \
  std::cout << std::endl << DRM_CONTROLLER_LOG_MESSAGE_HEADER << DRM_CONTROLLER_LOG_MESSAGE_SEPARATOR << \
  drmLogLevelString[level] << std::endl <<                                          \
  "\t\t" << DRM_CONTROLLER_LOG_MESSAGE_FILE << __FILE__ << std::endl <<                        \
  "\t\t" << DRM_CONTROLLER_LOG_MESSAGE_LINE << std::dec << __LINE__ << std::endl <<            \
  "\t\t" << DRM_CONTROLLER_LOG_MESSAGE_FUNCTION << __FUNCTION__ << std::endl;

/** DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE
*   \brief Generation of the message without a value.
*   \param[in] message is the message to display.
**/
#define DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE(message)                      \
  std::cout << "\t\t" << DRM_CONTROLLER_LOG_MESSAGE_TEXT << message << std::endl;

/** DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE_VALUE
*   \brief Generation of the message with a value.
*   \param[in] message is the message to display.
*   \param[in] value is the value to display.
**/
#define DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE_VALUE(message, value)                            \
  std::cout << "\t\t" << DRM_CONTROLLER_LOG_MESSAGE_TEXT << message << DRM_CONTROLLER_LOG_MESSAGE_SEPARATOR << \
  DRM_CONTROLLER_LOG_MESSAGE_TEXT_VALUE << std::dec << value << std::endl;

/** DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE_VALUE_HEX
*   \brief Generation of the message with a value displayed in hexadecimal.
*   \param[in] message is the message to display.
*   \param[in] value is the value to display.
**/
#define DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE_VALUE_HEX(message, value)                         \
  std::cout << "\t\t" << DRM_CONTROLLER_LOG_MESSAGE_TEXT << message << DRM_CONTROLLER_LOG_MESSAGE_SEPARATOR <<  \
  DRM_CONTROLLER_LOG_MESSAGE_TEXT_VALUE << "0x" << std::setfill ('0') << std::setw(8) << std::hex <<  \
  value << std::endl;

/** DRM_CONTROLLER_LOG_MESSAGE
*   \brief Generation of the full message without a value.
*   \param[in] level is the log level.
*   \param[in] message is the message to display.
**/
#define DRM_CONTROLLER_LOG_MESSAGE(level, message)                                                  \
  if (level >= DRM_CONTROLLER_LOG_LEVEL && DRM_CONTROLLER_LOG_LEVEL != DRM_CONTROLLER_LOG_NONE) {                       \
    DRM_CONTROLLER_LOG_MESSAGE_GENERATE_HEADER(level)                                               \
    DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE(message)                                            \
  }

/** DRM_CONTROLLER_LOG_MESSAGE_VALUE
*   \brief Generation of the full message with a value.
*   \param[in] level is the log level.
*   \param[in] message is the message to display.
*   \param[in] value is the value to display.
**/
#define DRM_CONTROLLER_LOG_MESSAGE_VALUE(level, message, value)                                     \
  if (level >= DRM_CONTROLLER_LOG_LEVEL && DRM_CONTROLLER_LOG_LEVEL != DRM_CONTROLLER_LOG_NONE) {                       \
    DRM_CONTROLLER_LOG_MESSAGE_GENERATE_HEADER(level)                                               \
    DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE_VALUE(message, value)                               \
  }

/** DRM_CONTROLLER_LOG_MESSAGE_VALUE_HEX
*   \brief Generation of the full message with a value diplayed in hexadecimal.
*   \param[in] level is the log level.
*   \param[in] message is the message to display.
*   \param[in] value is the value to display.
**/
#define DRM_CONTROLLER_LOG_MESSAGE_VALUE_HEX(level, message, value)                                 \
  if (level >= DRM_CONTROLLER_LOG_LEVEL && DRM_CONTROLLER_LOG_LEVEL != DRM_CONTROLLER_LOG_NONE) {                       \
    DRM_CONTROLLER_LOG_MESSAGE_GENERATE_HEADER(level)                                               \
    DRM_CONTROLLER_LOG_MESSAGE_GENERATE_MESSAGE_VALUE_HEX(message, value)                           \
  }

#endif // __DRM_CONTROLLER_COMMON_HPP__

