/*
Copyright (C) 2022, Accelize

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <json/version.h>
#include <future>
#include <numeric>
#include <fstream>
#include <dirent.h>

#include "accelize/drm/drm_manager.h"
#include "accelize/drm/version.h"
#include "ws_client.h"
#include "log.h"
#include "utils.h"
#include "csp.h"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "DrmControllerDataConverter.hpp"
#include "HAL/DrmControllerOperations.hpp"
#pragma GCC diagnostic pop


#define REG_FREQ_DETECTION_VERSION  0xFFF0
#define REG_FREQ_DETECTION_COUNTER_DRMACLK  0xFFF4
#define REG_FREQ_DETECTION_COUNTER_AXIACLK  0xFFF8

#define FREQ_DETECTION_VERSION_2	 0x60DC0DE0
#define FREQ_DETECTION_VERSION_3	 0x60DC0DE1

#define PAGE_INDEX_SHIFT        13
#define OFFSET_IN_PAGE_MASK     0x1FFF
#define NUM_PAGE_MAX            6

#define PNC_PAGE_SIZE               4096
#define PNC_ALLOC_SIZE              (PNC_PAGE_SIZE * 24)
#define PNC_DRM_INIT_SHM            11
#define PNC_DRM_LOG_ERROR           12
#define PNC_DRM_LOG_WARN            13
#define PNC_DRM_LOG_INFO            14
#define PNC_DRM_LOG_DEBUG           15
#define PNC_DRM_LOG_TRACE1          16
#define PNC_DRM_LOG_TRACE2          17


#define TRY try {

#define CATCH_AND_THROW                                                           \
        sLogger->flush();                                                         \
    } catch( const Exception& e ) {                                               \
        DRM_ErrorCode errcode = e.getErrCode();                                   \
        if ( errcode != DRM_Exit ) {                                              \
            std::string errmsg = std::string( e.what() );                         \
            if ( ( errcode == DRM_WSMayRetry ) || ( errcode == DRM_WSTimedOut ) ) \
                errmsg += DRM_CONNECTION_ERROR_MESSAGE;                           \
            Fatal( errmsg );                                                      \
            sLogger->flush();                                                     \
            f_asynch_error( errmsg );                                             \
        }                                                                         \
        throw;                                                                    \
    } catch( const std::exception &e ) {                                          \
        Fatal( e.what() );                                                        \
        sLogger->flush();                                                         \
        f_asynch_error( e.what() );                                               \
        throw;                                                                    \
    }


#define MTX_ACQUIRE( mtx ) {                               \
    Debug( "Waiting mutex {} from {}", #mtx, __func__ );  \
    std::lock_guard<std::mutex> lock( mtx );               \
    Debug( "Acquired mutex {} from {}", #mtx, __func__ )

#define MTX_RELEASE( mtx ) }                               \
    Debug( "Released mutex {} from {}", #mtx, __func__ )


pnc_session_t *Accelize::DRM::DrmManager::s_pnc_session = nullptr;
uint32_t *Accelize::DRM::DrmManager::s_pnc_tzvaddr = nullptr;
size_t Accelize::DRM::DrmManager::s_pnc_tzsize = 0;
uint32_t Accelize::DRM::DrmManager::s_pnc_page_offset = 0;

const std::string Accelize::DRM::DrmManager::DRM_SELF_TEST_ERROR_MESSAGE = std::string(
        "Could not access DRM Controller registers.\nPlease verify:\n"
        "\t-The read/write callbacks implementation in the host application: verify it uses the correct offset address of DRM Controller IP in the design address space.\n"
        "\t-The DRM Controller IP instantiation in the FPGA design: verify the correctness of 16-bit address received by the AXI-Lite port of the DRM Controller.\n" );

const std::string Accelize::DRM::DrmManager::DRM_CONNECTION_ERROR_MESSAGE = std::string(
        "\n!!! The issue could either be caused by a networking problem, by a firewall or NAT blocking incoming traffic or by a wrong server address. "
        "Please verify your configuration and try again !!!\n" );

const std::string Accelize::DRM::DrmManager::DRM_CTRL_TA_INIT_ERROR_MESSAGE = std::string(
        "Please verify:\n"
        "\t- the DRM Controller instance in the PL is at the right offset address.\n"
        "\t- the PUF has been registered.\n" );

const std::string Accelize::DRM::DrmManager::DRM_DOC_LINK = std::string(
        "https://tech.accelize.com/documentation/stable");


namespace Accelize {
namespace DRM {

const char* getApiVersion() {
    return DRMLIB_VERSION;
}

class DRM_LOCAL DrmManager::Impl {

private:

    // DRM Controller TA communication callbacks

    // Read Callback Function
    int32_t pnc_read_drm_ctrl_ta( uint32_t addr, uint32_t *value ) {
        uint32_t page_index = addr >> PAGE_INDEX_SHIFT;
        if (page_index >= NUM_PAGE_MAX)
            Throw( DRM_Fatal, "Attempting to read invalid page index {} of the DRM Controller. ", page_index );
        uint32_t offset_in_page = (addr & OFFSET_IN_PAGE_MASK) >> 2;
        s_pnc_page_offset = s_pnc_tzvaddr[page_index];
        *value = *(s_pnc_tzvaddr + s_pnc_page_offset + offset_in_page);
        return 0;
    }

    // Write Callback Function
    int32_t pnc_write_drm_ctrl_ta( uint32_t addr, uint32_t value ) {
        uint32_t page_index = addr >> PAGE_INDEX_SHIFT;
        if (page_index >= NUM_PAGE_MAX)
            Throw( DRM_Fatal, "Attempting to read invalid page index {} of the DRM Controller. ", page_index );
        uint32_t offset_in_page = (addr & OFFSET_IN_PAGE_MASK) >> 2;
        s_pnc_page_offset = s_pnc_tzvaddr[page_index];
        *(s_pnc_tzvaddr + s_pnc_page_offset + offset_in_page) = value;
        return 0;
    }

    bool pnc_initialize_drm_ctrl_ta() const {
        int err = 0;
        if ( s_pnc_session != nullptr) {
            Debug( "Found and reuse an existing ProvenCore session. " );
            return true;
        }
        err = pnc_session_new(PNC_ALLOC_SIZE, &s_pnc_session);
        Debug( "pnc_session_new returned {}", err );
        if ( err == -ENODEV ) {
            Debug( "ProvenCore driver is not loaded" );
            return false;
        }
        if ( err < 0 ) {
            Throw( DRM_PncInitError, "Failed to open TrustZone module: {}. ", strerror(errno) );
        }
        Debug( "ProvenCore session created. " );
        try {
            int ret = 0;
            for (int timeout = 10; timeout > 0; timeout--) {
                ret = pnc_session_config_by_name(s_pnc_session, "drm_controller_rs");
                Debug( "pnc_session_config_by_name returned {}", ret );
                if (ret < 0) {
                    if (errno == EAGAIN) {
                        sleep(1);
                        continue;
                    }
                    Throw( DRM_PncInitError, "Failed to configure ProvenCore for DRM Controller TA: {}. ",
                        strerror(errno) );
                }
                break;
            }
            if (ret < 0) {
                Throw( DRM_PncInitError, "Failed to allocate resource for DRM Controller TA: Timeout. " );
            }
            Debug( "ProvenCore session configured for DRM Controller TA. " );

            // get virtual address and size of shared memory region
            ret = pnc_session_getinfo(s_pnc_session, (void**)&s_pnc_tzvaddr, &s_pnc_tzsize);
            Debug( "pnc_session_getinfo returned {}", ret );
            if ( ret < 0) {
                Throw( DRM_PncInitError, "Failed to get information from DRM Controller TA: {}. ",
                    strerror(errno) );
            }
            if (s_pnc_tzvaddr == NULL) {
                Throw( DRM_PncInitError, "Failed to create shared memory for DRM Controller TA: {}. ",
                    strerror(errno) );
            }
            Debug( "DRM Controller TA information collected. " );

            // Request initialization of the Drm Controller Trusted App
            uint32_t response = 0;
            ret = pnc_session_send_request_and_wait_response(s_pnc_session, PNC_DRM_INIT_SHM, 0, &response);
            Debug( "pnc_session_send_request_and_wait_response returned {} with response {}", ret, response );
            if ( (ret < 0) || (response != 0) ) {
                std::string msg = fmt::format( "Failed to initialize DRM Controller TA: retcode={}, stderr={} / response={}. ", ret, strerror(errno), response );
                msg += DRM_CTRL_TA_INIT_ERROR_MESSAGE;
                msg += fmt::format(
                    "For more details refer to the online documentation: {}/drm_hardware_integration.html#xilinx-r-som-boards",
                    DRM_DOC_LINK );
                Throw( DRM_PncInitError, msg );
            }
            Debug( "DRM Controller TA initialized and ready to operate." );
            return true;
        }
        catch( const Exception &e ) {
            pnc_session_destroy(s_pnc_session);
            Debug( "pnc_session_send_request_and_wait_response returned" );
            s_pnc_session = nullptr;
            throw;
        }
    }

    void pnc_uninitialize_drm_ctrl_ta() const {
        pnc_session_destroy( s_pnc_session );
        s_pnc_session = nullptr;
        Debug( "Provencore session closed. " );
    }


protected:

    // Enum
    enum class eLogFileType: uint8_t {NONE=0, BASIC, ROTATING};
    enum class eLicenseType: uint8_t {METERED, NODE_LOCKED, NONE};
    enum class eMailboxOffset: uint8_t {MB_LOCK_DRM=0, MB_CUSTOM_FIELD, MB_USER};
    enum class eHostDataVerbosity: uint8_t {FULL=0, PARTIAL, NONE};
    enum class eCtrlLogVerbosity: uint8_t {ERROR=0, WARN, INFO, DEBUG, TRACE1, TRACE2};

    // Design constants
    const uint32_t HDK_COMPATIBILITY_LIMIT_MAJOR = 3;
    const uint32_t HDK_COMPATIBILITY_LIMIT_MINOR = 2;

    const std::map<eLicenseType, std::string> LicenseTypeStringMap = {
            {eLicenseType::NONE       , "Idle"},
            {eLicenseType::METERED    , "Floating/Metering"},
            {eLicenseType::NODE_LOCKED, "Node-Locked"}
    };

    uint32_t SDK_CTRL_TIMEOUT_IN_US = 2000000;
    uint32_t SDK_CTRL_SLEEP_IN_US = 1000;

    const std::map<eCtrlLogVerbosity, uint32_t> LogCtrlLevelMap = {
            {eCtrlLogVerbosity::ERROR  , PNC_DRM_LOG_ERROR},
            {eCtrlLogVerbosity::WARN   , PNC_DRM_LOG_WARN},
            {eCtrlLogVerbosity::INFO   , PNC_DRM_LOG_INFO},
            {eCtrlLogVerbosity::DEBUG  , PNC_DRM_LOG_DEBUG},
            {eCtrlLogVerbosity::TRACE1 , PNC_DRM_LOG_TRACE1},
            {eCtrlLogVerbosity::TRACE2 , PNC_DRM_LOG_TRACE2}
    };

    // DNA & Product
    std::string mDeviceID;
    std::string mProductID;

    // HDK version
    uint32_t mDrmVersionNum = 0;
    std::string mDrmVersionStr;

    // Composition
    std::unique_ptr<DrmWSClient> mWsClient;
    std::unique_ptr<DrmControllerLibrary::DrmControllerOperations> mDrmController;
    mutable std::recursive_mutex mDrmControllerMutex;
    bool mIsLockedToDrm = false;

    // Logging parameters
    spdlog::level::level_enum sLogConsoleVerbosity = spdlog::level::err;
    std::string sLogConsoleFormat = std::string("[%^%=8l%$] %-6t, %v");

    spdlog::level::level_enum sLogFileVerbosity = spdlog::level::warn;
    std::string  sLogFilePath         = fmt::format( "accelize_drmlib_{}.log", getpid() );
    std::string  sLogFileFormat       = std::string("%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v");
    eLogFileType sLogFileType         = eLogFileType::NONE;
    bool         sLogFileAppend       = false;
    size_t       sLogFileRotatingSize = 100*1024; ///< Size max in KBytes of the log roating file
    size_t       sLogFileRotatingNum  = 3;

    eCtrlLogVerbosity sLogCtrlVerbosity = eCtrlLogVerbosity::ERROR;

    // Function callbacks
    DrmManager::ReadRegisterCallback  f_read_register = nullptr;
    DrmManager::WriteRegisterCallback f_write_register = nullptr;
    DrmManager::AsynchErrorCallback   f_asynch_error = nullptr;

    // Settings files
    std::string mConfFilePath;
    std::string mCredFilePath;

    // Node-Locked parameters
    std::string mNodeLockLicenseDirPath;
    std::string mNodeLockRequestFilePath;
    std::string mNodeLockLicenseFilePath;

    // License related properties
    uint32_t mWSRetryPeriodLong  = 60;    ///< Time in seconds before the next request attempt to the Web Server when the time left before timeout is large
    uint32_t mWSRetryPeriodShort = 2;     ///< Time in seconds before the next request attempt to the Web Server when the time left before timeout is short
    uint32_t mWSApiRetryDuration = 60;    ///< Period of time in seconds during which retries occur on activate and deactivate functions

    eLicenseType mLicenseType = eLicenseType::METERED;
    uint32_t mLicenseDuration = 0;        ///< Time duration in seconds of the license

    uint32_t mActivationTransmissionTimeoutMS = 0;  ///< Timeout in milliseconds to complete the transmission of the activation code to the Activator's interface

    uint32_t mCtrlTimeoutInUS = 0;
    uint32_t mCtrlSleepInUS = 0;
    uint32_t mCtrlTimeFactor = 1;

    // To protect access to the metering data (to securize the segment ID check in HW)
    mutable std::mutex mMeteringAccessMutex;

    // Operating mode
    bool mIsHybrid = false;
    bool mIsPnR = false;

    // DRM Frequency parameters
    int32_t mFrequencyInit = 0;
    double mFrequencyCurr = 0.0;
    uint32_t mFrequencyDetectionPeriod = 100;       // in milliseconds
    double mFrequencyDetectionThreshold = 10.0;     // Error in percentage
    uint8_t mFreqDetectionMethod = 0;
    bool mBypassFrequencyDetection = false;
    uint32_t mAxiFrequency = 0;

    // Session state
    std::string mSessionID;
    std::string mEntitlementID;

    // Web service communication
    Json::Value mHeaderJsonRequest;

    // Health/Asynchronous metering
    uint32_t mHealthPeriod;                 ///< Time in seconds before performing the next health request
    uint32_t mHealthRetryTimeout;           ///< Timeout in seconds for the health request
    uint32_t mHealthRetrySleep;             ///< Time in seconds before perforing a new health retry
    mutable uint32_t mHealthCounter = 0;
    mutable std::mutex mHealthAccessMutex;  ///< To protect access to the health parameters
    std::future<void> mHealthThread;        ///< Thread to maintain health alive

    // Thread to maintain license alive
    uint32_t mLicenseCounter = 0;
    std::future<void> mLicenseThread;
    TClock::time_point mExpirationTime;

    // Threads exit elements
    bool mSecurityStop{false};
    std::mutex mThreadExitMtx;
    std::condition_variable mThreadExitCondVar;
    bool mThreadExit{false};

    // XRT PATH
    std::string mXrtPath;
    std::string mXbutil;

    Json::Value mDiagnostics = Json::nullValue;
    eHostDataVerbosity mHostDataVerbosity = eHostDataVerbosity::PARTIAL;
    Json::Value mSettings = Json::nullValue;
    Json::Value mMailboxRoData = Json::nullValue;

    // Debug parameters
    spdlog::level::level_enum mDebugMessageLevel;

    // User accessible parameters
    const std::map<ParameterKey, std::string> mParameterKeyMap = {
    #   define PARAMETERKEY_ITEM(id) {id, #id},
    #   include "accelize/drm/ParameterKey.def"
    #   undef PARAMETERKEY_ITEM
        {ParameterKeyCount, "ParameterKeyCount"}
    };


    #define checkDRMCtlrRet( func ) {                                                           \
        unsigned int errcode = DRM_OK;                                                          \
        try {                                                                                   \
            std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );                  \
            errcode = func;                                                                     \
            Debug( "{} returned {}", #func, errcode );                                          \
            if ( errcode ) {                                                                    \
                logDrmCtrlError();                                                              \
                logDrmCtrlTrngStatus();                                                         \
                Error( "{} failed with error code {}", #func, errcode );                        \
                Throw( DRM_CtlrError, "{} failed with error code {}. ", #func, errcode );       \
            }                                                                                   \
        } catch( const std::exception &e ) {                                                    \
            Error( "{} threw an exception: {}", #func, e.what() );                              \
            logDrmCtrlError();                                                                  \
            logDrmCtrlTrngStatus();                                                             \
            Throw( DRM_CtlrError, e.what() );                                                   \
        }                                                                                       \
    }


    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path )
    {
        // Basic logging setup
        initLog();

        Json::Value conf_json;

        mSecurityStop = false;
        mIsLockedToDrm = false;

        mLicenseDuration = 0;
        mLicenseCounter = 0;
        mHealthCounter = 0;

        mConfFilePath = conf_file_path;
        mCredFilePath = cred_file_path;

        mFrequencyInit = 0;
        mFrequencyCurr = 0.0;

        mIsHybrid = false;
        mIsPnR = false;

        mDebugMessageLevel = spdlog::level::trace;

        // Define default asynchronous error callback
        f_asynch_error = [](std::string msg) { std::cerr << "ERROR: " << msg << std::endl; };

        // Parse configuration file
        conf_json = parseJsonFile( conf_file_path );

        try {
            Json::Value param_lib = JVgetOptional( conf_json, "settings", Json::objectValue );
            if ( param_lib != Json::nullValue ) {
                // Console logging
                sLogConsoleVerbosity = static_cast<spdlog::level::level_enum>( JVgetOptional(
                        param_lib, "log_verbosity", Json::uintValue, (uint32_t)sLogConsoleVerbosity ).asUInt());
                sLogConsoleFormat = JVgetOptional(
                        param_lib, "log_format", Json::stringValue, sLogConsoleFormat ).asString();

                // File logging
                sLogFileVerbosity = static_cast<spdlog::level::level_enum>( JVgetOptional(
                        param_lib, "log_file_verbosity", Json::uintValue, (uint32_t)sLogFileVerbosity ).asUInt() );
                sLogFileFormat = JVgetOptional(
                        param_lib, "log_file_format", Json::stringValue, sLogFileFormat ).asString();
                sLogFilePath = JVgetOptional(
                        param_lib, "log_file_path", Json::stringValue, sLogFilePath ).asString();
                sLogFileType = static_cast<eLogFileType>( JVgetOptional(
                        param_lib, "log_file_type", Json::uintValue, (uint32_t)sLogFileType ).asUInt() );
                sLogFileAppend = JVgetOptional(
                        param_lib, "log_file_append", Json::booleanValue, sLogFileAppend ).asBool();
                sLogFileRotatingSize = JVgetOptional( param_lib, "log_file_rotating_size",
                        Json::uintValue, (uint32_t)sLogFileRotatingSize ).asUInt();
                sLogFileRotatingNum = JVgetOptional( param_lib, "log_file_rotating_num",
                        Json::uintValue, (uint32_t)sLogFileRotatingNum ).asUInt();

                // Software Controller logging
                sLogCtrlVerbosity = static_cast<eCtrlLogVerbosity>( JVgetOptional(
                        param_lib, "log_ctrl_verbosity", Json::uintValue, (uint32_t)sLogCtrlVerbosity ).asUInt() );

                // Frequency detection
                mFrequencyDetectionPeriod = JVgetOptional( param_lib, "frequency_detection_period",
                        Json::uintValue, mFrequencyDetectionPeriod).asUInt();
                mFrequencyDetectionThreshold = JVgetOptional( param_lib, "frequency_detection_threshold",
                        Json::uintValue, mFrequencyDetectionThreshold).asDouble();

                // Retry parameters
                mWSRetryPeriodLong = JVgetOptional( param_lib, "ws_retry_period_long",
                        Json::uintValue, mWSRetryPeriodLong).asUInt();
                mWSRetryPeriodShort = JVgetOptional( param_lib, "ws_retry_period_short",
                        Json::uintValue, mWSRetryPeriodShort).asUInt();
                mWSApiRetryDuration = JVgetOptional( param_lib, "ws_api_retry_duration",
                        Json::uintValue, mWSApiRetryDuration).asUInt();

                // Host and Card information
                mHostDataVerbosity = static_cast<eHostDataVerbosity>( JVgetOptional(
                        param_lib, "host_data_verbosity", Json::uintValue, (uint32_t)mHostDataVerbosity ).asUInt() );
            }
            mHealthPeriod = 0;
            mHealthRetryTimeout = 0;
            mHealthRetrySleep = 0;

            // Customize logging configuration
            updateLog();

            if ( mWSRetryPeriodLong <= mWSRetryPeriodShort )
                Throw( DRM_BadArg, "ws_retry_period_long ({} sec) must be greater than ws_retry_period_short ({} sec). ",
                        mWSRetryPeriodLong, mWSRetryPeriodShort );

            // Licensing configuration
            Json::Value conf_licensing = JVgetRequired( conf_json, "licensing", Json::objectValue );
            // Get licensing mode
            bool is_nodelocked = JVgetOptional( conf_licensing, "nodelocked", Json::booleanValue, false ).asBool();
            if ( is_nodelocked ) {
                // If this is a node-locked license, get the license path
                mNodeLockLicenseDirPath = JVgetRequired( conf_licensing, "license_dir", Json::stringValue ).asString();
                mLicenseType = eLicenseType::NODE_LOCKED;
                Debug( "Configuration file specifies a Node-locked license" );
            } //else {
                Debug( "Configuration file specifies a floating/metered license" );
                // Get DRM frequency related parameters
                Json::Value conf_drm = JVgetRequired( conf_json, "drm", Json::objectValue );
                mFrequencyInit = JVgetRequired( conf_drm, "frequency_mhz", Json::intValue ).asUInt();
                mFrequencyCurr = double(mFrequencyInit);
                mBypassFrequencyDetection = JVgetOptional( conf_drm, "bypass_frequency_detection", Json::booleanValue,
                        mBypassFrequencyDetection ).asBool();
                mIsHybrid = JVgetOptional( conf_drm, "drm_software", Json::booleanValue, false ).asBool();
//            }
        } catch( const Exception &e ) {
            if ( e.getErrCode() != DRM_BadFormat )
                throw;
            Throw( DRM_BadFormat, "Error in configuration file '{}: {}. ", conf_file_path, e.what() );
        }
    }

    void initLog() {
        try {
            std::vector<spdlog::sink_ptr> sinks;

            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level( sLogConsoleVerbosity );
            console_sink->set_pattern( sLogConsoleFormat );
            sinks.push_back( console_sink );

            sLogger = std::make_shared<spdlog::logger>( "drmlib_logger", sinks.begin(), sinks.end() );
            sLogger->set_level( sLogConsoleVerbosity );
            spdlog::set_default_logger( sLogger );

            //spdlog::flush_on(spdlog::level::trace); TO UNCOMMENT ONLY FOR DEBUG
        }
        catch( const spdlog::spdlog_ex& ex ) { //LCOV_EXCL_LINE
            std::cerr << "Failed to initialize logging: " << ex.what() << std::endl; //LCOV_EXCL_LINE
        }
    }

    void createFileLog( const std::string file_path, const eLogFileType type,
            const spdlog::level::level_enum level, const std::string format,
            const size_t rotating_size, const size_t rotating_num, const bool file_append ) {
        spdlog::sink_ptr log_sink;
        std::string version_list = fmt::format( "Installed versions:\n\t-drmlib: {}\n\t-libcurl: {}\n\t-jsoncpp: {}\n\t-spdlog: {}.{}.{}",
                DRMLIB_VERSION, curl_version(), JSONCPP_VERSION_STRING,
                SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH );

        if ( type == eLogFileType::NONE )
            log_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
        else {
            // Check if parent directory exists
            std::string parentDir = getDirName( file_path );
            if ( !makeDirs( parentDir ) ) {
                Throw( DRM_ExternFail, "Failed to create log file {}. ", file_path );
            }
            if ( type == eLogFileType::BASIC )
                log_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                        file_path, !file_append);
            else // type == eLogFileType::ROTATING
                log_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                        file_path, rotating_size*1024, rotating_num);
        }
        log_sink->set_pattern( format );
        log_sink->set_level( spdlog::level::info );
        log_sink->log( spdlog::details::log_msg( "", spdlog::level::info, version_list ) );
        log_sink->set_level( level );
        sLogger->sinks().push_back( log_sink );
        if ( level < sLogger->level() )
            sLogger->set_level( level );
        if ( type != eLogFileType::NONE )
            Debug( "Created log file '{}' of type {}, with verbosity {}", file_path, (int)type, (int)level );
    }

    void updateLog() {
        try {
            auto console_sink = sLogger->sinks()[0];
            console_sink->set_level( sLogConsoleVerbosity );
            console_sink->set_pattern( sLogConsoleFormat );
            if ( sLogConsoleVerbosity < sLogger->level() )
                sLogger->set_level( sLogConsoleVerbosity );

            // File logging
            createFileLog( sLogFilePath, sLogFileType, sLogFileVerbosity, sLogFileFormat,
                    sLogFileRotatingSize, sLogFileRotatingNum, sLogFileAppend );
        }
        catch( const spdlog::spdlog_ex& ex ) {  //LCOV_EXCL_LINE
            std::cerr << "Failed to update logging settings: " << ex.what() << std::endl; //LCOV_EXCL_LINE
        }
    }

    void checkCtrlLogLevel( eCtrlLogVerbosity level_e ) {
        if ( LogCtrlLevelMap.find( level_e ) == LogCtrlLevelMap.end() ) {
            Throw( DRM_BadArg, "Invalid log level for SW Controller: {}", (uint32_t)level_e );
        }
    }

    void updateCtrlLogLevel( eCtrlLogVerbosity level_e, bool force = false ) {
        if ( !mIsPnR ) {
            Warning( "This command has no effect on HW DRM Controller IP" );
            return;
        }
        checkCtrlLogLevel( level_e );
        if ( force || ( level_e != sLogCtrlVerbosity ) ) {
            uint32_t level_id = LogCtrlLevelMap.find( level_e )->second;
            if ( pnc_session_request(s_pnc_session, level_id, 0) < 0) {
                Throw( DRM_PncInitError, "Failed to set the log level of the DRM Controller TA to {}: {}. ",
                        level_id, strerror(errno) );
            }
            Debug( "Updated log level for SW Controller from {} to {}", (uint32_t)sLogCtrlVerbosity, (uint32_t)level_e );
            sLogCtrlVerbosity = level_e;
        } else {
            Debug( "Log level for SW Controller is already set to {}", (uint32_t)sLogCtrlVerbosity );
        }
    }

    void getDrmCtrlError( uint8_t& activation_error, uint8_t& dna_error, uint8_t& vlnv_error, uint8_t& license_error ) const {
        getDrmController().readActivationErrorRegister( activation_error );
        getDrmController().readExtractDnaErrorRegister( dna_error );
        getDrmController().readExtractVlnvErrorRegister( vlnv_error );
        getDrmController().readLicenseTimerLoadErrorRegister( license_error );
    }

    void logDrmCtrlError() const {
        uint8_t activation_error=0, dna_error=0, vlnv_error=0, license_error=0;
        getDrmCtrlError( activation_error, dna_error, vlnv_error, license_error );
        Debug( "Controller Activation error register: 0x{:02x}", activation_error );
        Debug( "Controller DNA error register: 0x{:02x}", dna_error );
        Debug( "Controller VLNV error register: 0x{:02x}", vlnv_error );
        Debug( "Controller License Timer error register: 0x{:02x}", license_error );
    }

    void getTrngStatus( bool securityAlertBit, uint32_t& adaptiveProportionTestError,
                        uint32_t& repetitionCountTestError ) const {
        auto drmMajor = ( mDrmVersionNum >> 16 ) & 0xFF;
        auto drmMinor = ( mDrmVersionNum >> 8  ) & 0xFF;
        if ( ( drmMajor > 4 ) || ( ( drmMajor == 4 ) && ( drmMinor >= 2 ) ) ) {
            checkDRMCtlrRet( getDrmController().readSecurityAlertStatusRegister( securityAlertBit ) );
            checkDRMCtlrRet( getDrmController().extractAdaptiveProportionTestFailures( adaptiveProportionTestError ) );
            checkDRMCtlrRet( getDrmController().extractRepetitionCountTestFailures( repetitionCountTestError ) );
        } else {
            Debug( "TRNG status bits are not supported in this HDK version." );
        }
    }

    void logDrmCtrlTrngStatus() const {
        bool securityAlertBit( false );
        uint32_t adaptiveProportionTestError, repetitionCountTestError;
        getTrngStatus( securityAlertBit, adaptiveProportionTestError, repetitionCountTestError );
        Debug( "Controller TRNG status: security alert bit = {}, adaptative proportion test error = {}, repetition count test error = {}",
                securityAlertBit, adaptiveProportionTestError, repetitionCountTestError );
    }

    Json::Value detectBoards() {
        Json::Value devices;
        // Get list of known boards
/*        std::string suburl = "/customer/boards";
        std::string response = getDrmWSClient().sendSaasRequest( suburl,
                    tHttpRequestType::GET, Json::nullValue );
        Json::Value known_boards = parseJsonString( response );*/
        Json::Value known_boards;
        known_boards["0x10ee"] = Json::nullValue;
        known_boards["0x1d0f"].append("0x1041");
        known_boards["0x1d0f"].append("0xcd01");

        // Gather detected boards
        std::string sys_path = "/sys/bus/pci/devices/";
        for( const auto &entry: listDir( sys_path ) ) {
            std::string vendor = rtrim( readFile( sys_path + entry + "/vendor" ) );
            std::string device = rtrim( readFile( sys_path + entry + "/device" ) );
            if ( ( mHostDataVerbosity == eHostDataVerbosity::FULL ) ||
                known_boards.isMember(vendor) ) {
                if ( !devices.isMember( vendor ) ) {
                    devices[vendor] = Json::arrayValue;
                }
            }
            if ( devices.isMember(vendor) ) {
                for( const auto& known_device: known_boards[vendor] ) {
                    if ( ( mHostDataVerbosity == eHostDataVerbosity::FULL ) ||
                         ( known_device == device ) ) {
                        devices[vendor].append( device );
                    }
                }
            }
        }
        Debug( "Listing devices on PCIe tree: {}", devices.toStyledString() );
        return devices;
    }

    bool findXrtUtility() {
        // Check XILINX_XRT environment variable existence
        char* env_val = getenv( "XILINX_XRT" );
        if (env_val == NULL) {
            Debug( "XILINX_XRT variable is not defined" );
            mXrtPath.clear();
            return false;
        }
        mXrtPath = std::string( env_val );
        Debug( "XILINX_XRT variable is defined: {}", mXrtPath );

        // Check xbutil existence
        std::string xrt_bin_dir = fmt::format( "{}{}bin", mXrtPath, PATH_SEP );
        mXbutil = fmt::format( "{}{}xbutil", xrt_bin_dir, PATH_SEP );
        if ( !isFile( mXbutil ) ) {
            // If xbutil does not exist
            Debug( "xbutil tool could not be found in {}", xrt_bin_dir );
            mXbutil.clear();
            return false;
        }
        Debug( "xbutil tool has been found in {}", mXbutil );

        return true;
    }

    bool getXrtPlatformInfoV1( Json::Value& hostcard_node ) {
        Debug( "Attempt to gather host and card information with XRT method 1" );
        std::string cmd_out;
        Json::Value details_json;

        details_json["method"] = 1;
//TODO: solve the extra fields not permitted        hostcard_node["xrt_details"].append(details_json);

        // Call xbutil to collect host and card data
        try {
            std::string cmd = fmt::format( "{} dump", mXbutil );
            cmd_out = execCmd( cmd );
        } catch( const std::exception &e ) {
            details_json["msg"] = fmt::format( "Host and card information not collected with XRT method 1: execution error '{}'\n",
                                    e.what() );
            return false;
        }
        // Parse collected data and save to header
        Json::Value xbutil_json;
        try {
            xbutil_json = parseJsonString( cmd_out );
        } catch( const std::exception &e ) {
            details_json["msg"] = fmt::format( "Host and card information not collected with XRT method 1: error '{}' parsing:\n{}\n",
                                    e.what(), cmd_out );
            return false;
        }
        // Filtering meta-data
        if ( mHostDataVerbosity == eHostDataVerbosity::FULL ) {
            // Verbosity is FULL
            details_json = xbutil_json;
            details_json["method"] = 1;
        } else {
            // Verbosity is PARTIAL
            for(Json::Value::iterator itr=xbutil_json.begin(); itr!=xbutil_json.end(); ++itr) {
                std::string key = itr.key().asString();
                try {
                    if (   ( key == "version" )
                        || ( key == "system" )
                        || ( key == "runtime" )
                       )
                        details_json[key] = *itr;
                    else if ( key == "board" ) {
                        //  Add general info node
                        details_json[key]["info"] = itr->get("info", Json::nullValue);
                        // Try to get the number of kernels
                        Json::Value compute_unit_node = itr->get("compute_unit", Json::nullValue);
                        if ( compute_unit_node != Json::nullValue )
                            details_json[key]["compute_unit"] = compute_unit_node.size();
                        else
                            details_json[key]["compute_unit"] = -1;
                        // Add XCLBIN UUID
                        details_json[key]["xclbin"] = itr->get("xclbin", Json::nullValue);
                    }
                } catch( const std::exception &e ) {
                    Debug( "Could not extract Host Information for key {}", key );
                }
            }
        }
        Debug( "Succeeded to gather host and card information with XRT method 1" );
        return true;
    }

    bool getXrtPlatformInfoV2( Json::Value& hostcard_node ) {
        Debug( "Attempt to gather host and card information with XRT method 2" );
        std::string xbutil_log("xbutil.log");
        std::string cmd_out;
        Json::Value details_json;

        details_json["method"] = 2;

        // Call xbutil to examine the platform
        try {
            std::string cmd = fmt::format( "{} examine -f JSON -o {} --force", mXbutil, xbutil_log );
            cmd_out = execCmd( cmd );
        } catch( const std::exception &e ) {
            details_json["msg"] = fmt::format( "Error executing XRT command: {}\n", e.what() );
//TODO: solve the extra fields not permitted            hostcard_node["xrt_details"].append(details_json);
            return false;
        }
        // Parse available devices
        Json::Value xbutil_json;
        try {
            xbutil_json = parseJsonFile( xbutil_log );
        } catch( const std::exception &e ) {
            details_json["msg"] = fmt::format( "Error parsing XRT global data file {}: {}.\n",
                                                xbutil_log, e.what() );
//TODO: solve the extra fields not permitted            hostcard_node["xrt_details"].append(details_json);
            return false;
        }
        removeFile( xbutil_log );
        // Collect XRT meta-data
        hostcard_node["device_driver_version"] = xbutil_json["system"]["host"]["xrt"]["version"];
        // Filtering meta-data
        if ( mHostDataVerbosity == eHostDataVerbosity::FULL ) {
            // Verbosity is FULL
            details_json = xbutil_json;
            details_json["method"] = 2;
        } else {
            // Verbosity is PARTIAL
            details_json = xbutil_json["system"]["host"]["xrt"];
            details_json["method"] = 2;
        }
//TODO: solve the extra fields not permitted        hostcard_node["xrt_details"].append(details_json);
        return true;
    }

    void getHostAndCardInfo() {

        Debug( "Host and card information verbosity: {}", static_cast<uint32_t>( mHostDataVerbosity ) );

        // Depending on the host data verbosity
        if ( mHostDataVerbosity == eHostDataVerbosity::NONE ) {
            return;
        }

//TODO: solve the extra fields not permitted        mDiagnostics["pci_devices"] = detectBoards();

        // Get host info
        std::string os_version = rtrim( execCmd( "grep -Po 'PRETTY_NAME=\"\\K[^\"]+' /etc/os-release" ) );
        std::string kernel_version = rtrim( execCmd( "uname -r" ) );
        std::string cpu_version = rtrim( execCmd( "uname -m" ) );

        // Fulfill with DRM section
        mDiagnostics["drm_library_version"] = DRMLIB_VERSION;
        mDiagnostics["os_version"] = os_version;
        mDiagnostics["os_kernel_version"] = kernel_version;
        mDiagnostics["cpu_architecture"] = cpu_version;
        if ( mMailboxRoData.isMember( "extra" ) )
            mDiagnostics["drm_controller_version"] = mMailboxRoData["extra"]["lgdn_full_version"];

        // Gather host and card information if xbutil existing
        if ( findXrtUtility() ) {
            Json::Value hostcard_node = Json::nullValue;
            if ( !getXrtPlatformInfoV2( mDiagnostics ) ) {
                getXrtPlatformInfoV1( mDiagnostics );
            }
        }

        Debug( "Diagnostics: {}", mDiagnostics.toStyledString() );
    }

    void getCstInfo() {

        // Depending on the host data verbosity
        if ( mHostDataVerbosity == eHostDataVerbosity::NONE ) {
            return;
        }

        // Gather CSP information if detected
        try {
            uint32_t ws_verbosity = getDrmWSClient().getVerbosity();
            GetCspInfo( mDiagnostics, ws_verbosity );
        } catch( const std::exception &e ) {
            Debug( "No CSP information collected: {}", e.what() );
        }
        Debug( "CSP information:\n{}", mDiagnostics.toStyledString() );
    }

    Json::Value buildSettingsNode() {
        Json::Value settings;
        settings["drm_software"] = mIsHybrid;
        settings["drm_pnr"] = mIsPnR;
        settings["frequency_detection_method"] = mFreqDetectionMethod;
        settings["bypass_frequency_detection"] = mBypassFrequencyDetection;
        settings["frequency_detection_threshold"] = mFrequencyDetectionThreshold;
        settings["frequency_detection_period"] = mFrequencyDetectionPeriod;
        settings["log_file_type"] = static_cast<uint32_t>( sLogFileType );
        settings["log_file_append"] = sLogFileAppend;
        settings["log_file_rotating_size"] = static_cast<uint32_t>( sLogFileRotatingSize );
        settings["log_file_rotating_num"] = static_cast<uint32_t>( sLogFileRotatingNum );
        settings["log_file_verbosity"] = static_cast<uint32_t>( sLogFileVerbosity );
        settings["log_verbosity"] = static_cast<uint32_t>( sLogConsoleVerbosity );
        settings["log_ctrl_verbosity"] = static_cast<uint32_t>( sLogCtrlVerbosity );
        settings["ws_retry_period_long"] = mWSRetryPeriodLong;
        settings["ws_retry_period_short"] = mWSRetryPeriodShort;
        settings["ws_request_timeout"] = (int32_t)(getDrmWSClient().getRequestTimeoutMS() / 1000);
        MTX_ACQUIRE( mHealthAccessMutex );
        settings["health_period"] = mHealthPeriod;
        settings["health_retry"] = mHealthRetryTimeout;
        settings["health_retry_sleep"] = mHealthRetrySleep;
        MTX_RELEASE( mHealthAccessMutex );
        settings["ws_api_retry_duration"] = mWSApiRetryDuration;
        settings["host_data_verbosity"] = static_cast<uint32_t>( mHostDataVerbosity );
        settings["drm_ctrl_time_factor"] = mCtrlTimeFactor;
        settings["drm_ctrl_timeout_us"] = mCtrlTimeoutInUS;
        settings["drm_ctrl_sleep_us"] = mCtrlSleepInUS;
        settings["axi_frequency"] = mAxiFrequency;
        return settings;
    }

    uint32_t getMailboxSize() const {
        uint32_t roSize, rwSize;
        checkDRMCtlrRet( getDrmController().readMailboxFileSizeRegister( roSize, rwSize ) );
        Debug2( "Full R/W mailbox size: {}", rwSize );
        return rwSize;
    }

    uint32_t getUserMailboxSize() const {
        uint32_t mbSize = getMailboxSize() - (uint32_t)eMailboxOffset::MB_USER;
        auto drmMajor = ( mDrmVersionNum >> 16 ) & 0xFF;
        if ( (drmMajor <= 3) && (mbSize >= 4) )
            // Used to compensate the bug in the HDK that prevent any access to the highest addresses of the mailbox
            mbSize -= 4;

        Debug( "User Mailbox size: {}", mbSize );
        return mbSize;
    }

    std::vector<uint32_t> readMailbox( const eMailboxOffset& offset, const uint32_t& nb_elements ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( (uint32_t)index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory; max index is {}. ", index, rwData.size()-1 ); //LCOV_EXCL_LINE
        if ( index + nb_elements > rwData.size() )
            Unreachable( "Trying to read out of Mailbox memory space; size is {}. ", rwData.size() ); //LCOV_EXCL_LINE

        auto first = rwData.cbegin() + index;
        auto last = rwData.cbegin() + index + nb_elements;
        std::vector<uint32_t> value_vec( first, last );
        Debug( "Read {} elements in Mailbox from index {}", value_vec.size(), index);
        return value_vec;
    }

    template< class T >
    T readMailbox( const eMailboxOffset& offset ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize, nb_elements;
        std::vector<uint32_t> roData, rwData;

        if ( sizeof(T) % sizeof(uint32_t) )
            Unreachable( "Data type to read shall be multiple of {}. ", sizeof(uint32_t) ); //LCOV_EXCL_LINE

        if ( sizeof(T) < sizeof(uint32_t) )
            nb_elements = 1;
        else
            nb_elements = (sizeof(T) - 1) / sizeof(uint32_t) + 1;

        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );
        if ( (uint32_t)index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory; max index is {}. ", index, rwData.size()-1 ); //LCOV_EXCL_LINE
        if ( index + nb_elements > rwData.size() )
            Unreachable( "Trying to read out of Mailbox memory space; size is {}. ", rwData.size() ); //LCOV_EXCL_LINE

        auto first = rwData.cbegin() + index;
        auto last = rwData.cbegin() + index + nb_elements;
        std::vector<uint32_t> value_vec( first, last );
        T result = *((T*)value_vec.data());
        Debug( "Read {} elements in Mailbox from index {}", value_vec.size(), index);
        return result;
    }

    void writeMailbox( const eMailboxOffset& offset, const std::vector<uint32_t>& value_vec ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );
        if ( index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory: max index is {}. ", index, rwData.size()-1 ); //LCOV_EXCL_LINE
        if ( index + value_vec.size() > rwData.size() )
            Throw( DRM_BadArg, "Trying to write out of Mailbox memory space: {}. ", rwData.size() );

        std::copy( std::begin( value_vec ), std::end( value_vec ), std::begin( rwData ) + index );
        checkDRMCtlrRet( getDrmController().writeMailboxFileRegister( rwData, rwSize ) );
        Debug( "Wrote {} elements in Mailbox from index {}", value_vec.size(), index );
    }

    template< class T >
    void writeMailbox( const eMailboxOffset& offset, const T& data ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize, nb_elements;
        std::vector<uint32_t> roData, rwData;

        if ( sizeof(T) < sizeof(uint32_t) )
            nb_elements = 1;
        else
            nb_elements = (sizeof(T) - 1) / sizeof(uint32_t) + 1;
        const uint32_t* p_data = (uint32_t*)&data;
        std::vector<uint32_t> value_vec(p_data, p_data + nb_elements);

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );
        if ( index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory: max index is {}. ", index, rwData.size()-1 ); //LCOV_EXCL_LINE
        if ( index + value_vec.size() > rwData.size() )
            Throw( DRM_BadArg, "Trying to write out of Mailbox memory space: {}. ", rwData.size() );

        std::copy( std::begin( value_vec ), std::end( value_vec ), std::begin( rwData ) + index );
        checkDRMCtlrRet( getDrmController().writeMailboxFileRegister( rwData, rwSize ) );
        Debug( "Wrote {} elements in Mailbox from index {}", value_vec.size(), index );
    }

    DrmControllerLibrary::DrmControllerOperations& getDrmController() const {
        if ( mDrmController )
            return *mDrmController;
        Unreachable( "No DRM Controller available. " ); //LCOV_EXCL_LINE
    }

    DrmWSClient& getDrmWSClient() const {
        if ( mWsClient )
            return *mWsClient;
        Unreachable( "No Web Service has been defined. " ); //LCOV_EXCL_LINE
    }

    unsigned int readDrmAddress( const uint32_t address, uint32_t& value ) const {
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        int ret = f_read_register( address, &value );
        if ( ret )
            Error( "Error in read register callback, errcode = {}: failed to read address {}", ret, address );
        else
            Debug2( "Read DRM Ctrl address 0x{:x} = 0x{:08x}", address, value );
        return ret;
    }

    unsigned int writeDrmAddress( const uint32_t address, uint32_t value ) const {
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        int ret = f_write_register( address, value );
        if ( ret )
            Error( "Error in write register callback, errcode = {}: failed to write {} to address {}", ret, value, address );
        else
            Debug2( "Wrote DRM Ctrl address 0x{:x} = 0x{:08x}", address, value );
        return ret;
    }

    void lockDrmToInstance() {
        return;
/*        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        uint32_t isLocked = readMailbox<uint32_t>( eMailboxOffset::MB_LOCK_DRM );
        if ( isLocked )
            Throw( DRM_BadUsage, "Another instance of the DRM Manager is currently owning the HW. " );
        writeMailbox<uint32_t>( eMailboxOffset::MB_LOCK_DRM, 1 );
        mIsLockedToDrm = true;
        Debug( "DRM Controller is now locked to this object instance" );
*/
    }

    void unlockDrmToInstance() {
        return;
/*        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        if ( !mIsLockedToDrm )
            return;
        uint32_t isLocked = readMailbox<uint32_t>( eMailboxOffset::MB_LOCK_DRM );
        if ( isLocked ) {
            writeMailbox<uint32_t>( eMailboxOffset::MB_LOCK_DRM, 0 );
            Debug( "DRM Controller is now unlocked to this object instance" );
        }
*/
    }

    // Check compatibility of the DRM Version with Algodone version
    void checkHdkCompatibility() {
        std::string drmVersion = getDrmCtrlVersion();
        mDrmVersionNum = DrmControllerLibrary::DrmControllerDataConverter::hexStringToBinary( drmVersion )[0];
        mDrmVersionStr = DrmControllerLibrary::DrmControllerDataConverter::binaryToVersionString( mDrmVersionNum );

        auto drmMajor = ( mDrmVersionNum >> 16 ) & 0xFF;
        auto drmMinor = ( mDrmVersionNum >> 8  ) & 0xFF;

        if ( drmMajor < HDK_COMPATIBILITY_LIMIT_MAJOR ) {
            Throw( DRM_CtlrError,
                    "This DRM Library version {} is not compatible with the DRM HDK version {}: To be compatible HDK version shall be > or equal to {}.{}.x ",
                    DRMLIB_VERSION, mDrmVersionStr, HDK_COMPATIBILITY_LIMIT_MAJOR, HDK_COMPATIBILITY_LIMIT_MINOR );
        } else if ( ( drmMajor == HDK_COMPATIBILITY_LIMIT_MAJOR ) && ( drmMinor < HDK_COMPATIBILITY_LIMIT_MINOR ) ) {
            Throw( DRM_CtlrError,
                    "This DRM Library version {} is not compatible with the DRM HDK version {}: To be compatible HDK version shall be > or equal to {}.{}.x ",
                    DRMLIB_VERSION, mDrmVersionStr, HDK_COMPATIBILITY_LIMIT_MAJOR, HDK_COMPATIBILITY_LIMIT_MINOR );
        }
        Debug( "DRM HDK Version: {}", mDrmVersionStr );
    }

    /* Run BIST to check Page register access
     * This test write and read DRM Ctrl Page register to verify the page switch is working
     */
    void runBistLevel1() const {
        if ( (mDrmVersionNum >> 16) >= 8 ) {
            Debug( "DRM Communication Self-Test 1 is skipped for DRM HDK Version >= 8.x" );
            return;
        }
        unsigned int reg;
        for(unsigned int i=0; i<=5; i++) {
            if ( writeDrmAddress( 0, i ) != 0 )
                Throw( DRM_BadArg, "DRM Communication Self-Test 1 failed: Could not write DRM Ctrl page register\n{}", DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
            if ( readDrmAddress( 0, reg ) != 0 )
                Throw( DRM_BadArg, "DRM Communication Self-Test 1 failed: Could not read DRM Ctrl page register\n{}", DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
            if ( reg != i ) {
                Throw( DRM_BadArg, "DRM Communication Self-Test 1 failed: Could not switch DRM Ctrl register page.\n{}", DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
            }
        }
        Debug( "DRM Communication Self-Test 1 succeeded" );
    }

    /* Run BIST to check register accesses
     * This test write and read mailbox registers to verify the read and write callbacks are working correctly.
     */
    void runBistLevel2() const {
        // Check mailbox size
        uint32_t mb_full_size = getMailboxSize();
        if ( mb_full_size < (uint32_t)eMailboxOffset::MB_USER ) {
            Throw( DRM_BadArg, "DRM Communication Self-Test 2 failed: Unexpected mailbox size {}: Must be > {}.\n{}", mb_full_size, (uint32_t)eMailboxOffset::MB_USER, DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
        }
        uint32_t mbSize = getUserMailboxSize();

        // Check mailbox size
        uint32_t mbSizeMax = 0x8000;
        if ( mbSize >= mbSizeMax ) {
            Debug( "DRM Communication Self-Test 2 failed: bad size {}", mbSize );
            Throw( DRM_BadArg, "DRM Communication Self-Test 2 failed: Unexpected mailbox size ({} > {}).\n{}", mbSize, mbSizeMax, DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
        }
        Debug( "DRM Communication Self-Test 2: test size of mailbox passed" );

        // Write 0 to User Mailbox
        std::vector<uint32_t> wrData( mbSize, 0 );
        writeMailbox( eMailboxOffset::MB_USER, wrData );
        // Read back the mailbox and verify it has been set correctly
        std::vector<uint32_t> rdData = readMailbox( eMailboxOffset::MB_USER, mbSize );
        std::string badData;
        for( uint32_t i = 0; i < mbSize; i++ ) {
            if ( rdData[i] != wrData[i] )
                badData += fmt::format( "\tMailbox[{}]=0x{:08X} != 0x{:08X}\n", i, rdData[i], wrData[i]);
        }
        if ( badData.size() ) {
            Debug( "DRM Communication Self-Test 2 failed: writing zeros!\n" + badData );
            Throw( DRM_BadArg, "DRM Communication Self-Test 2 failed: all 0 test failed.\n{}", DRM_SELF_TEST_ERROR_MESSAGE); //LCOV_EXCL_LINE
        }
        Debug( "DRM Communication Self-Test 2: all 0 test passed" );

        // Write 1 to User Mailbox
        for( uint32_t i = 0; i < mbSize; i++ )
            wrData[i] = 0xFFFFFFFF;
        writeMailbox( eMailboxOffset::MB_USER, wrData );
        // Read back the mailbox and verify it has been set correctly
        rdData = readMailbox( eMailboxOffset::MB_USER, mbSize );
        badData.clear();
        for( uint32_t i = 0; i < mbSize; i++ ) {
            if ( rdData[i] != wrData[i] )
                badData += fmt::format( "\tMailbox[{}]=0x{:08X} != 0x{:08X}\n", i, rdData[i], wrData[i]);
        }
        if ( badData.size() ) {
            Debug( "DRM Communication Self-Test 2 failed: writing ones!\n" + badData );
            Throw( DRM_BadArg, "DRM Communication Self-Test 2 failed: all 1 test failed.\n{}", DRM_SELF_TEST_ERROR_MESSAGE); //LCOV_EXCL_LINE
        }
        Debug( "DRM Communication Self-Test 2: all 1 test passed" );

        // Then, write random values to User Mailbox
        srand( time(NULL) ); // initialize random seed:
        for( uint32_t i = 0; i < mbSize; i++ )
            wrData[i] = rand();
        writeMailbox( eMailboxOffset::MB_USER, wrData );
        // Read back the mailbox and verify it has been set correctly
        rdData = readMailbox( eMailboxOffset::MB_USER, mbSize );
        badData.clear();
        for( uint32_t i = 0; i < mbSize; i++ ) {
            if ( rdData[i] != wrData[i] )
                badData += fmt::format( "\tMailbox[{}]=0x{:08X} != 0x{:08X}\n", i, rdData[i], wrData[i]);
        }
        if ( badData.size() ) {
            Debug( "DRM Communication Self-Test 2 failed: writing randoms!\n" + badData );
            Throw( DRM_BadArg, "DRM Communication Self-Test 2 failed: random test failed.\n{}", DRM_SELF_TEST_ERROR_MESSAGE); //LCOV_EXCL_LINE
        }
        Debug( "DRM Communication Self-Test 2: random test passed" );

        Debug( "DRM Communication Self-Test 2 succeeded" );
    }

    bool isConfigInNodeLock() const {
        return mLicenseType == eLicenseType::NODE_LOCKED;
    }

    void initDrmInterface() {

        if ( mDrmController )
            return;

        // Check DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS variable exists
        const char* ctrl_timeout = getenv( "DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS" );
        mCtrlTimeoutInUS = std::stoul(std::string(ctrl_timeout));
        Debug("DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS environment variable is {}", mCtrlTimeoutInUS);
        mActivationTransmissionTimeoutMS = int( 2 * mCtrlTimeoutInUS / 1000 );

        const char* ctrl_sleep = getenv( "DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS" );
        mCtrlSleepInUS = std::stoul(std::string(ctrl_sleep));
        Debug("DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS environment variable is {}", mCtrlSleepInUS);

        // create instance
        try {
            mDrmController.reset(
                    new DrmControllerLibrary::DrmControllerOperations(
                            std::bind( &DrmManager::Impl::readDrmAddress,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2 ),
                            std::bind( &DrmManager::Impl::writeDrmAddress,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2 )
                    ));
        } catch( const std::exception &e ) {
            std::string err_msg(e.what());
            if ( err_msg.find( "Unable to select a register strategy that is compatible with the DRM Controller" )
                    != std::string::npos )
                Throw( DRM_CtlrError, "Unable to find DRM Controller registers.\n{}", DRM_SELF_TEST_ERROR_MESSAGE );
            Throw( DRM_CtlrError, "Failed to initialize DRM Controller: {}", e.what() );
        }
        Debug( "DRM Controller SDK is initialized" );

        // Check compatibility of the DRM Version with Algodone version
        checkHdkCompatibility();

        // Try to lock the DRM controller to this instance, return an error is already locked.
        lockDrmToInstance();

        // Run auto-test level 1
        runBistLevel1();

        // Run auto-test of register accesses
        runBistLevel2();

        // Determine frequency detection method if metering/floating mode is active
        if ( !isConfigInNodeLock() ) {
            determineFrequencyDetectionMethod();
            if ( mFreqDetectionMethod == 3 ) {
                detectDrmFrequencyMethod3();
            } else if ( mFreqDetectionMethod == 2 ) {
                detectDrmFrequencyMethod2();
            } else {
                Warning( "DRM frequency auto-detection is disabled: {:0.1f} will be used to compute license timers", mFrequencyCurr );
            }
        }

        // Save header information
        mHeaderJsonRequest = getMeteringHeader();

        // Create curl management object
        mWsClient.reset( new DrmWSClient( mConfFilePath, mCredFilePath ) );

        // If node-locked license is requested, create license request file
        if ( isConfigInNodeLock() ) {

            // Check license directory exists
            if ( !isDir( mNodeLockLicenseDirPath ) )
                Throw( DRM_BadArg,
                       "License directory path '{}' specified in configuration file '{}' is not existing on file system",
                       mNodeLockLicenseDirPath, mConfFilePath );
            // Create license request file
            createNodelockedLicenseRequestFile();
        } else {
            // Anticipate by requesting a token.
            TClock::time_point deadline;
            int32_t short_retry_period_ms = -1;
            if ( mWSApiRetryDuration == 0 ) {
                deadline = TClock::now() + std::chrono::milliseconds( getDrmWSClient().getRequestTimeoutMS() );
                short_retry_period_ms = -1;
            } else {
                deadline = TClock::now() + std::chrono::milliseconds( mWSApiRetryDuration * 1000 );
                short_retry_period_ms = mWSRetryPeriodShort * 1000;
            }
            requestTokenUntilValid( deadline, short_retry_period_ms );
        }
    }

    // Get DRM HDK version
    std::string getDrmCtrlVersion() const {
        std::string drmVersion;
        checkDRMCtlrRet( getDrmController().extractDrmVersion( drmVersion ) );
        return drmVersion;
    }

    void checkSessionIDFromWS( const Json::Value license_json ) {
        std::string ws_sessionID = JVgetOptional( license_json["drm_config"], "drm_session_id", Json::stringValue, mSessionID ).asString();
        if ( mSessionID.empty() ) {
            mSessionID = ws_sessionID;
            Debug( "Saving session ID: {}", mSessionID );
        } else if ( mSessionID != ws_sessionID ) {
            Warning( "Session ID mismatch: WebService returns '{}' but '{}' is expected", ws_sessionID, mSessionID ); //LCOV_EXCL_LINE
        }
    }

    void checkSessionIDFromDRM( const Json::Value license_json )  {
        std::string drm_session_id = license_json["drm_config"]["drm_session_id"].asString();
        if ( mSessionID.empty() ) {
            mSessionID = drm_session_id;
            Debug( "Saving session ID: {}", mSessionID );
        } else if ( mSessionID != drm_session_id ) {
            Throw( DRM_CtlrError, "Session ID mismatch: DRM IP returns '{}' but '{}' is expected", drm_session_id, mSessionID );
        }
    }

    void getNumActivator( uint32_t& value ) const {
        checkDRMCtlrRet( getDrmController().readNumberOfDetectedIpsStatusRegister( value ) );
    }

    uint64_t getTimerCounterValue() const {
        uint32_t licenseTimerCounterMsb(0), licenseTimerCounterLsb(0);
        uint64_t licenseTimerCounter(0);
        checkDRMCtlrRet( getDrmController().sampleLicenseTimerCounter( licenseTimerCounterMsb,
                licenseTimerCounterLsb ) );
        licenseTimerCounter = licenseTimerCounterMsb;
        licenseTimerCounter <<= 32;
        licenseTimerCounter |= licenseTimerCounterLsb;
        return licenseTimerCounter;
    }

    std::string getDrmReport() const {
        std::stringstream ss;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        getDrmController().printHwReport( ss );
        return ss.str();
    }

    Json::Value getMeteringHeader() {
        Json::Value json_output;
        std::string drmVersion;
        std::vector<std::string> vlnvFile;
        std::string mailboxReadOnly;

        Json::Value &drm_config = json_output["drm_config"];

        // Get information from DRM Controller
        getDesignInfo( drmVersion, mDeviceID, vlnvFile, mailboxReadOnly );
        mMailboxRoData = parseJsonString( mailboxReadOnly );

//TODO: UNCOMMENT THIS LINE AND REMOVE THE NEXT ONE       Json::Value product_id_json = mMailboxRoData["product_id"];
//Json::Value product_id_json = "AGCJ6WVJBFYODDFUEG2AGWNWZM";
Json::Value product_id_json = "AGCRK2ODF57PBE7ZZANNWPAVHY";
        if ( product_id_json.isString() ) {
            // v2.x HDK
            mProductID = product_id_json.asString();
        } else {
            // v1.x HDK
            std::string product_vendor = JVgetOptional( product_id_json, "vendor", Json::stringValue, "" ).asString();
            std::string product_library = JVgetOptional( product_id_json, "library", Json::stringValue, "" ).asString();
            std::string product_name = JVgetOptional( product_id_json, "name", Json::stringValue, "" ).asString();
            if ( product_vendor.empty() || product_library.empty() || product_name.empty() ) {
                Throw( DRM_CtlrError, "Unsupported product ID: {}", product_id_json.toStyledString() );
            }
            std::string product_id = product_vendor + "::" + product_library + "::" + product_name;
            mProductID = getDrmWSClient().escape( product_id );
        }

        // Fulfill drm_config section
        drm_config["lgdn_version"] = fmt::format( "{:06X}", mDrmVersionNum );
        if ( !isConfigInNodeLock() && !mIsHybrid ) {
//        if ( !mIsHybrid ) {
            drm_config["drm_frequency_init"] = mFrequencyInit;
// TODO: verify float is accepted by removing the int()
            drm_config["drm_frequency"] = int(mFrequencyCurr);
        }
// TODO: verify where to add this        drm_config["license_type"] = (uint8_t)mLicenseType;
        drm_config["drm_type"] = mIsHybrid ? 2:1;
        for ( uint32_t i = 0; i < vlnvFile.size(); i++ ) {
            std::string v_str = vlnvFile[i].substr(0, 4);
            v_str += vlnvFile[i].substr(4, 4);
            v_str += vlnvFile[i].substr(8, 4);
            v_str += vlnvFile[i].substr(12, 4);
            drm_config["vlnv_file"].append( v_str );
        }

        // Fulfill tmp section
        if ( mMailboxRoData.isMember( "pkg_version" ) ) {
// TODO: Verify it's accepted            drm_config["pkg_version"] = mMailboxRoData["pkg_version"];
//            Debug( "HDK Generator version: {}", drm_config["pkg_version"].asString() );
        }
        if ( mMailboxRoData.isMember( "dna_type" ) ) {
            drm_config["dna_type"] = mMailboxRoData["dna_type"];
            Debug( "HDK DNA type: {}", drm_config["dna_type"].asString() );
        }
        if ( mMailboxRoData.isMember( "extra" ) ) {
// TODO: Verify where to include this extra node            json_output["extra"] = mMailboxRoData["extra"];
//            Debug( "HDK extra data: {}", mMailboxRoData["extra"].toStyledString() );
            drm_config["dualclk"] = mMailboxRoData["extra"]["dualclk"];
        }

        return json_output;
    }

    Json::Value getMeteringStart() const {
        Json::Value json_output( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build starting license request #{} to create new session", mLicenseCounter );
        json_output["device_id"] = mDeviceID;

        // Request challenge and metering info for first request
        checkDRMCtlrRet( getDrmController().initialization( numberOfDetectedIps, saasChallenge, meteringFile ) );
        Json::Value &drm_config = json_output["drm_config"];
        drm_config["saas_challenge"] = saasChallenge;
        drm_config["metering_file"]  = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
//TODO: verify where to add this        drm_config["license_type"] = (uint8_t)mLicenseType;

        if (isConfigInNodeLock() ) {
            json_output["node_locked_only"] = true;
        }

        return json_output;
    }

    Json::Value getMeteringRunning() {
        Json::Value json_output( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build license request #{} to maintain current session", mLicenseCounter );

        // Check if an error occurred
        uint32_t timeout = 5 * mCtrlTimeFactor;
        checkDRMCtlrRet( getDrmController().waitNotTimerInitLoaded( timeout ) );
        // Request challenge and metering info for new request
        checkDRMCtlrRet( getDrmController().synchronousExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        Json::Value& drm_config = json_output["drm_config"];
        drm_config["saas_challenge"] = saasChallenge;
        drm_config["drm_session_id"] = meteringFile[0].substr( 0, 16 );
        drm_config["metering_file"] = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        checkSessionIDFromDRM( json_output );
        return json_output;
    }

    Json::Value getMeteringStop() {
        Json::Value json_output( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build ending license request #{} to stop current session", mLicenseCounter );

        // Request challenge and metering info for latest request
        checkDRMCtlrRet( getDrmController().endSessionAndExtractMeteringFile(
                numberOfDetectedIps, saasChallenge, meteringFile ) );
        Json::Value& drm_config = json_output["drm_config"];
        drm_config["saas_challenge"] = saasChallenge;
        drm_config["drm_session_id"] = meteringFile[0].substr( 0, 16 );
        drm_config["metering_file"]  = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        json_output["is_closed"] = true;
        checkSessionIDFromDRM( json_output );
        return json_output;
    }

    Json::Value getMeteringHealth() const {
        Json::Value json_output( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build health request #{}", mHealthCounter );
        {
            std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
            if ( isConfigInNodeLock() || isSessionRunning() ) {
                checkDRMCtlrRet( getDrmController().asynchronousExtractMeteringFile(
                        numberOfDetectedIps, saasChallenge, meteringFile ) );
            } else {
                Warning( "Cannot access metering data when no session is running" );
            }
        }

        // Add metering info and health marker
        Json::Value& drm_config = json_output["drm_config"];
        if ( meteringFile.size() ) {
            drm_config["saas_challenge"] = saasChallenge;
            drm_config["drm_session_id"] = meteringFile[0].substr( 0, 16 );
            drm_config["metering_file"]  = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        } else {
            drm_config["metering_file"]  = std::string("");
        }
        json_output["is_health"] = true;

        return json_output;
    }

    // Get common info
    void getDesignInfo( std::string &drmVersion,
                        std::string &dna,
                        std::vector<std::string> &vlnvFile,
                        std::string &mailboxReadOnly ) {
        uint32_t nbOfDetectedIps;
        uint32_t readOnlyMailboxSize, readWriteMailboxSize;
        std::vector<uint32_t> readOnlyMailboxData, readWriteMailboxData;

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().extractDrmVersion( drmVersion ) );
        checkDRMCtlrRet( getDrmController().extractDna( dna ) );
        checkDRMCtlrRet( getDrmController().extractVlnvFile( nbOfDetectedIps, vlnvFile ) );
        Debug( "Number of detected activators: {}", nbOfDetectedIps );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( readOnlyMailboxSize, readWriteMailboxSize,
                                                                     readOnlyMailboxData, readWriteMailboxData ) );
        Debug( "Mailbox sizes: read-only={}, read-write={}", readOnlyMailboxSize, readWriteMailboxSize );
        readOnlyMailboxData.push_back( 0 );
        mailboxReadOnly = std::string( (char*)readOnlyMailboxData.data() );
        if ( mailboxReadOnly.empty() ) {
            Debug( "Could not find Product ID information in DRM Controller Memory" );
        }
    }

    bool isDrmCtrlInNodelock() const  {
        bool isNodelocked( false );
        checkDRMCtlrRet( getDrmController().readLicenseNodeLockStatusRegister( isNodelocked ) );
        Debug( "DRM Controller node-locked status: {}", isNodelocked );
        return isNodelocked;
    }

    bool isDrmCtrlInMetering() const  {
        bool isMetering( false );
        checkDRMCtlrRet( getDrmController().readLicenseMeteringStatusRegister( isMetering ) );
        Debug( "DRM Controller metering status: {}", isMetering );
        return isMetering;
    }

    bool isReadyForNewLicense() const {
        uint32_t numberOfLicenseTimerLoaded;
        checkDRMCtlrRet( getDrmController().readNumberOfLicenseTimerLoadedStatusRegister(
                    numberOfLicenseTimerLoaded ) );
        bool readiness = ( numberOfLicenseTimerLoaded < 2 );
        Debug( "DRM readiness to receive a new license: {} (# loaded licenses = {})", readiness, numberOfLicenseTimerLoaded );
        return readiness;
    }

    bool isSessionRunning() const  {
        bool sessionRunning( false );
        if ( isDrmCtrlInNodelock() ) {
            checkDRMCtlrRet( getDrmController().readActivationCodesTransmittedStatusRegister( sessionRunning ) );
        } else {
            checkDRMCtlrRet( getDrmController().readSessionRunningStatusRegister( sessionRunning ) );
        }
        Debug( "DRM session running state: {}", sessionRunning );
        return sessionRunning;
    }

    bool isLicenseActive() const {
        bool isActive( false );
        checkDRMCtlrRet( getDrmController().readActivationCodesTransmittedStatusRegister( isActive ) );
        return isActive;
    }

    void requestTokenUntilValid( const TClock::time_point& deadline, int32_t short_retry_period_ms = -1, int32_t long_retry_period_ms = -1 ) {
        TClock::duration wait_duration;
        TClock::duration long_duration = std::chrono::milliseconds( long_retry_period_ms );
        TClock::duration short_duration = std::chrono::milliseconds( short_retry_period_ms );
        bool token_valid(false);
        uint32_t http_attempts = 0;
        int32_t timeout_msec;
        std::chrono::milliseconds timeout_chrono;

        while ( !token_valid ) {
            /// Get valid OAUth2 token
            try {
                timeout_chrono = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 deadline - TClock::now() );
                timeout_msec = timeout_chrono.count();
                getDrmWSClient().getOAuth2token( timeout_msec );
                token_valid = true;
            } catch ( const Exception& e ) {
                if ( e.getErrCode() == DRM_WSTimedOut ) {
                    // Reached timeout
                    Throw( DRM_WSTimedOut, "Timeout on Authentication request after {} attempts", http_attempts );
                }
                if ( e.getErrCode() != DRM_WSMayRetry ) {
                    throw;
                }
                /// It is retryable
                http_attempts ++;
                if ( short_retry_period_ms == -1 ) {
                    // No retry
                    Debug( "OAuthentication retry mechanism is disabled" );
                    throw;
                }
                if ( long_retry_period_ms == -1 )
                     wait_duration = short_duration;
                else if ( ( deadline - TClock::now() ) <= ( long_duration + 2*short_duration )  )
                    wait_duration = short_duration;
                else
                    wait_duration = long_duration;
                Warning( "Attempt #{} on Authentication request failed with message: {}. New attempt planned in {} seconds",
                        http_attempts, e.what(), wait_duration.count()/1000000000 );
                /// Wait a bit before retrying
                sleepOrExit( wait_duration );
            }
        }
    }

    Json::Value getLicense( Json::Value& request_json, const uint32_t& timeout_ms,
                            int32_t short_retry_period_ms = -1, int32_t long_retry_period_ms = -1, bool open = false ) {
        TClock::time_point deadline;
        if ( timeout_ms == 0 ) {
            deadline = TClock::now() + std::chrono::milliseconds( getDrmWSClient().getRequestTimeoutMS() );
            short_retry_period_ms = -1;
            long_retry_period_ms = -1;
        } else {
            deadline = TClock::now() + std::chrono::milliseconds( timeout_ms );
        }
        return getLicense( request_json, deadline, short_retry_period_ms, long_retry_period_ms, open );
    }

    Json::Value getLicense( Json::Value& request_json, const TClock::time_point& deadline,
                        int32_t short_retry_period_ms = -1, int32_t long_retry_period_ms = -1, bool open = false ) {
        TClock::duration wait_duration;
        TClock::duration long_duration = std::chrono::milliseconds( long_retry_period_ms );
        TClock::duration short_duration = std::chrono::milliseconds( short_retry_period_ms );
        uint32_t http_attempts = 0;
        int32_t timeout_msec;
        std::chrono::milliseconds timeout_chrono;
        std::string suburl;
        tHttpRequestType httpType;
        std::string request_type = request_json.isMember("is_health") ? "Health":"License";

        if ( open ) {
// TODO: check where to add these            request_json["settings"] = buildSettingsNode();     // Add settings parameters
            suburl = fmt::format( "/customer/product/{}/entitlement_session", mProductID );
            httpType = tHttpRequestType::POST;
        } else {
            suburl = fmt::format( "/customer/entitlement_session/{}", mEntitlementID );
            httpType = tHttpRequestType::PATCH;
        }

        if ( mLicenseCounter == 1 ) {
            request_json["diagnostic"] = mDiagnostics;
            Debug( "Added diagnostics information to request" );
        }

        while ( 1 ) {
            /// Get valid OAUth2 token
            requestTokenUntilValid( deadline, short_retry_period_ms, long_retry_period_ms );

            /// Get new license
            try {
                /// Send license request and wait for the answer
                timeout_chrono = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 deadline - TClock::now() );
                timeout_msec = timeout_chrono.count();
                return getDrmWSClient().sendSaasRequest( suburl, httpType, request_json, timeout_msec );
            } catch ( const Exception& e ) {
                if ( e.getErrCode() == DRM_WSTimedOut ) {
                    // Reached timeout
                    Throw( DRM_WSTimedOut, "Timeout on {} request after {} attempts. ", request_type, http_attempts );
                }
                if ( e.getErrCode() != DRM_WSMayRetry ) {
                    throw;
                }
                // It is retryable
                http_attempts ++;
                if ( short_retry_period_ms == -1 ) {
                    Warning( "Attempt on {} request failed with message: {}.", request_type, e.what() );
                    throw;
                }
                /// Evaluate the next retry
                if ( long_retry_period_ms == -1 )
                     wait_duration = short_duration;
                else if ( ( deadline - TClock::now() ) <= ( long_duration + 2*short_duration ) )
                    wait_duration = short_duration;
                else
                    wait_duration = long_duration;
                Warning( "Attempt #{} on {} request failed with message: {}. New attempt planned in {} seconds",
                        http_attempts, request_type, e.what(), wait_duration.count()/1000000000 );
                /// Wait a bit before retrying
                sleepOrExit( wait_duration );
            }
        }
    }

    void setLicense( const Json::Value& license_json ) {
        Debug( "Provisioning license #{} on DRM controller", mLicenseCounter );

        std::string licenseKey, licenseTimer;

        try {
            /// Check session ID received from web service and check it if possible
            checkSessionIDFromWS( license_json );

            /// Get needed parameters from response
            std::string entitlement_id = JVgetRequired( license_json, "id", Json::stringValue ).asString();
            if ( mEntitlementID != entitlement_id ) {
                mEntitlementID = entitlement_id;
                Debug( "Entitlement ID '{}' is updated to '{}'", mEntitlementID, entitlement_id );
            }
            Json::Value drm_config_node = JVgetRequired( license_json, "drm_config", Json::objectValue );
            mLicenseDuration = JVgetOptional( drm_config_node, "license_period_second", Json::uintValue, mLicenseDuration ).asUInt();

            /// Get health parameters
            MTX_ACQUIRE( mHealthAccessMutex );
            mHealthPeriod = JVgetOptional( drm_config_node, "health_period", Json::uintValue, mHealthPeriod ).asUInt();
            mHealthRetryTimeout = JVgetOptional( drm_config_node, "health_retry", Json::uintValue, mHealthRetryTimeout ).asUInt();
            mHealthRetrySleep = JVgetOptional( drm_config_node, "health_retry_sleep", Json::uintValue, mHealthRetrySleep ).asUInt();
            Debug( "Health parameters update: healthPeriod={}s, healthRetry={}s, healthRetrySleep={}s",
                            mHealthPeriod, mHealthRetryTimeout, mHealthRetrySleep );
            if ( mHealthPeriod )
                startHealthContinuityThread();
            MTX_RELEASE( mHealthAccessMutex );

            /// Get license node
            Json::Value license_node = JVgetRequired( drm_config_node, "license", Json::objectValue );
            Json::Value dna_node = JVgetRequired( license_node, mDeviceID.c_str(), Json::objectValue );

            /// Extract license key and license timer from web service response
            licenseKey = JVgetRequired( dna_node, "key", Json::stringValue ).asString();
            licenseTimer = JVgetOptional( dna_node, "timer", Json::stringValue, "" ).asString();
        } catch( const Exception &e ) {
            Throw( DRM_WSRespError, "Malformed response from License Web Service: {}", e.what() );
        }

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        if ( mLicenseCounter == 0 ) {
            // Load key
            checkDRMCtlrRet( getDrmController().activate( licenseKey ) );
            Debug( "Wrote license key of session ID {}", mSessionID );
        }

        // Check DRM Controller has switched to the right license mode
        checkDRMControllerLicenseType();

        // Load license timer
        if ( !isConfigInNodeLock() ) {
            if ( mLicenseDuration == 0 ) {
                Warning( "'license_period_second' field sent by License Server must not be 0" );
            }
            uint32_t timeout = 5 * mCtrlTimeFactor;
            checkDRMCtlrRet( getDrmController().loadLicenseTimerInit( licenseTimer, mIsHybrid, timeout ) );
            Debug( "Wrote license timer #{} of session ID {} for a duration of {} seconds",
                    mLicenseCounter, mSessionID, mLicenseDuration );

            // Update expiration time
            if ( mExpirationTime.time_since_epoch().count() == 0 ) {
                Debug( "Initialize expiration time");
                mExpirationTime = TClock::now();
            }
            mExpirationTime += std::chrono::seconds( mLicenseDuration );
            Debug( "Update expiration time to {}", time_t_to_string( steady_clock_to_time_t( mExpirationTime ) ) );
        }

        // Wait until license has been pushed to Activator's port
        waitActivationCodeTransmitted();

        // Wait until session is running if license is metering
        waitUntilSessionIsRunning();

        Debug( "Provisioned license #{} for session {} on DRM controller", mLicenseCounter, mSessionID );
        mLicenseCounter ++;
    }

    std::string getNodelockBaseName() {
        /*std::string name = mHeaderJsonRequest["product"]["vendor"].asString();
        name += "_" + mHeaderJsonRequest["product"]["library"].asString();
        name += "_" + mHeaderJsonRequest["product"]["name"].asString();
        */
        std::string name = mProductID + "_" + mDeviceID;
        return name;
    }

    void createNodelockedLicenseRequestFile() {
        // Create hash name based on design info
        std::string basname = getNodelockBaseName();
        mNodeLockRequestFilePath = mNodeLockLicenseDirPath + PATH_SEP + basname + ".req";
        mNodeLockLicenseFilePath = mNodeLockLicenseDirPath + PATH_SEP + basname + ".lic";
        // Check if license request file already exists
        if ( isFile( mNodeLockRequestFilePath ) ) {
            Debug( "A license request file is already existing in license directory: {}", mNodeLockLicenseDirPath );
            return;
        }
        // Build request for node-locked license
        Json::Value request_json = getMeteringStart();
        Debug( "License request JSON:\n{}", request_json.toStyledString() );

        // Save license request to file
        saveJsonToFile( mNodeLockRequestFilePath, request_json );
        Debug( "Node-locked license request file saved on: {}", mNodeLockRequestFilePath );
    }

    void installNodelockedLicense() {
        Json::Value license_json;
            std::ifstream ifs;

        Debug( "Looking for local node-locked license file: {}", mNodeLockLicenseFilePath );

        // Check if license file exists
        if ( isFile( mNodeLockLicenseFilePath ) ) {
            try {
                // Try to load the local license file
                license_json = parseJsonFile( mNodeLockLicenseFilePath );
                Debug( "Parsed existing node-locked License file: {}", license_json .toStyledString() );
            } catch( const Exception& e ) {
                Throw( e.getErrCode(), "Invalid local license file {} because {}. "
                     "If this machine is connected to the License server network, rename the file and retry. "
                     "Otherwise request a new Node-Locked license from your supplier. ",
                     mNodeLockLicenseFilePath, e.what() );
            }
        } else {
            /// No license has been found locally, request one to License WS:
            Debug( "Could not find nodelocked license file: {}", mNodeLockLicenseFilePath );
            /// - Clear Session IS
            mSessionID = std::string("");
            mEntitlementID = std::string("");
            Debug( "Cleared session ID: {}", mSessionID );
            try {
                /// - Read request file
                Json::Value request_json = parseJsonFile( mNodeLockRequestFilePath );
                /// - Add diagnostic info
                getCstInfo();
                getHostAndCardInfo();
                request_json["diagnostic"] = mDiagnostics;
                Debug( "Parsed newly created node-locked License Request file: {}", request_json .toStyledString() );
                /// - Send request to web service and receive the new license
                license_json = getLicense( request_json, mWSApiRetryDuration * 1000, mWSRetryPeriodShort * 1000, true );
                /// - Save the license to file
                saveJsonToFile( mNodeLockLicenseFilePath, license_json );
                Debug( "Requested and saved new node-locked license file: {}", mNodeLockLicenseFilePath );
            } catch( const Exception& e ) {
                Throw( e.getErrCode(), "Failed to request license file: {}. ", e.what() );
            }
        }
        /// Install the license
        setLicense( license_json );
        Info( "Installed node-locked license successfully" );
    }

    void determineFrequencyDetectionMethod() {
        uint32_t reg;

        if ( mIsHybrid ){
            Debug( "SW DRM Controller: no frequency detection is performed" );
            mFreqDetectionMethod = 0;
            mBypassFrequencyDetection = true;
            mFrequencyCurr = 0.001;
            return;
        }

        if ( mBypassFrequencyDetection ) {
            Debug( "Frequency detection sequence is bypassed." );
            return;
        }

        int ret = writeDrmAddress(0, 0 );
        ret |= readDrmAddress( REG_FREQ_DETECTION_VERSION, reg );
        if ( ret != 0 ) {
            Debug( "Failed to read DRM Ctrl frequency detection version register, errcode = {}. ", ret ); //LCOV_EXCL_LINE
        }
        if ( reg == FREQ_DETECTION_VERSION_3 ) {
            // Use Method 3
            Debug( "Use dedicated counter to compute DRM frequency (method 3)" );
            mFreqDetectionMethod = 3;
        } else if ( reg == FREQ_DETECTION_VERSION_2 ) {
            // Use Method 2
            Debug( "Use dedicated counter to compute DRM frequency (method 2)" );
            mFreqDetectionMethod = 2;
        } else {
            // Use Method 1
            Debug( "Use license timer counter to compute DRM frequency (method 1)" );
            mFreqDetectionMethod = 1;
        }
    }

    void detectDrmFrequencyMethod1() {
        std::vector<int32_t> frequency_list;

        if ( mBypassFrequencyDetection ) {
            return;
        }

        frequency_list.push_back( detectDrmFrequencyFromLicenseTimer() );
        frequency_list.push_back( detectDrmFrequencyFromLicenseTimer() );
        frequency_list.push_back( detectDrmFrequencyFromLicenseTimer() );
        std::sort( frequency_list.begin(), frequency_list.end());

        int32_t measured_frequency = frequency_list[1];
        checkDrmFrequency( measured_frequency );
    }

    void detectDrmFrequencyMethod2() {
        int ret;
        uint32_t counter;
        TClock::duration wait_duration = std::chrono::milliseconds( mFrequencyDetectionPeriod );

        if ( mBypassFrequencyDetection ) {
            return;
        }

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        // Reset detection counter by writing drm_aclk counter register
        ret = writeDrmAddress( REG_FREQ_DETECTION_VERSION, 0 );
        if ( ret != 0 )
            Unreachable( "Failed to start DRM frequency detection counter, errcode = {}. ", ret ); //LCOV_EXCL_LINE

        // Wait a fixed period of time
        sleepOrExit( wait_duration );

        // Sample drm_aclk counter
        ret = readDrmAddress( REG_FREQ_DETECTION_COUNTER_DRMACLK, counter );
        if ( ret != 0 ) {
            Unreachable( "Failed to read DRM Ctrl frequency detection counter register, errcode = {}. ", ret ); //LCOV_EXCL_LINE
        }

        if ( counter == 0xFFFFFFFF )
            Throw( DRM_BadFrequency, "Frequency auto-detection failed: frequency_detection_period parameter ({} ms) is too long. ",
                   mFrequencyDetectionPeriod );

        // Compute estimated DRM frequency
        int32_t measured_frequency = (int32_t)((double)counter / mFrequencyDetectionPeriod / 1000);
        Debug( "Frequency detection counter after {} ms is 0x{:08x}  => estimated frequency = {} MHz",
            mFrequencyDetectionPeriod, counter, measured_frequency );

        checkDrmFrequency( measured_frequency );
    }

    void detectDrmFrequencyMethod3() {
        int ret;
        uint32_t counter_drmaclk, counter_axiaclk;
        TClock::duration wait_duration = std::chrono::milliseconds( mFrequencyDetectionPeriod );

        if ( mBypassFrequencyDetection ) {
            return;
        }

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        // Reset detection counter by writing drm_aclk counter register
        ret = writeDrmAddress( REG_FREQ_DETECTION_VERSION, 0 );
        if ( ret != 0 )
            Unreachable( "Failed to start DRM frequency detection counter, errcode = {}. ", ret ); //LCOV_EXCL_LINE

        // Wait a fixed period of time
        sleepOrExit( wait_duration );

        // Sample drm_aclk and s_axi_aclk counters
        ret = readDrmAddress( REG_FREQ_DETECTION_COUNTER_DRMACLK, counter_drmaclk );
        if ( ret != 0 ) {
            Unreachable( "Failed to read drm_aclk frequency detection counter register, errcode = {}. ", ret ); //LCOV_EXCL_LINE
        }
        ret = readDrmAddress( REG_FREQ_DETECTION_COUNTER_AXIACLK, counter_axiaclk );
        if ( ret != 0 ) {
            Unreachable( "Failed to read s_axi_aclk frequency detection counter register, errcode = {}. ", ret ); //LCOV_EXCL_LINE
        }

        if ( counter_drmaclk == 0xFFFFFFFF )
            Throw( DRM_BadFrequency, "Frequency auto-detection of drm_aclk failed: frequency_detection_period parameter ({} ms) is too long. ",
                   mFrequencyDetectionPeriod );
        if ( counter_axiaclk == 0xFFFFFFFF )
            Throw( DRM_BadFrequency, "Frequency auto-detection of s_axi_aclk failed: frequency_detection_period parameter ({} ms) is too long. ",
                   mFrequencyDetectionPeriod );

        // Compute estimated DRM frequency for s_axi_aclk
        mAxiFrequency = (int32_t)((double)counter_axiaclk / mFrequencyDetectionPeriod / 1000);
        Debug( "Frequency detection of s_axi_aclk counter after {} ms is 0x{:08x}  => estimated frequency = {} MHz",
            mFrequencyDetectionPeriod, counter_axiaclk, mAxiFrequency );

        // Compute estimated DRM frequency for drm_aclk
        int32_t measured_drmaclk = (int32_t)((double)counter_drmaclk / mFrequencyDetectionPeriod / 1000);
        Debug( "Frequency detection of drm_aclk counter after {} ms is 0x{:08x}  => estimated frequency = {} MHz",
            mFrequencyDetectionPeriod, counter_drmaclk, measured_drmaclk );
        checkDrmFrequency( measured_drmaclk ); // Only drm_aclk can be verified because provided in the config.json
    }

    int32_t detectDrmFrequencyFromLicenseTimer() {
        TClock::time_point timeStart, timeEnd;
        uint64_t counterStart, counterEnd;
        TClock::duration wait_duration = std::chrono::milliseconds( mFrequencyDetectionPeriod );
        int max_attempts = 3;

        Debug( "Detecting DRM frequency in {} ms", mFrequencyDetectionPeriod );

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        while ( max_attempts > 0 ) {

            counterStart = getTimerCounterValue();

            // Wait until counter starts decrementing
            while (1) {
                if ( getTimerCounterValue() < counterStart ) {
                    counterStart = getTimerCounterValue();
                    timeStart = TClock::now();
                    break;
                }
            }

            // Wait a fixed period of time
            sleepOrExit( wait_duration );

            counterEnd = getTimerCounterValue();
            timeEnd = TClock::now();

            if ( counterEnd == 0 )
                Unreachable( "Frequency auto-detection failed: license timeout counter is 0. " ); //LCOV_EXCL_LINE
            if (counterEnd > counterStart)
                Debug( "License timeout counter has been reset: taking another sample" );
            else
                break;
            max_attempts--;
        }
        if ( max_attempts == 0 )
            Unreachable("Failed to estimate DRM frequency after 3 attempts. "); //LCOV_EXCL_LINE

        Debug( "Start time = {} / Counter start = {}", timeStart.time_since_epoch().count(), counterStart );
        Debug( "End time = {} / Counter end = {}", timeEnd.time_since_epoch().count(), counterEnd );

        // Compute estimated DRM frequency
        TClock::duration timeSpan = timeEnd - timeStart;
        double seconds = double( timeSpan.count() ) * TClock::period::num / TClock::period::den;
        auto ticks = (uint32_t)(counterStart - counterEnd);
        auto measuredFrequency = (int32_t)(std::ceil((double)ticks / seconds / 1000000));
        Debug( "Duration = {} s   /   ticks = {}   =>   estimated frequency = {} MHz", seconds, ticks, measuredFrequency );

        return measuredFrequency;
    }

    void checkDrmFrequency( int32_t measuredFrequency ) {
        // Compute precision error compared to config file
        double precisionError = 100.0 * abs( measuredFrequency - mFrequencyInit ) / mFrequencyInit ; // At that point mFrequencyCurr = mFrequencyInit
        Debug( "Estimated DRM frequency = {} MHz, config frequency = {} MHz: gap = {}%",
                measuredFrequency, mFrequencyInit, precisionError );
        if ( precisionError >= mFrequencyDetectionThreshold ) {
            mFrequencyCurr = measuredFrequency;
            Throw( DRM_BadFrequency,
                   "Estimated DRM frequency ({} MHz) differs from the value ({} MHz) defined in the configuration file '{}' by {}% (threshold is {}%): From now on the estimated frequency will be used.",
                    mFrequencyCurr, mFrequencyInit, mConfFilePath, precisionError, mFrequencyDetectionThreshold);
        } else {
            mFrequencyCurr = mFrequencyInit;
        }
    }

    template< class Clock, class Duration >
    void sleepOrExit( const std::chrono::time_point<Clock, Duration> &timeout_time ) {
        std::unique_lock<std::mutex> lock( mThreadExitMtx );
        bool isExitRequested = mThreadExitCondVar.wait_until( lock, timeout_time,
                [ this ]{ return mThreadExit; } );
        if ( isExitRequested )
            Throw( DRM_Exit, "Exit requested. " );
    }

    template< class Rep, class Period >
    void sleepOrExit( const std::chrono::duration<Rep, Period> &rel_time ) {
        std::unique_lock<std::mutex> lock( mThreadExitMtx );
        bool isExitRequested = mThreadExitCondVar.wait_for( lock, rel_time,
                [ this ]{ return mThreadExit; } );
        if ( isExitRequested )
            Throw( DRM_Exit, "Exit requested. " );
    }

    bool isStopRequested() {
        std::lock_guard<std::mutex> lock( mThreadExitMtx );
        return mThreadExit;
    }

    uint32_t getCurrentLicenseTimeLeft() {
        uint64_t counterCurr = getTimerCounterValue();
        return (uint32_t)std::ceil( (double)counterCurr / mFrequencyCurr / 1000000 );
    }

    void startLicenseContinuityThread() {

        if ( mLicenseThread.valid() ) {
            Warning( "Licensing thread already started" );
            return;
        }

        mLicenseThread = std::async( std::launch::async, [ this ]() {
            Debug( "Starting background thread which maintains licensing" );
            try {
                // Collect CSP information if possible
                getCstInfo();
                // Collect Host information
                getHostAndCardInfo();

                /// Detecting DRM controller frequency if needed
                if ( mFreqDetectionMethod == 1 )
                    detectDrmFrequencyMethod1();

                bool go_sleeping( false );

                /// Starting license request loop
                while( 1 ) {
                    if ( isStopRequested() )
                        break;

                    /// Check DRM licensing queue
                    MTX_ACQUIRE( mMeteringAccessMutex );
                    if ( !isReadyForNewLicense() ) {
                        go_sleeping = true;

                    } else {
                        go_sleeping = false;

                        /// Close health thread if it's been disabled
                        if ( ( mHealthPeriod == 0 ) && mHealthThread.valid() ) {
                            mHealthThread.get();  // Wait until the Health thread ends
                        }

                        /// Build new license request
                        Debug( "Requesting new license #{} now", mLicenseCounter );
                        Json::Value request_json = getMeteringRunning();

                        /// Attempt to get the next license from server
                        Json::Value license_json = getLicense( request_json, mExpirationTime,
                                        mWSRetryPeriodShort*1000, mWSRetryPeriodLong*1000 );

                        /// New license has been received: now send it to the DRM Controller
                        setLicense( license_json );
                    }
                    MTX_RELEASE( mMeteringAccessMutex );

                    if ( go_sleeping ) {
                        // DRM license queue is full, wait until current license expires
                        uint32_t licenseTimeLeft = getCurrentLicenseTimeLeft();
                        TClock::duration wait_duration = std::chrono::seconds( licenseTimeLeft + 1 );
                        Debug( "License thread sleeping {} seconds", licenseTimeLeft );
                        sleepOrExit( wait_duration );
                        // Resync expiration time
                        licenseTimeLeft = getCurrentLicenseTimeLeft();
                        mExpirationTime = TClock::now() + std::chrono::seconds( licenseTimeLeft );
                        Debug( "Update expiration time to {}", time_t_to_string( steady_clock_to_time_t( mExpirationTime ) ) );
                    }
                }

            } catch( const Exception& e ) {
                DRM_ErrorCode errcode = e.getErrCode();
                if ( errcode != DRM_Exit ) {
                    std::string errmsg = std::string( e.what() );
                    if ( ( errcode >= DRM_WSReqError ) && ( errcode <= DRM_WSTimedOut ) ) {
                        errmsg += DRM_CONNECTION_ERROR_MESSAGE;
                    }
                    Error( errmsg );
                    f_asynch_error( errmsg );
                }
            } catch( const std::exception &e ) {
                std::string errmsg = fmt::format( "[errCode={}] Unexpected error: {}", (uint32_t)DRM_ExternFail, e.what() );
                Error( errmsg );
                f_asynch_error( errmsg );
            }
            logDrmCtrlError();
            logDrmCtrlTrngStatus();
            Debug( "Exiting background thread which maintains licensing" );
            sLogger->flush();
        });
    }

    void startHealthContinuityThread() {

        if ( mHealthThread.valid() ) {
            Debug( "Health thread already started" );
            return;
        }

        mHealthThread = std::async( std::launch::async, [ this ]() {
            Debug( "Starting background thread which checks health" );
            try {
                int32_t retry_timeout_ms, retry_sleep_ms;
                uint32_t health_period, health_retry_timeout, health_retry_sleep;

                MTX_ACQUIRE( mHealthAccessMutex );
                health_period = mHealthPeriod;
                MTX_RELEASE( mHealthAccessMutex );

                /// Starting async metering update loop
                while( 1 ) {
                    /// Sleep until it's time to collect the next metering data
                    TClock::time_point wakeup_time = TClock::now() + std::chrono::seconds( health_period );
                    Debug( "Health thread sleeping {} seconds before gathering new metering", health_period );
                    sleepOrExit( wakeup_time );

                    MTX_ACQUIRE( mHealthAccessMutex );
                    health_period = mHealthPeriod;
                    health_retry_timeout = mHealthRetryTimeout;
                    health_retry_sleep = mHealthRetrySleep;
                    MTX_RELEASE( mHealthAccessMutex );

                    if ( health_period == 0 ) {
                        Debug( "Health thread is disabled" );
                        break;
                    }
                    if ( health_retry_timeout == 0 ) {
                        retry_timeout_ms = 0;
                        retry_sleep_ms = -1;
                        Debug( "Health retry is disabled" );
                    } else {
                        retry_timeout_ms = health_retry_timeout * 1000;
                        retry_sleep_ms = health_retry_sleep * 1000;
                        Debug( "Health retry is enabled" );
                    }

                    // Report metering to service
                    try {
                        MTX_ACQUIRE( mMeteringAccessMutex );
                        // Get next data from DRM Controller
                        Json::Value request_json = getMeteringHealth();
                        // Post next data to server
                        Debug( "Sending new health info #{}", mHealthCounter );
                        Json::Value license_json = getLicense( request_json, retry_timeout_ms, retry_sleep_ms );
                        MTX_RELEASE( mMeteringAccessMutex );
                        // Increment health request counter
                        MTX_ACQUIRE( mHealthAccessMutex );
                        mHealthCounter ++;
                        MTX_RELEASE( mHealthAccessMutex );
                    } catch( const Exception& e ) {}
                }
            } catch( const Exception& e ) {
                DRM_ErrorCode errcode = e.getErrCode();
                if ( errcode != DRM_Exit ) {
                    if ( errcode != DRM_Exit ) {
                        std::string errmsg = std::string( e.what() );
                        if ( ( errcode >= DRM_WSReqError ) && ( errcode <= DRM_WSTimedOut ) ) {
                            errmsg += DRM_CONNECTION_ERROR_MESSAGE;
                        }
                        Error( errmsg );
                        f_asynch_error( errmsg );
                    }
                }
            } catch( const std::exception &e ) {
                Error( e.what() );
                f_asynch_error( e.what() );
            }
            Debug( "Exiting background thread which checks health" );
            sLogger->flush();
        });
    }

    void stopThread() {
        if ( ( mLicenseThread.valid() == 0 ) && ( mHealthThread.valid() == 0 ) ) {
            Debug( "Background threads are not running" );
            return;
        }
        {
            std::lock_guard<std::mutex> lock( mThreadExitMtx );
            Debug( "Set Stop flag for threads" );
            mThreadExit = true;
        }
        mThreadExitCondVar.notify_all();
        if ( mLicenseThread.valid() )
            mLicenseThread.get();     // Wait until the License thread ends
        if ( mHealthThread.valid() )
            mHealthThread.get();     // Wait until the Health thread ends
        Debug( "Background threads stopped" );
        {
            std::lock_guard<std::mutex> lock( mThreadExitMtx );
            Debug( "Clear Stop flag for threads" );
            mThreadExit = false;
        }
    }

    void waitActivationCodeTransmitted() {
        if ( mLicenseCounter > 0 ) {
            // Don't need to check activation code transmitted bit on license renewal (cause it's already set to 1)
            return;
        }
        bool activationCodesTransmitted( false );
        TClock::duration timeSpan;
        uint32_t mseconds( 0 );
        uint32_t sleep_period = mCtrlSleepInUS * 200;
        TClock::time_point timeStart = TClock::now();
        while( mseconds < mActivationTransmissionTimeoutMS ) {
            checkDRMCtlrRet( getDrmController().readActivationCodesTransmittedStatusRegister(
                    activationCodesTransmitted ) );
            timeSpan = TClock::now() - timeStart;
            mseconds = int( 1000 * double( timeSpan.count() ) * TClock::period::num / TClock::period::den );
            if ( activationCodesTransmitted ) {
                Debug( "License #{} transmitted (latency = {} ms)", mLicenseCounter, mseconds );
                break;
            }
            Debug2( "License #{} not transmitted yet (latency = {} ms)", mLicenseCounter, mseconds );
            usleep(sleep_period);
        }
        if ( !activationCodesTransmitted ) {
            Throw( DRM_CtlrError, "DRM Controller could not transmit Licence #{} to activators after {} ms. ", mLicenseCounter, mseconds ); //LCOV_EXCL_LINE
        }
    }

    void checkDRMControllerLicenseType() {
        bool is_nodelocked = isDrmCtrlInNodelock();
        bool is_metered = isDrmCtrlInMetering();
        if ( is_nodelocked && is_metered )
            Unreachable( "DRM Controller cannot be in both Node-Locked and Metering/Floating license modes. " ); //LCOV_EXCL_LINE
        if ( isConfigInNodeLock() && !is_nodelocked ) {
            // Nodelock forced by conf file but received license is a metering license
            Throw( DRM_BadUsage, "DRM Controller failed to switch to Node-Locked license mode. " );
        }
        if ( is_metered ) {
            Debug( "DRM Controller is in Metering license mode" );
        } else if ( is_nodelocked ) {
            mLicenseType = eLicenseType::NODE_LOCKED;
            Debug( "DRM Controller is in Node-Locked license mode" );
        }
    }

    void waitUntilSessionIsRunning() {
        if ( !isDrmCtrlInMetering() ) {
            Debug( "There is no session in Node-Locked licensing mode" );
            return;
        }
        uint32_t mseconds( 0 );
        bool is_running(false);
        TClock::duration timeSpan;
        uint32_t sleep_period = mCtrlSleepInUS * 100;
        TClock::time_point timeStart = TClock::now();
        while( mseconds < mActivationTransmissionTimeoutMS ) {
            is_running = isSessionRunning();
            timeSpan = TClock::now() - timeStart;
            mseconds = int( 1000 * double( timeSpan.count() ) * TClock::period::num / TClock::period::den );
            if ( is_running ) {
                Debug( "Session ID {} is now running (latency = {} ms)", mSessionID, mseconds );
                break;
            }
            Debug2( "Session ID {} is not running yet (latency = {} ms)", mSessionID, mseconds );
            usleep(sleep_period);
        }
        if ( !is_running ) {
            Throw( DRM_CtlrError, "DRM Controller could not run Session ID {} after {} ms. ", mSessionID, mseconds ); //LCOV_EXCL_LINE
        }
    }

    void startSession() {
        MTX_ACQUIRE( mMeteringAccessMutex );

        if ( !isReadyForNewLicense() )
            Unreachable( "To start a new session the DRM Controller shall be ready to accept a new license. " ); //LCOV_EXCL_LINE

        mHealthCounter = 0;
        mLicenseCounter = 0;

        // Build start request message for new license
        Debug( "Requesting new license #{} now", mLicenseCounter );
        Json::Value request_json = getMeteringStart();

        // Send request and receive new license
        Json::Value license_json = getLicense( request_json, mWSApiRetryDuration * 1000, mWSRetryPeriodShort * 1000, true );
        setLicense( license_json );

        MTX_RELEASE( mMeteringAccessMutex );

        Info( "DRM session {} started.", mSessionID );
    }

    void stopSession() {
        Json::Value request_json;

        // Stop background thread
        stopThread();

        try {
            MTX_ACQUIRE( mMeteringAccessMutex );
            // Get and send metering data to web service
            request_json = getMeteringStop();
            getLicense( request_json, mWSApiRetryDuration * 1000, mWSRetryPeriodShort * 1000 );
            MTX_RELEASE( mMeteringAccessMutex );
            Debug( "Session ID {} stopped and last metering data uploaded", mSessionID );
        } catch( const Exception& e ) {}

        mExpirationTime = TClock::time_point();
        Debug( "Reset expiration time" );
        // Clear security flag
        Debug( "Clearing stop security flag" );
        mSecurityStop = false;
        // Clear Session ID
        Info( "DRM session {} stopped.", mSessionID );
        mSessionID = std::string("");
        mEntitlementID = std::string("");
    }

    ParameterKey findParameterKey( const std::string& key_string ) const {
        for ( auto const& it : mParameterKeyMap ) {
            if ( key_string == it.second ) {
                return it.first;
            }
        }
        Throw( DRM_BadArg, "Cannot find parameter: {}. ", key_string );
    }

    std::string findParameterString( const ParameterKey key_id ) const {
        std::map<ParameterKey, std::string>::const_iterator it;
        it = mParameterKeyMap.find( key_id );
        if ( it == mParameterKeyMap.end() )
            Throw( DRM_BadArg, "Cannot find parameter with ID: {}. ", (uint32_t)key_id );
        return it->second;
    }

    Json::Value list_parameter_key() const {
        Json::Value node;
        for( int i=0; i<ParameterKey::ParameterKeyCount; i++ ) {
            ParameterKey e = static_cast<ParameterKey>( i );
            std::string keyStr = findParameterString( e );
            node.append( keyStr );
        }
        return node;
    }

    Json::Value dump_parameter_key() const {
        Json::Value node;
        for( int i=0; i<ParameterKey::dump_all; i++ ) {
            ParameterKey e = static_cast<ParameterKey>( i );
            std::string keyStr = findParameterString( e );
            node[ keyStr ] = Json::nullValue;
        }
        get( node );
        return node;
    }


public:

    // Non copyable non movable as we create closure with "this"
    Impl( const Impl& ) = delete;
    Impl( Impl&& ) = delete;

    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path,
          ReadRegisterCallback f_user_read_register,
          WriteRegisterCallback f_user_write_register,
          AsynchErrorCallback f_user_asynch_error )
        : Impl( conf_file_path, cred_file_path )
    {
        TRY
            Debug( "Calling Impl public constructor" );
            const char* ctrl_timeout = getenv( "DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS" );
            const char* ctrl_sleep = getenv( "DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS" );
            if ( f_user_asynch_error )
                f_asynch_error = f_user_asynch_error;
            // Determine DRM Ctrl TA existance by trying to initialize it
            mIsPnR = pnc_initialize_drm_ctrl_ta();
            if ( mIsPnR ) {
                mIsHybrid = true;
                //  Set Ctrl TA logging
                updateCtrlLogLevel( sLogCtrlVerbosity, true );
                //  Set callbacks
                f_read_register = [&]( uint32_t  offset, uint32_t *value ) {
                    return pnc_read_drm_ctrl_ta( offset, value );
                };
                f_write_register = [&]( uint32_t  offset, uint32_t value ) {
                    return pnc_write_drm_ctrl_ta(offset, value );
                };
            } else {
                f_read_register = f_user_read_register;
                f_write_register = f_user_write_register;
            }
            if ( mIsHybrid ) {
                if ( mIsPnR )
                    Debug( "DRM Controller is a PnR Trusted Application" );
                else
                    Debug( "DRM Controller is a Software Application" );
                // Set sleep period because SW Controller is slower
                mCtrlTimeFactor = 100;
            } else {
                Debug( "DRM Controller is a Hardware IP" );
                mCtrlTimeFactor = 1;
            }
            uint32_t timeout_period = SDK_CTRL_TIMEOUT_IN_US * mCtrlTimeFactor;
            uint32_t sleep_period = SDK_CTRL_SLEEP_IN_US * mCtrlTimeFactor;
            std::string s_timeout_period = std::to_string(timeout_period);
            std::string s_sleep_period = std::to_string(sleep_period);
            if (ctrl_timeout == NULL) {
                Debug( "DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS variable is not defined, use default {}", s_timeout_period.c_str() );
                setenv("DRM_CONTROLLER_TIMEOUT_IN_MICRO_SECONDS", s_timeout_period.c_str() , 0);
            }
            if (ctrl_sleep == NULL) {
                Debug( "DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS variable is not defined, use default {}", s_sleep_period.c_str() );
                setenv("DRM_CONTROLLER_SLEEP_IN_MICRO_SECONDS", s_sleep_period.c_str(), 0);
            }
            if ( !f_read_register )
                Throw( DRM_BadArg, "Read register callback function must not be NULL. " );
            if ( !f_write_register )
                Throw( DRM_BadArg, "Write register callback function must not be NULL. " );
            if ( !f_asynch_error )
                Throw( DRM_BadArg, "Asynchronous error callback function must not be NULL. " );
            initDrmInterface();
            Debug( "Exiting Impl public constructor" );
        CATCH_AND_THROW
    }

    ~Impl() {
        try {
            TRY
                Debug( "Calling Impl destructor" );
                if ( mSecurityStop && isSessionRunning() ) {
                    Debug( "Security stop triggered: stopping current session" );
                    stopSession();
                } else {
                    stopThread();
                }

            CATCH_AND_THROW
        } catch(...) {}
        unlockDrmToInstance();
        pnc_uninitialize_drm_ctrl_ta();
        Debug( "Exiting Impl destructor" );
        sLogger->flush();
    }

    void activate() {
        TRY
            Debug( "Calling 'activate'" );

            // If a floating/metering session is still running, try to close it gracefully.
            if ( !isConfigInNodeLock() && isSessionRunning() ) {
                Debug( "The floating/metering session is still pending: trying to close it gracefully." );
                try {
                    stopSession();
                } catch( const Exception& e ) {
                    Debug( "Failed to stop gracefully the pending session" );
                }
            }
            if ( isConfigInNodeLock() ) {
                // Install the node-locked license
                installNodelockedLicense();
                return;
            }
            if ( isDrmCtrlInNodelock() ) {
                Throw( DRM_BadUsage, "DRM Controller is locked in Node-Locked licensing mode: "
                                    "To use other modes you must reprogram the FPGA device. " );
            }
            // Start new session and the background threads
            mThreadExit = false;
            startSession();
            startLicenseContinuityThread();
            mSecurityStop = true;
        CATCH_AND_THROW
    }

    void deactivate() {
        TRY
            Debug( "Calling 'deactivate'" );

            if ( isConfigInNodeLock() ) {
                return;
            }
            if ( !isSessionRunning() ) {
                Debug( "No session is currently running" );
                return;
            }
            stopSession();
        CATCH_AND_THROW
    }

    void get( Json::Value& json_value ) const {
        TRY
            for( const std::string& key_str : json_value.getMemberNames() ) {
                const ParameterKey key_id = findParameterKey( key_str );
                Debug2( "Getting parameter '{}'", key_str );
                switch( key_id ) {
                    case ParameterKey::log_verbosity: {
                        uint32_t logVerbosity = static_cast<uint32_t>( sLogConsoleVerbosity );
                        json_value[key_str] = logVerbosity;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                logVerbosity );
                        break;
                    }
                    case ParameterKey::log_format: {
                        json_value[key_str] = sLogConsoleFormat;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                sLogConsoleFormat );
                        break;
                    }
                    case ParameterKey::log_file_verbosity: {
                        uint32_t logVerbosity = static_cast<uint32_t>( sLogFileVerbosity );
                        json_value[key_str] = logVerbosity;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                logVerbosity );
                        break;
                    }
                    case ParameterKey::log_file_format: {
                        json_value[key_str] = sLogFileFormat;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                sLogFileFormat );
                        break;
                    }
                    case ParameterKey::log_file_path: {
                        json_value[key_str] = sLogFilePath;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                sLogFilePath );
                        break;
                    }
                    case ParameterKey::log_file_type: {
                        json_value[key_str] = (uint32_t)sLogFileType;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               (uint32_t)sLogFileType );
                        break;
                    }
                    case ParameterKey::log_file_rotating_num: {
                        json_value[key_str] = (uint32_t)sLogFileRotatingNum;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               sLogFileRotatingNum );
                        break;
                    }
                    case ParameterKey::log_file_rotating_size: {
                        json_value[key_str] = (uint32_t)sLogFileRotatingSize;
                        Debug( "Get value of parameter '{}' (ID={}): {} KB", key_str, (uint32_t)key_id,
                               sLogFileRotatingSize );
                        break;
                    }
                    case ParameterKey::license_type: {
                        auto it = LicenseTypeStringMap.find( mLicenseType );
                        if ( it == LicenseTypeStringMap.end() )
                            Unreachable( "License_type '{}' is missing in LicenseTypeStringMap. ", (uint32_t)mLicenseType ); //LCOV_EXCL_LINE
                        std::string license_type_str = it->second;
                        json_value[key_str] = license_type_str;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                license_type_str );
                        break;
                    }
                    case ParameterKey::license_duration: {
                        json_value[key_str] = mLicenseDuration;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mLicenseDuration );
                        break;
                    }
                    case ParameterKey::num_activators: {
                        uint32_t nbActivators = 0;
                        getNumActivator( nbActivators );
                        json_value[key_str] = nbActivators;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                nbActivators );
                        break;
                    }
                    case ParameterKey::session_id: {
                        json_value[key_str] = mSessionID;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mSessionID );
                        break;
                    }
                    case ParameterKey::session_status: {
                        bool status = isSessionRunning();
                        json_value[key_str] = status;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               status );
                        break;
                    }
                    case ParameterKey::license_status: {
                        bool status = isLicenseActive();
                        json_value[key_str] = status;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               status );
                        break;
                    }
                    case ParameterKey::metered_data: {
                        #if ((JSONCPP_VERSION_MAJOR ) >= 1 and ((JSONCPP_VERSION_MINOR) > 7 or ((JSONCPP_VERSION_MINOR) == 7 and JSONCPP_VERSION_PATCH >= 5)))
                        uint64_t ip_metering = 0;
                        #else
                        // No "int64_t" support with JsonCpp < 1.7.5
                        unsigned long long int ip_metering = 0;
                        #endif
                        Json::Value json_request = getMeteringHealth();
                        std::string meteringFileStr = json_request["drm_config"]["metering_file"].asString();
                        if  ( meteringFileStr.size() ) {
                            std::vector<std::string> meteringFileList = splitByLength( meteringFileStr, 32 );
                            std::vector<std::string> meteringDataList = std::vector<std::string>(meteringFileList.begin() + 2, meteringFileList.end()-1);
                            Json::Value meteringIntList;
                            for ( auto meteringData: meteringDataList ) {
                                uint32_t ip_idx = (uint32_t)str2int64( meteringData.substr( 0, 16 ) );
                                ip_metering = str2int64( meteringData.substr( 16 ) );
                                Debug("Metering for IP#{}: {}", ip_idx, ip_metering);
                                json_value[key_str].append( ip_metering );
                            }
                        } else {
                            json_value[key_str].append(0);
                        }
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               json_value[key_str].toStyledString() );
                        break;
                    }
                    case ParameterKey::nodelocked_request_file: {
                        if ( mLicenseType != eLicenseType::NODE_LOCKED ) {
                            json_value[key_str] = std::string("Not applicable");
                            Warning( "Parameter only available with Node-Locked licensing" );
                        } else {
                            json_value[key_str] = mNodeLockRequestFilePath;
                            Debug( "Get value of parameter '{}' (ID={}): Node-locked license request file is saved in {}",
                                    key_str, (uint32_t)key_id, mNodeLockRequestFilePath );
                        }
                        break;
                    }
                    case ParameterKey::hw_report: {
                        std::string str = getDrmReport();
                        json_value[key_str] = str;
                        Debug( "Get value of parameter '{}' (ID={})", key_str, (uint32_t)key_id );
                        Info( "Print HW report:\n{}", str );
                        break;
                    }
                    case ParameterKey::drm_frequency: {
                        json_value[key_str] = mFrequencyCurr;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mFrequencyCurr );
                        break;
                    }
                    case ParameterKey::drm_license_type: {
                        eLicenseType lic_type;
                        bool is_nodelock = isDrmCtrlInNodelock();
                        bool is_metering = isDrmCtrlInMetering();
                        if ( is_metering )
                            lic_type = eLicenseType::METERED;
                        else if ( is_nodelock )
                            lic_type = eLicenseType::NODE_LOCKED;
                        else
                            lic_type = eLicenseType::NONE;
                        auto it = LicenseTypeStringMap.find( lic_type );
                        std::string status = it->second;
                        json_value[key_str] = status;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               status );
                        break;
                    }
                    case ParameterKey::bypass_frequency_detection: {
                        json_value[key_str] = mBypassFrequencyDetection;
                        Debug( "Get value of parameter '{}' (ID={}): Method {}", key_str, (uint32_t)key_id,
                               mBypassFrequencyDetection );
                        break;
                    }
                    case ParameterKey::frequency_detection_method: {
                        json_value[key_str] = mFreqDetectionMethod;
                        Debug( "Get value of parameter '{}' (ID={}): Method {}", key_str, (uint32_t)key_id,
                               mFreqDetectionMethod );
                        break;
                    }
                    case ParameterKey::frequency_detection_threshold: {
                        json_value[key_str] = mFrequencyDetectionThreshold;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mFrequencyDetectionThreshold );
                        break;
                    }
                    case ParameterKey::frequency_detection_period: {
                        json_value[key_str] = mFrequencyDetectionPeriod;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mFrequencyDetectionPeriod );
                        break;
                    }
                    case ParameterKey::product_info: {
                        json_value[key_str] = mHeaderJsonRequest["product"];
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mHeaderJsonRequest["product"].toStyledString() );
                        break;
                    }
                    case ParameterKey::token_string: {
                        std::string token_str = getDrmWSClient().getTokenString();
                        json_value[key_str] = token_str;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               token_str );
                        break;
                    }
                    case ParameterKey::token_validity: {
                        uint32_t validity = getDrmWSClient().getTokenValidity();
                        json_value[key_str] = validity ;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               validity  );
                        break;
                    }
                    case ParameterKey::token_time_left: {
                        uint32_t time_left = getDrmWSClient().getTokenTimeLeft();
                        json_value[key_str] = time_left;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               time_left );
                        break;
                    }
                    case ParameterKey::mailbox_size: {
                        uint32_t mbSize = getUserMailboxSize();
                        json_value[key_str] = mbSize;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mbSize );
                        break;
                    }
                    case ParameterKey::mailbox_data: {
                        uint32_t mbSize = getUserMailboxSize();
                        std::vector<uint32_t> data_array = readMailbox( eMailboxOffset::MB_USER, mbSize );
                        for( const auto& val: data_array )
                            json_value[key_str].append( val );
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               json_value[key_str].toStyledString() );
                        break;
                    }
                    case ParameterKey::ws_retry_period_long: {
                        json_value[key_str] = mWSRetryPeriodLong;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mWSRetryPeriodLong );
                        break;
                    }
                    case ParameterKey::ws_retry_period_short: {
                        json_value[key_str] = mWSRetryPeriodShort;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mWSRetryPeriodShort );
                        break;
                    }
                    case ParameterKey::ws_api_retry_duration: {
                        json_value[key_str] = mWSApiRetryDuration ;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mWSApiRetryDuration );
                        break;
                    }
                    case ParameterKey::ws_request_timeout: {
                        int32_t req_timeout_sec = (int32_t)(getDrmWSClient().getRequestTimeoutMS() / 1000);
                        json_value[key_str] = req_timeout_sec;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id, req_timeout_sec );
                        break;
                    }
                    case ParameterKey::log_message_level: {
                        uint32_t msgLevel = static_cast<uint32_t>( mDebugMessageLevel );
                        json_value[key_str] = msgLevel;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               msgLevel );
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = readMailbox<uint32_t>( eMailboxOffset::MB_CUSTOM_FIELD );
                        json_value[key_str] = customField;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               customField );
                        break;
                    }
                    case ParameterKey ::list_all: {
                        Json::Value list = list_parameter_key();
                        json_value[key_str] = list;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               list.toStyledString() );
                        break;
                    }
                    case ParameterKey::dump_all: {
                        Json::Value list = dump_parameter_key();
                        json_value[key_str] = list;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               list.toStyledString() );
                        break;
                    }
                    case ParameterKey::hdk_compatibility: {
                        std::string hdk_limit = fmt::format( "{}.{}", HDK_COMPATIBILITY_LIMIT_MAJOR, HDK_COMPATIBILITY_LIMIT_MINOR );
                        json_value[key_str] = hdk_limit;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               hdk_limit );
                        break;
                    }
                    case ParameterKey::health_period: {
                        MTX_ACQUIRE( mHealthAccessMutex );
                        json_value[key_str] = mHealthPeriod;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id, mHealthPeriod );
                        MTX_RELEASE( mHealthAccessMutex );
                        break;
                    }
                    case ParameterKey::health_retry: {
                        MTX_ACQUIRE( mHealthAccessMutex );
                        json_value[key_str] = mHealthRetryTimeout;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id, mHealthRetryTimeout );
                        MTX_RELEASE( mHealthAccessMutex );
                        break;
                    }
                    case ParameterKey::health_retry_sleep: {
                        MTX_ACQUIRE( mHealthAccessMutex );
                        json_value[key_str] = mHealthRetrySleep;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id, mHealthRetrySleep );
                        MTX_RELEASE( mHealthAccessMutex );
                        break;
                    }
                    case ParameterKey::host_data_verbosity: {
                        uint32_t dataLevel = static_cast<uint32_t>( mHostDataVerbosity );
                        json_value[key_str] = dataLevel;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               dataLevel );
                        break;
                    }
                    case ParameterKey::host_data: {
                        json_value[key_str] = mDiagnostics;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               mDiagnostics.toStyledString() );
                        break;
                    }
                    case ParameterKey::log_file_append: {
                        json_value[key_str] = sLogFileAppend;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               sLogFileAppend );
                        break;
                    }
                    case ParameterKey::ws_verbosity: {
                        uint32_t wsVerbosity = getDrmWSClient().getVerbosity();
                        json_value[key_str] = wsVerbosity;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               wsVerbosity );
                        break;
                    }
                    case ParameterKey::trng_status: {
                        Json::Value trng_status_json;
                        bool securityAlertBit( false );
                        uint32_t adaptiveProportionTestError, repetitionCountTestError;
                        getTrngStatus( securityAlertBit, adaptiveProportionTestError, repetitionCountTestError );
                        trng_status_json["security_alert_bit"] = securityAlertBit;
                        trng_status_json["adaptive_proportion_test_error"] = adaptiveProportionTestError;
                        trng_status_json["repetition_count_test_error"] = repetitionCountTestError;
                        json_value[key_str] = trng_status_json;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               trng_status_json.toStyledString() );
                        break;
                    }
                    case ParameterKey::num_license_loaded: {
                        uint32_t numberOfLicenseProvisioned;
                        checkDRMCtlrRet( getDrmController().readNumberOfLicenseTimerLoadedStatusRegister(
                                    numberOfLicenseProvisioned ) );
                        json_value[key_str] = numberOfLicenseProvisioned;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               numberOfLicenseProvisioned );
                        break;
                    }
                    case ParameterKey::ws_connection_timeout: {
                        int32_t con_timeout_sec = (int32_t)(getDrmWSClient().getConnectionTimeoutMS() / 1000);
                        json_value[key_str] = con_timeout_sec;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id, con_timeout_sec );
                        break;
                    }
                    case ParameterKey::log_ctrl_verbosity: {
                        uint32_t logVerbosity = static_cast<uint32_t>( sLogCtrlVerbosity );
                        json_value[key_str] = logVerbosity;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                logVerbosity );
                        break;
                    }
                    case ParameterKey::is_drm_software: {
                        json_value[key_str] = mIsHybrid;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mIsHybrid );
                        break;
                    }
                    case ParameterKey::controller_version: {
                        json_value[key_str] = mDrmVersionStr;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mDrmVersionStr );
                        break;
                    }
                    case ParameterKey::controller_rom: {
                        json_value[key_str] = mMailboxRoData;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mMailboxRoData.toStyledString() );
                        break;
                    }
                    case ParameterKey::license_counter: {
                        json_value[key_str] = mLicenseCounter;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mLicenseCounter );
                        break;
                    }
                    case ParameterKey::health_counter: {
                        MTX_ACQUIRE( mHealthAccessMutex );
                        json_value[key_str] = mHealthCounter;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mHealthCounter );
                        MTX_RELEASE( mHealthAccessMutex );
                        break;
                    }
                    case ParameterKey::entitlement_session_id: {
                        json_value[key_str] = mEntitlementID;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                                mEntitlementID );
                        break;
                    }
                    case ParameterKey::ParameterKeyCount: {
                        uint32_t count = static_cast<uint32_t>( ParameterKeyCount );
                        json_value[key_str] = count;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, (uint32_t)key_id,
                               count );
                        break;
                    }
                    default: {
                        Throw( DRM_BadArg, "Parameter '{}' cannot be read. ", key_str );
                        break;
                    }
                }
            }
        CATCH_AND_THROW
    }

    void get( std::string& json_string ) const {
        TRY
            Debug2( "Calling 'get' with in/out string: {}", json_string );
            Json::Value root = parseJsonString( json_string );
            get( root );
            json_string = root.toStyledString();
        CATCH_AND_THROW
    }

    template<typename T> T get( const ParameterKey /*key_id*/ ) const {
        Unreachable( "Default template for get function. " ); //LCOV_EXCL_LINE
    }

    void set( const Json::Value& json_value ) {
        TRY
            for( Json::ValueConstIterator it = json_value.begin() ; it != json_value.end() ; it++ ) {
                std::string key_str = it.key().asString();
                const ParameterKey key_id = findParameterKey( key_str );
                switch( key_id ) {
                    case ParameterKey::log_verbosity: {
                        int32_t verbosityInt = (*it).asInt();
                        sLogConsoleVerbosity = static_cast<spdlog::level::level_enum>( verbosityInt );
                        sLogger->sinks()[0]->set_level( sLogConsoleVerbosity );
                        if ( sLogConsoleVerbosity < sLogger->level() )
                            sLogger->set_level( sLogConsoleVerbosity );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                                verbosityInt );
                        break;
                    }
                    case ParameterKey::log_format: {
                        std::string logFormat = (*it).asString();
                        sLogger->sinks()[0]->set_pattern( logFormat );
                        sLogConsoleFormat = logFormat;
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                                sLogConsoleFormat );
                        break;
                    }
                    case ParameterKey::log_file_verbosity: {
                        int32_t verbosityInt = (*it).asInt();
                        sLogFileVerbosity = static_cast<spdlog::level::level_enum>( verbosityInt );
                        sLogger->sinks()[1]->set_level( sLogFileVerbosity );
                        if ( sLogFileVerbosity < sLogger->level() )
                            sLogger->set_level( sLogFileVerbosity );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               verbosityInt);
                        break;
                    }
                    case ParameterKey::log_file_format: {
                        sLogFileFormat = (*it).asString();
                        if ( sLogger->sinks().size() > 1 ) {
                            sLogger->sinks()[1]->set_pattern( sLogFileFormat );
                        }
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               sLogFileFormat );
                        break;
                    }
                    case ParameterKey::frequency_detection_threshold: {
                        mFrequencyDetectionThreshold = (*it).asDouble();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               mFrequencyDetectionThreshold );
                        break;
                    }
                    case ParameterKey::frequency_detection_period: {
                        mFrequencyDetectionPeriod = (*it).asUInt();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               mFrequencyDetectionPeriod );
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = (*it).asUInt();
                        writeMailbox<uint32_t>( eMailboxOffset::MB_CUSTOM_FIELD, customField );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               customField );
                        break;
                    }
                    case ParameterKey::mailbox_data: {
                        if ( !(*it).isArray() )
                            Throw( DRM_BadArg, "Value must be an array of integers. " );
                        std::vector<uint32_t> data_array;
                        for( Json::ValueConstIterator itr = (*it).begin(); itr != (*it).end(); itr++ )
                            data_array.push_back( (*itr).asUInt() );
                        writeMailbox( eMailboxOffset::MB_USER, data_array );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               (*it).toStyledString());
                        break;
                    }
                    case ParameterKey::ws_retry_period_long: {
                        uint32_t retry_period = (*it).asUInt();
                        if ( retry_period <= mWSRetryPeriodShort )
                            Throw( DRM_BadArg,
                                    "ws_retry_period_long ({}) must be greater than ws_retry_period_short ({}). ",
                                    retry_period, mWSRetryPeriodShort );
                        mWSRetryPeriodLong = retry_period;
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               mWSRetryPeriodLong );
                        break;
                    }
                    case ParameterKey::ws_retry_period_short: {
                        uint32_t retry_period = (*it).asUInt();
                        if ( mWSRetryPeriodLong <= retry_period )
                            Throw( DRM_BadArg,
                                    "ws_retry_period_long ({}) must be greater than ws_retry_period_short ({}). ",
                                    mWSRetryPeriodLong, retry_period );
                        mWSRetryPeriodShort = retry_period;
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               mWSRetryPeriodShort );
                        break;
                    }
                    case ParameterKey::ws_api_retry_duration: {
                        mWSApiRetryDuration = (*it).asUInt();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               mWSApiRetryDuration );
                        break;
                    }
                    case ParameterKey::trigger_async_callback: {
                        std::string custom_msg = (*it).asString();
                        Exception e( DRM_Debug, custom_msg );
                        f_asynch_error( e.what() );
                        sLogger->flush();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               custom_msg );
                        break;
                    }
                    case ParameterKey::log_message_level: {
                        int32_t message_level = (*it).asInt();
                        if ( ( message_level < spdlog::level::trace)
                          || ( message_level > spdlog::level::off) )
                            Throw( DRM_BadArg, "log_message_level ({}) is out of range [{:d}:{:d}] ",
                                    message_level, (int32_t)spdlog::level::trace, (int32_t)spdlog::level::off );
                        mDebugMessageLevel = static_cast<spdlog::level::level_enum>( message_level );
                        Debug( "Set parameter '{}' (ID={}) to value {}", key_str, (uint32_t)key_id,
                                message_level );
                        break;
                    }
                    case ParameterKey::log_message: {
                        std::string custom_msg = (*it).asString();
                        SPDLOG_LOGGER_CALL( sLogger, (spdlog::level::level_enum)mDebugMessageLevel, custom_msg);
                        break;
                    }
                    case ParameterKey::log_ctrl_verbosity: {
                        int32_t verbosityInt = (*it).asInt();
                        eCtrlLogVerbosity level_e = static_cast<eCtrlLogVerbosity>( verbosityInt );
                        updateCtrlLogLevel( level_e );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, (uint32_t)key_id,
                               verbosityInt);
                        break;
                    }
                    default:
                        Throw( DRM_BadArg, "Parameter '{}' cannot be overwritten. ", key_str );
                }
            }
        CATCH_AND_THROW
    }

    void set( const std::string& json_string ) {
        Debug2( "Calling 'set' with in/out string: {}", json_string );
        Json::Value root = parseJsonString( json_string );
        set( root );
    }

    template<typename T> void set( const ParameterKey /*key_id*/, const T& /*value*/ ) {}

};

/*************************************/
// DrmManager::Impl class definition
/*************************************/

#define IMPL_GET_BODY \
    Json::Value json_value; \
    std::string key_str = findParameterString( key_id ); \
    json_value[key_str] = Json::nullValue; \
    get( json_value );


template<> std::string DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        if ( json_value[key_str].isString() )
            return json_value[key_str].asString();
        return json_value[key_str].toStyledString();
    CATCH_AND_THROW
}

template<> bool DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asBool();
    CATCH_AND_THROW
}

template<> int32_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asInt();
    CATCH_AND_THROW
}

template<> uint32_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asUInt();
    CATCH_AND_THROW
}

template<> int64_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asInt64();
    CATCH_AND_THROW
}

template<> uint64_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asUInt64();
    CATCH_AND_THROW
}

template<> float DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asFloat();
    CATCH_AND_THROW
}

template<> double DrmManager::Impl::get( const ParameterKey key_id ) const {
    TRY
        IMPL_GET_BODY
        return json_value[key_str].asDouble();
    CATCH_AND_THROW
}

#define IMPL_SET_BODY \
    Json::Value json_value; \
    std::string key_str = findParameterString( key_id ); \
    json_value[key_str] = value; \
    set( json_value );


template<> void DrmManager::Impl::set( const ParameterKey key_id, const std::string& value ) {
    TRY
        IMPL_SET_BODY
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const bool& value ) {
    TRY
        IMPL_SET_BODY
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const int32_t& value ) {
    TRY
        IMPL_SET_BODY
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const uint32_t& value ) {
    TRY
        IMPL_SET_BODY
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const int64_t& value ) {
    TRY
        Json::Value json_value;
        std::string key_str = findParameterString( key_id );
        json_value[key_str] = Json::Int64( value );
        set( json_value );
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const uint64_t& value ) {
    TRY
        Json::Value json_value;
        std::string key_str = findParameterString( key_id );
        json_value[key_str] = Json::UInt64( value );
        set( json_value );
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const float& value ) {
    TRY
        IMPL_SET_BODY
    CATCH_AND_THROW
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const double& value ) {
    TRY
        IMPL_SET_BODY
    CATCH_AND_THROW
}


/*************************************/
// DrmManager class definition
/*************************************/

DrmManager::DrmManager( const std::string& conf_file_path,
                    const std::string& cred_file_path,
                    ReadRegisterCallback read_register,
                    WriteRegisterCallback write_register,
                    AsynchErrorCallback async_error )
    : pImpl( new Impl( conf_file_path, cred_file_path, read_register, write_register, async_error ) ) {
}

DrmManager::~DrmManager() {
    delete pImpl;
    pImpl = nullptr;
}


void DrmManager::activate() {
    pImpl->activate();
}

void DrmManager::deactivate() {
    pImpl->deactivate();
}

void DrmManager::get( Json::Value& json_value ) const {
    pImpl->get( json_value );
}

void DrmManager::get( std::string& json_string ) const {
    pImpl->get( json_string );
}

template<typename T> T DrmManager::get( const ParameterKey key ) const {
    return pImpl->get<T>( key );
}

template<> std::string DrmManager::get( const ParameterKey key ) const { return pImpl->get<std::string>( key ); }
template<> bool DrmManager::get( const ParameterKey key ) const { return pImpl->get<bool>( key ); }
template<> int32_t DrmManager::get( const ParameterKey key ) const { return pImpl->get<int32_t>( key ); }
template<> uint32_t DrmManager::get( const ParameterKey key ) const { return pImpl->get<uint32_t>( key ); }
template<> int64_t DrmManager::get( const ParameterKey key ) const { return pImpl->get<int64_t>( key ); }
template<> uint64_t DrmManager::get( const ParameterKey key ) const { return pImpl->get<uint64_t>( key ); }
template<> float DrmManager::get( const ParameterKey key ) const { return pImpl->get<float>( key ); }
template<> double DrmManager::get( const ParameterKey key ) const { return pImpl->get<double>( key ); }

void DrmManager::set( const std::string& json_string ) {
    pImpl->set( json_string );
}

void DrmManager::set( const Json::Value& json_value ) {
    pImpl->set( json_value );
}

template<typename T>
void DrmManager::set( const ParameterKey key, const T& value ) {
    pImpl->set<T>( key, value );
}

template<> void DrmManager::set( const ParameterKey key, const std::string& value ) { pImpl->set<std::string>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const bool& value ) { pImpl->set<bool>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const int32_t& value ) { pImpl->set<int32_t>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const uint32_t& value ) { pImpl->set<uint32_t>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const int64_t& value ) { pImpl->set<int64_t>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const uint64_t& value ) { pImpl->set<uint64_t>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const float& value ) { pImpl->set<float>( key, value ); }
template<> void DrmManager::set( const ParameterKey key, const double& value ) { pImpl->set<double>( key, value ); }

}
}
