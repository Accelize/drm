/*
Copyright (C) 2018, Accelize

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

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <json/json.h>
#include <json/version.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <typeinfo>
#include <sys/types.h>
#include <sys/stat.h>
#include <cmath>
#include <algorithm>

#include "accelize/drm/drm_manager.h"
#include "accelize/drm/version.h"
#include "ws_client.h"
#include "log.h"
#include "utils.h"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "DrmControllerDataConverter.hpp"
#include "HAL/DrmControllerOperations.hpp"
#pragma GCC diagnostic pop

#define NB_MAX_REGISTER  32

#define REG_FREQ_DETECTION_VERSION  0xFFF8
#define REG_FREQ_DETECTION_COUNTER  0xFFFC

#define FREQ_DETECTION_VERSION_EXPECTED	 0x60DC0DE0

static const std::string DRM_SELF_TEST_ERROR_MESSAGE( "Could not access DRM Controller registers.\nPlease verify:\n"
                        "\t-The read/write callbacks implementation in the SW application: verify it uses the correct offset address of DRM Controller IP in the design address space.\n"
                        "\t-The DRM Controller IP instantiation in the FPGA design: verify the correctness of 16-bit address received by the AXI-Lite port of the DRM Controller." );

#define TRY try {

#define CATCH_AND_THROW                     \
    } catch( const std::exception &e ) {    \
        Fatal( e.what() );                  \
        throw;                              \
    }


namespace Accelize {
namespace DRM {


const char* getApiVersion() {
    return DRMLIB_VERSION;
}


class DRM_LOCAL DrmManager::Impl {

protected:

    // Helper typedef
    typedef std::chrono::steady_clock TClock; /// Shortcut type def to steady clock which is monotonic (so unaffected by clock adjustments)

    // Enum
    enum class eLogFileType: uint8_t {NONE=0, BASIC, ROTATING};
    enum class eLicenseType: uint8_t {METERED, NODE_LOCKED, NONE};
    enum class eMailboxOffset: uint8_t {MB_LOCK_DRM=0, MB_CUSTOM_FIELD, MB_USER};

    // Design constants
    const uint32_t HDK_COMPATIBILITY_LIMIT_MAJOR = 3;
    const uint32_t HDK_COMPATIBILITY_LIMIT_MINOR = 1;

    const std::map<eLicenseType, std::string> LicenseTypeStringMap = {
            {eLicenseType::NONE       , "Idle"},
            {eLicenseType::METERED    , "Floating/Metering"},
            {eLicenseType::NODE_LOCKED, "Node-Locked"}
    };

    const uint32_t LICENSE_DURATION_DEFAULT = 30;

    const double ACTIVATIONCODE_TRANSMISSION_TIMEOUT_MS = 2000.0;


#ifdef _WIN32
    const char path_sep = '\\';
# else
    const char path_sep = '/';
#endif

    // HDK version
    uint32_t mDrmVersion = 0;

    bool mSecurityStop = false;

    // Composition
    std::unique_ptr<DrmWSClient> mWsClient;
    std::unique_ptr<DrmControllerLibrary::DrmControllerOperations> mDrmController;
    mutable std::recursive_mutex mDrmControllerMutex;
    bool mIsLockedToDrm = false;

    // Logging parameters
    spdlog::level::level_enum sLogConsoleVerbosity = spdlog::level::err;
    std::string sLogConsoleFormat = std::string("[%^%=8l%$] %-6t, %v");

    spdlog::level::level_enum sLogFileVerbosity = spdlog::level::info;
    std::string  sLogFileFormat       = std::string("%Y-%m-%d %H:%M:%S.%e - %18s:%-4# [%=8l] %=6t, %v");
    eLogFileType sLogFileType         = eLogFileType::NONE;
    std::string  sLogFilePath         = fmt::format( "accelize_drmlib_{}.log", getpid() );
    size_t       sLogFileRotatingSize = 100*1024; ///< Size max in KBytes of the log roating file
    size_t       sLogFileRotatingNum  = 3;

    // Function callbacks
    DrmManager::ReadRegisterCallback  f_read_register;
    DrmManager::WriteRegisterCallback f_write_register;
    DrmManager::AsynchErrorCallback   f_asynch_error;

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
    uint32_t mWSRequestTimeout   = 10;    ///< Time in seconds during which retries occur on activate and deactivate functions

    eLicenseType mLicenseType = eLicenseType::METERED;
    uint32_t mLicenseDuration = 0;        ///< Time duration in seconds of the license
    uint32_t mLicenseWaitPeriod = 5;      ///< Time in seconds to wait for the load of a new license

    // To protect access to the metering data (to securize the segment ID check in HW)
    mutable std::mutex mMeteringAccessMutex;

    // Design parameters
    int32_t mFrequencyInit = 0;
    int32_t mFrequencyCurr = 0;
    uint32_t mFrequencyDetectionPeriod = 100;       // in milliseconds
    double mFrequencyDetectionThreshold = 12.0;     // Error in percentage
    bool mIsFreqDetectionMethod1 = false;
    bool mBypassFrequencyDetection = false;

    // Session state
    std::string mSessionID;
    std::string mUDID;
    std::string mBoardType;

    // Web service communication
    Json::Value mHeaderJsonRequest;

    // Health/Asynchronous metering parameters
    uint32_t mHealthPeriod;
    uint32_t mHealthRetryTimeout;
    uint32_t mHealthRetrySleep;

    // thread to maintain license alive
    uint32_t mLicenseCounter = 0;
    std::future<void> mThreadKeepAlive;

    // thread to maintain health alive
    mutable uint32_t mHealthCounter = 0;
    std::future<void> mThreadHealth;

    // All thread exit elements
    std::mutex mThreadExitMtx;
    std::condition_variable mThreadExitCondVar;
    bool mThreadExit{false};


    // Debug parameters
    spdlog::level::level_enum mDebugMessageLevel;

    // User accessible parameters
    const std::map<ParameterKey, std::string> mParameterKeyMap = {
    #   define PARAMETERKEY_ITEM(id) {id, #id},
    #   include "accelize/drm/ParameterKey.def"
    #   undef PARAMETERKEY_ITEM
        {ParameterKeyCount, "ParameterKeyCount"}
    };


    #define checkDRMCtlrRet( func ) {                                                  \
        unsigned int errcode = DRM_OK;                                                 \
        try {                                                                          \
            errcode = func;                                                            \
            Debug( "{} returned {}", #func, errcode );                                 \ 
        } catch( const std::exception &e ) {                                           \
            Debug( "{} threw an exception", #func );                                   \
            Throw( DRM_CtlrError, e.what() );                                          \
        }                                                                              \
        if ( errcode )                                                                 \
            Unreachable( "DRM Controller API failed with error code: {}.", errcode );  \
    }

    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path )
    {
        // Basic logging setup
        initLog();

        Json::Value conf_json;

        mSecurityStop = false;
        mIsLockedToDrm = false;

        mLicenseCounter = 0;
        mLicenseDuration = LICENSE_DURATION_DEFAULT;

        mConfFilePath = conf_file_path;
        mCredFilePath = cred_file_path;

        mFrequencyInit = 0;
        mFrequencyCurr = 0;

        mDebugMessageLevel = spdlog::level::trace;

        // Parse configuration file
        conf_json = parseJsonFile( conf_file_path );

        try {
            Json::Value param_lib = JVgetOptional( conf_json, "settings", Json::objectValue );
            if ( param_lib != Json::nullValue ) {
                // Console logging
                sLogConsoleVerbosity = static_cast<spdlog::level::level_enum>( JVgetOptional(
                        param_lib, "log_verbosity", Json::intValue, (int)sLogConsoleVerbosity ).asInt());
                sLogConsoleFormat = JVgetOptional(
                        param_lib, "log_format", Json::stringValue, sLogConsoleFormat ).asString();

                // File logging
                sLogFileVerbosity = static_cast<spdlog::level::level_enum>( JVgetOptional(
                        param_lib, "log_file_verbosity", Json::intValue, (int)sLogFileVerbosity ).asInt() );
                sLogFileFormat = JVgetOptional(
                        param_lib, "log_file_format", Json::stringValue, sLogFileFormat ).asString();
                sLogFilePath = JVgetOptional(
                        param_lib, "log_file_path", Json::stringValue, sLogFilePath ).asString();
                sLogFileType = static_cast<eLogFileType>( JVgetOptional(
                        param_lib, "log_file_type", Json::intValue, (int)sLogFileType ).asInt() );
                sLogFileRotatingSize = JVgetOptional( param_lib, "log_file_rotating_size",
                        Json::intValue, (int)sLogFileRotatingSize ).asInt();
                sLogFileRotatingNum = JVgetOptional( param_lib, "log_file_rotating_num",
                        Json::intValue, (int)sLogFileRotatingNum ).asInt();

                // Frequency detection
                mFrequencyDetectionPeriod = JVgetOptional( param_lib, "frequency_detection_period",
                        Json::uintValue, mFrequencyDetectionPeriod).asUInt();
                mFrequencyDetectionThreshold = JVgetOptional( param_lib, "frequency_detection_threshold",
                        Json::uintValue, mFrequencyDetectionThreshold).asDouble();

                // Others
                mWSRetryPeriodLong = JVgetOptional( param_lib, "ws_retry_period_long",
                        Json::uintValue, mWSRetryPeriodLong).asUInt();
                mWSRetryPeriodShort = JVgetOptional( param_lib, "ws_retry_period_short",
                        Json::uintValue, mWSRetryPeriodShort).asUInt();
                mWSRequestTimeout = JVgetOptional( param_lib, "ws_request_timeout",
                        Json::uintValue, mWSRequestTimeout).asUInt();
                if ( mWSRequestTimeout == 0 )
                    Throw( DRM_BadArg, "ws_request_timeout must not be 0");
            }
            mHealthPeriod = 0;
            mHealthRetryTimeout = mWSRequestTimeout;
            mHealthRetrySleep = mWSRetryPeriodShort;

            // Customize logging configuration
            updateLog();

            if ( mWSRetryPeriodLong <= mWSRetryPeriodShort )
                Throw( DRM_BadArg, "ws_retry_period_long ({}) must be greater than ws_retry_period_short ({})",
                        mWSRetryPeriodLong, mWSRetryPeriodShort );

            // Design configuration
            Json::Value conf_design = JVgetOptional( conf_json, "design", Json::objectValue );
            if ( !conf_design.empty() ) {
                mUDID = JVgetOptional( conf_design, "udid", Json::stringValue, "" ).asString();
                mBoardType = JVgetOptional( conf_design, "boardType", Json::stringValue, "" ).asString();
            }

            // Licensing configuration
            Json::Value conf_licensing = JVgetRequired( conf_json, "licensing", Json::objectValue );
            // Get licensing mode
            bool is_nodelocked = JVgetOptional( conf_licensing, "nodelocked", Json::booleanValue, false ).asBool();
            if ( is_nodelocked ) {
                // If this is a node-locked license, get the license path
                mNodeLockLicenseDirPath = JVgetRequired( conf_licensing, "license_dir", Json::stringValue ).asString();
                mLicenseType = eLicenseType::NODE_LOCKED;
                Debug( "Configuration file specifies a Node-locked license" );
            } else {
                Debug( "Configuration file specifies a floating/metered license" );
                // Get DRM frequency related parameters
                Json::Value conf_drm = JVgetRequired( conf_json, "drm", Json::objectValue );
                mFrequencyInit = JVgetRequired( conf_drm, "frequency_mhz", Json::intValue ).asUInt();
                mFrequencyCurr = mFrequencyInit;
                mBypassFrequencyDetection = JVgetOptional( conf_drm, "bypass_frequency_detection", Json::booleanValue,
                        mBypassFrequencyDetection ).asBool();
            }

        } catch( const Exception &e ) {
            if ( e.getErrCode() != DRM_BadFormat )
                throw;
            Throw( DRM_BadFormat, "Error in configuration file '{}: {}", conf_file_path, e.what() );
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
        }
        catch( const spdlog::spdlog_ex& ex ) {
            std::cout << "Failed to initialize logging: " << ex.what() << std::endl;
        }
    }

    void createFileLog( const std::string file_path, const eLogFileType type,
            const spdlog::level::level_enum level, const std::string format,
            const size_t rotating_size, const size_t rotating_num ) {
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
                Throw( DRM_ExternFail, "Failed to create log file {}", file_path );
            }
            if ( type == eLogFileType::BASIC )
                log_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                        file_path, true);
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
                    sLogFileRotatingSize, sLogFileRotatingNum );
        }
        catch( const spdlog::spdlog_ex& ex ) {
            std::cout << "Failed to update logging settings: " << ex.what() << std::endl;
        }
    }

    void uninitLog() {
        if ( sLogger )
            sLogger->flush();
    }

    uint32_t getMailboxSize() const {
        uint32_t roSize, rwSize;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileSizeRegister( roSize, rwSize ) );
        Debug2( "Full mailbox size: {}", rwSize );
        return rwSize;
    }

    uint32_t getUserMailboxSize() const {
        uint32_t mbSize = getMailboxSize() - (uint32_t)eMailboxOffset::MB_USER;
        auto drmMajor = ( mDrmVersion >> 16 ) & 0xFF;
        if ( (drmMajor <= 3) && (mbSize >= 4) )
            // Used to compensate the bug in the HDK that prevent any access to the highest addresses of the mailbox
            mbSize -= 4;

        Debug( "User Mailbox size: {}", mbSize );
        return mbSize;
    }

    uint32_t readMailbox( const eMailboxOffset offset ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory; max index is {}. ",
                index, rwData.size()-1 ); //LCOV_EXCL_LINE

        Debug( "Read '{}' in Mailbox at index {}", rwData[index], index );
        return rwData[index];
    }

    std::vector<uint32_t> readMailbox( const eMailboxOffset offset, const uint32_t& nb_elements ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( (uint32_t)index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory; max index is {}. ",
                    index, rwData.size()-1 ); //LCOV_EXCL_LINE
        if ( index + nb_elements > rwData.size() )
            Throw( DRM_BadArg, "Trying to read out of Mailbox memory space; size is {}", rwData.size() );

        auto first = rwData.cbegin() + index;
        auto last = rwData.cbegin() + index + nb_elements;
        std::vector<uint32_t> value_vec( first, last );
        Debug( "Read {} elements in Mailbox from index {}", value_vec.size(), index);
        return value_vec;
    }

    void writeMailbox( const eMailboxOffset offset, const uint32_t& value ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lockk( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory: max index is {}. ",
                index, rwData.size()-1 ); //LCOV_EXCL_LINE
        rwData[index] = value;
        checkDRMCtlrRet( getDrmController().writeMailboxFileRegister( rwData, rwSize ) );
        Debug( "Wrote '{}' in Mailbox at index {}", value, index );
    }

    void writeMailbox( const eMailboxOffset offset, const std::vector<uint32_t> &value_vec ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );
        if ( index >= rwData.size() )
            Unreachable( "Index {} overflows the Mailbox memory: max index is {}. ",
                    index, rwData.size()-1 ); //LCOV_EXCL_LINE
        if ( index + value_vec.size() > rwData.size() )
            Throw( DRM_BadArg, "Trying to write out of Mailbox memory space: {}", rwData.size() );
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

    static uint32_t getDrmRegisterOffset( const std::string& regName ) {
        if ( regName == "DrmPageRegister" )
            return 0;
        if ( regName.substr( 0, 15 ) == "DrmRegisterLine" )
            return (uint32_t)std::stoul( regName.substr( 15 ) ) * 4 + 4;
        Unreachable( "Unsupported regName argument: {}. ", regName ); //LCOV_EXCL_LINE
    }

    unsigned int readDrmRegister( const std::string& regName, uint32_t& value ) const {
        int ret = 0;
        ret = f_read_register( getDrmRegisterOffset( regName ), &value );
        if ( ret != 0 ) {
            Error( "Error in read register callback, errcode = {}: failed to read register {}", ret, regName );
            return (uint32_t)(-1);
        }
        Debug2( "Read DRM register {} = 0x{:08x}", regName, value );
        return 0;
    }

    unsigned int writeDrmRegister( const std::string& regName, uint32_t value ) const {
        int ret = 0;
        ret = f_write_register( getDrmRegisterOffset( regName ), value );
        if ( ret ) {
            Error( "Error in write register callback, errcode = {}: failed to write {} to register {}", ret, value, regName );
            return (uint32_t)(-1);
        }
        Debug2( "Write DRM register {} = 0x{:08x}", regName, value );
        return 0;
    }

    unsigned int readDrmAddress( const uint32_t address, uint32_t& value ) const {
        int ret = 0;
        ret = f_read_register( address, &value );
        if ( ret != 0 ) {
            Error( "Error in read register callback, errcode = {}: failed to read address {}", ret, address );
            return (uint32_t)(-1);
        }
        Debug2( "Read DRM address 0x{:x} = 0x{:08x}", address, value );
        return 0;
    }

    unsigned int writeDrmAddress( const uint32_t address, uint32_t value ) const {
        int ret = 0;
        ret = f_write_register( address, value );
        if ( ret ) {
            Error( "Error in write register callback, errcode = {}: failed to write {} to address {}", ret, value, address );
            return (uint32_t)(-1);
        }
        Debug2( "Wrote DRM address 0x{:x} = 0x{:08x}", address, value );
        return 0;
    }

    void lockDrmToInstance() {
        return;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        uint32_t isLocked = readMailbox( eMailboxOffset::MB_LOCK_DRM );
        if ( isLocked )
            Throw( DRM_BadUsage, "Another instance of the DRM Manager is currently owning the HW" );
        writeMailbox( eMailboxOffset::MB_LOCK_DRM, 1 );
        mIsLockedToDrm = true;
        Debug( "DRM Controller is now locked to this object instance" );
    }

    void unlockDrmToInstance() {
        return;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        if ( !mIsLockedToDrm )
            return;
        uint32_t isLocked = readMailbox( eMailboxOffset::MB_LOCK_DRM );
        if ( isLocked ) {
            writeMailbox( eMailboxOffset::MB_LOCK_DRM, 0 );
            Debug( "DRM Controller is now unlocked to this object instance" );
        }
    }

    // Check compatibility of the DRM Version with Algodone version
    void checkHdkCompatibility() {
        std::string drmVersionDot;

        std::string drmVersion = getDrmCtrlVersion();
        mDrmVersion = DrmControllerLibrary::DrmControllerDataConverter::hexStringToBinary( drmVersion )[0];
        drmVersionDot = DrmControllerLibrary::DrmControllerDataConverter::binaryToVersionString( mDrmVersion );

        auto drmMajor = ( mDrmVersion >> 16 ) & 0xFF;
        auto drmMinor = ( mDrmVersion >> 8  ) & 0xFF;

        if ( drmMajor < HDK_COMPATIBILITY_LIMIT_MAJOR ) {
            Throw( DRM_CtlrError,
                    "This DRM Lib {} is not compatible with the DRM HDK version {}: To be compatible HDK version shall be > or equal to {}.{}.x",
                    DRMLIB_VERSION, drmVersionDot, HDK_COMPATIBILITY_LIMIT_MAJOR, HDK_COMPATIBILITY_LIMIT_MINOR );
        } else if ( ( drmMajor == HDK_COMPATIBILITY_LIMIT_MAJOR ) && ( drmMinor < HDK_COMPATIBILITY_LIMIT_MINOR ) ) {
            Throw( DRM_CtlrError,
                    "This DRM Library version {} is not compatible with the DRM HDK version {}: To be compatible HDK version shall be > or equal to {}.{}.x",
                    DRMLIB_VERSION, drmVersionDot, HDK_COMPATIBILITY_LIMIT_MAJOR, HDK_COMPATIBILITY_LIMIT_MINOR );
        }
        Debug( "DRM HDK Version: {}", drmVersionDot );
    }

    /* Run BIST to check Page register access
     * This test write and read DRM Page register to verify the page switch is working
     */
    void runBistLevel1() const {
        unsigned int reg;
        for(unsigned int i=0; i<=5; i++) {
            if ( writeDrmRegister( "DrmPageRegister", i ) != 0 )
                Throw( DRM_BadArg, "DRM Communication Self-Test 1 failed: Could not write DRM page register\n{}", DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
            if ( readDrmRegister( "DrmPageRegister", reg ) != 0 )
                Throw( DRM_BadArg, "DRM Communication Self-Test 1 failed: Could not read DRM page register\n{}", DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
            if ( reg != i ) {
                Throw( DRM_BadArg, "DRM Communication Self-Test 1 failed: Could not switch DRM register page.\n{}", DRM_SELF_TEST_ERROR_MESSAGE ); //LCOV_EXCL_LINE
            }
        }
        Debug( "DRM Communication Self-Test 1 succeeded" );
    }

    /* Run BIST to check register accesses
     * This test write and read mailbox registers to verify the read and write callbacks are working correctly.
     */
    void runBistLevel2() const {
        // Get mailbox size
        uint32_t mbSize = getUserMailboxSize();

        // Check mailbox size
        if ( mbSize >= 0x10000 ) {
            Debug( "DRM Communication Self-Test 2 failed: bad size {}", mbSize );
            Throw( DRM_BadArg, "DRM Communication Self-Test 2 failed: Unexpected mailbox size ({} > 0x10000).\n{}", mbSize, DRM_SELF_TEST_ERROR_MESSAGE); //LCOV_EXCL_LINE
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

    bool isNodeLockedMode() const {
        return mLicenseType == eLicenseType::NODE_LOCKED;
    }

    void initDrmInterface() {

        if ( mDrmController )
            return;

        // create instance
        try {
            mDrmController.reset(
                    new DrmControllerLibrary::DrmControllerOperations(
                            std::bind( &DrmManager::Impl::readDrmRegister,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2 ),
                            std::bind( &DrmManager::Impl::writeDrmRegister,
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
        if ( !isNodeLockedMode() ) {
            determineFrequencyDetectionMethod();
            if ( mIsFreqDetectionMethod1 ) {
                detectDrmFrequencyMethod1();
            }
        }

        // Save header information
        mHeaderJsonRequest = getMeteringHeader();

        // If node-locked license is requested, create license request file
        if ( isNodeLockedMode() ) {

            // Check license directory exists
            if ( !isDir( mNodeLockLicenseDirPath ) )
                Throw( DRM_BadArg,
                        "License directory path '{}' specified in configuration file '{}' is not existing on file system",
                        mNodeLockLicenseDirPath, mConfFilePath );

            // If a floating/metering session is still running, try to close it gracefully.
            if ( isDrmCtrlInMetering() && isSessionRunning() ) {
                Debug( "A floating/metering session is still pending: trying to close it gracefully before switching to nodelocked license." );
                mHeaderJsonRequest["mode"] = (uint8_t)eLicenseType::METERED;
                try {
                    mWsClient.reset( new DrmWSClient( mConfFilePath, mCredFilePath ) );
                    stopSession();
                } catch( const Exception& e ) {
                    Debug( "Failed to stop gracefully the pending session because: {}", e.what() );
                }
                mHeaderJsonRequest["mode"] = (uint8_t)eLicenseType::NODE_LOCKED;
            }

            // Create license request file
            createNodelockedLicenseRequestFile();
        } else {
            mWsClient.reset( new DrmWSClient( mConfFilePath, mCredFilePath ) );
        }
    }

    // Get DRM HDK version
    std::string getDrmCtrlVersion() const {
        std::string drmVersion;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().extractDrmVersion( drmVersion ) );
        return drmVersion;
    }

    void checkSessionIDFromWS( const Json::Value license_json ) const {
        std::string ws_sessionID = license_json["metering"]["sessionId"].asString();
        if ( !mSessionID.empty() && ( mSessionID != ws_sessionID ) ) {
            Warning( "Session ID mismatch: WebService returns '{}' but '{}' is expected", ws_sessionID, mSessionID ); //LCOV_EXCL_LINE
        }
    }

    void checkSessionIDFromDRM( const Json::Value license_json ) const {
        std::string drm_sessionID = license_json["sessionId"].asString();
        if ( !mSessionID.empty() && ( mSessionID != drm_sessionID ) ) {
            Warning( "Session ID mismatch: DRM IP returns '{}' but '{}' is expected", drm_sessionID, mSessionID ); //LCOV_EXCL_LINE
        }
    }

    void getNumActivator( uint32_t& value ) const {
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readNumberOfDetectedIpsStatusRegister( value ) );
    }

    uint64_t getTimerCounterValue() const {
        uint32_t licenseTimerCounterMsb(0), licenseTimerCounterLsb(0);
        uint64_t licenseTimerCounter(0);
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().sampleLicenseTimerCounter( licenseTimerCounterMsb,
                licenseTimerCounterLsb ) );
        licenseTimerCounter = licenseTimerCounterMsb;
        licenseTimerCounter <<= 32;
        licenseTimerCounter |= licenseTimerCounterLsb;
        return licenseTimerCounter;
    }

    std::string getDrmPage( uint32_t page_index ) const {
        uint32_t value;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        writeDrmRegister( "DrmPageRegister", page_index );
        std::string str = fmt::format( "DRM Page {}  registry:\n", page_index );
        for( uint32_t r=0; r < NB_MAX_REGISTER; r++ ) {
            readDrmAddress( r*4, value );
            str += fmt::format( "\tRegister @0x{:02X}: 0x{:08X} ({:d})\n", r*4, value, value );
        }
        return str;
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
        std::string dna;
        std::vector<std::string> vlnvFile;
        std::string mailboxReadOnly;

        // Fulfill application section
        if ( !mUDID.empty() )
            json_output["udid"] = mUDID;
        if ( !mBoardType.empty() )
            json_output["boardType"] = mBoardType;
        json_output["mode"] = (uint8_t)mLicenseType;
        if ( !isNodeLockedMode() )
            json_output["drm_frequency_init"] = mFrequencyInit;

        // Get information from DRM Controller
        getDesignInfo( drmVersion, dna, vlnvFile, mailboxReadOnly );

        // Fulfill with DRM section
        json_output["drmlibVersion"] = DRMLIB_VERSION;
        json_output["lgdnVersion"] = drmVersion;
        json_output["dna"] = dna;
        for ( uint32_t i = 0; i < vlnvFile.size(); i++ ) {
            std::string i_str = std::to_string(i);
            json_output["vlnvFile"][i_str]["vendor"] = std::string("x") + vlnvFile[i].substr(0, 4);
            json_output["vlnvFile"][i_str]["library"] = std::string("x") + vlnvFile[i].substr(4, 4);
            json_output["vlnvFile"][i_str]["name"] = std::string("x") + vlnvFile[i].substr(8, 4);
            json_output["vlnvFile"][i_str]["version"] = std::string("x") + vlnvFile[i].substr(12, 4);
        }

        // Fulfill with Product information
        if ( !mailboxReadOnly.empty() ) {
            try {
                Json::Value product_info = parseJsonString( mailboxReadOnly );
                if ( product_info.isMember( "product_id" ) )
                    json_output["product"] = product_info["product_id"];
                else
                    json_output["product"] = product_info;
                if ( product_info.isMember( "pkg_version" ) )
                    json_output["pkg_version"] = product_info["pkg_version"];
                if ( product_info.isMember( "dna_type" ) )
                    json_output["dna_type"] = product_info["dna_type"];
            } catch( const Exception &e ) {
                if ( e.getErrCode() == DRM_BadFormat )
                    Throw( DRM_BadFormat, "Failed to parse Read-Only Mailbox in DRM Controller: {}", e.what() );
                throw;
            }
        } else {
            Debug( "Could not find product ID information in DRM Controller Mailbox" );
        }

        return json_output;
    }

    Json::Value getMeteringStart() const {
        Json::Value json_request( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build license request #{} to create new session", mLicenseCounter );

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        // Request challenge and metering info for first request
        checkDRMCtlrRet( getDrmController().initialization( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_request["saasChallenge"] = saasChallenge;
        json_request["meteringFile"]  = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        json_request["request"] = "open";
        if ( !isNodeLockedMode() )
            json_request["drm_frequency"] = mFrequencyCurr;
        json_request["mode"] = (uint8_t)mLicenseType;

        return json_request;
    }

    Json::Value getMeteringRunning() const {
        Json::Value json_request( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build license request #{} to maintain current session", mLicenseCounter );
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        // Check if an error occurred
        checkDRMCtlrRet( getDrmController().waitNotTimerInitLoaded( 5 ) );
        // Request challenge and metering info for new request
        checkDRMCtlrRet( getDrmController().synchronousExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_request["saasChallenge"] = saasChallenge;
        json_request["sessionId"] = meteringFile[0].substr( 0, 16 );
        checkSessionIDFromDRM( json_request );

        if ( !isNodeLockedMode() )
            json_request["drm_frequency"] = mFrequencyCurr;
        json_request["meteringFile"] = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        json_request["request"] = "running";
        return json_request;
    }

    Json::Value getMeteringStop() const {
        Json::Value json_request( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug( "Build license request #{} to stop current session", mLicenseCounter );
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        // Request challenge and metering info for first request
        checkDRMCtlrRet( getDrmController().endSessionAndExtractMeteringFile(
                numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_request["saasChallenge"] = saasChallenge;
        json_request["sessionId"] = meteringFile[0].substr( 0, 16 );
        checkSessionIDFromDRM( json_request );

        if ( !isNodeLockedMode() )
            json_request["drm_frequency"] = mFrequencyCurr;
        json_request["meteringFile"]  = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        json_request["request"] = "close";
        return json_request;
    }

    Json::Value getMeteringHealth() const {
        Json::Value json_request( mHeaderJsonRequest );
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;
        {
            Debug( "Waiting metering access mutex from getMeteringHealth" );
            std::lock_guard<std::mutex> lockMetering( mMeteringAccessMutex );
            Debug( "Acquired metering access mutex from getMeteringHealth" );

            Debug( "Build health request #{}", mHealthCounter );
            {
                std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
                if ( isNodeLockedMode() || isLicenseActive() ) {
                    checkDRMCtlrRet( getDrmController().asynchronousExtractMeteringFile(
                            numberOfDetectedIps, saasChallenge, meteringFile ) );
                } else {
                    warning( "Cannot access metering data when no license is active" );
                }
            }
        }
        Debug( "Released metering access mutex from getMeteringHealth" );
        json_request["saasChallenge"] = saasChallenge;
        if ( meteringFile.size() ) {
            json_request["sessionId"] = meteringFile[0].substr( 0, 16 );
        } else {
            json_request["sessionId"] = "";
        }
        // Finalize the request with the collected data
        json_request["meteringFile"]  = std::accumulate( meteringFile.begin(), meteringFile.end(), std::string("") );
        json_request["drm_frequency"] = mFrequencyCurr;
        json_request["request"] = "health";
        json_request["health_id"] = mHealthCounter++;
        return json_request;
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
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( readOnlyMailboxSize, readWriteMailboxSize,
                                                                     readOnlyMailboxData, readWriteMailboxData ) );
        Debug( "Mailbox sizes: read-only={}, read-write={}", readOnlyMailboxSize, readWriteMailboxSize );
        readOnlyMailboxData.push_back( 0 );
        if ( readOnlyMailboxSize ) {
            mailboxReadOnly = std::string( (char*)readOnlyMailboxData.data() );
        }
        else
            mailboxReadOnly = std::string("");
    }

    bool isSessionRunning()const  {
        bool sessionRunning( false );
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readSessionRunningStatusRegister( sessionRunning ) );
        Debug( "DRM session running state: {}", sessionRunning );
        return sessionRunning;
    }

    bool isDrmCtrlInNodelock()const  {
        bool isNodelocked( false );
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readLicenseNodeLockStatusRegister( isNodelocked ) );
        Debug( "DRM Controller node-locked status: {}", isNodelocked );
        return isNodelocked;
    }

    bool isDrmCtrlInMetering()const  {
        bool isMetering( false );
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readLicenseMeteringStatusRegister( isMetering ) );
        Debug( "DRM Controller metering status: {}", isMetering );
        return isMetering;
    }

    bool isReadyForNewLicense() const {
        uint32_t numberOfLicenseTimerLoaded;
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readNumberOfLicenseTimerLoadedStatusRegister( numberOfLicenseTimerLoaded ) );
        bool readiness = ( numberOfLicenseTimerLoaded < 2 );
        Debug( "DRM readiness to receive a new license: {} (# loaded licenses = {})", readiness, numberOfLicenseTimerLoaded );
        return readiness;
    }

    bool isLicenseActive() const {
        bool isLicenseEmpty( false );
        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readLicenseTimerCountEmptyStatusRegister( isLicenseEmpty ) );
        return !isLicenseEmpty;
    }

    Json::Value getLicense( const Json::Value& request_json, const uint32_t& timeout,
            const uint32_t& short_retry_period = 0, const uint32_t& long_retry_period = 0 ) {
        TClock::time_point deadline = TClock::now() + std::chrono::seconds( timeout );
        return getLicense( request_json, deadline, short_retry_period, long_retry_period );
    }

    Json::Value getLicense( const Json::Value& request_json, const TClock::time_point& deadline,
            const uint32_t& short_retry_period = 0, const uint32_t& long_retry_period = 0 ) {

        TClock::duration long_duration = std::chrono::seconds( long_retry_period );
        TClock::duration short_duration = std::chrono::seconds( short_retry_period );

        TClock::duration wait_duration;
        bool token_valid(false);
        uint32_t oauth_attempt = 0;
        uint32_t lic_attempt = 0;

        while ( 1 ) {
            token_valid = false;
            // Get valid OAUth2 token
            try {
                getDrmWSClient().requestOAuth2token( deadline );
                token_valid = true;
            } catch ( const Exception& e ) {
                lic_attempt = 0;
                if ( e.getErrCode() == DRM_WSTimedOut ) {
                    // Reached timeout
                    Throw( DRM_WSError, "Timeout on Authentication request after {} attempts", oauth_attempt );
                }
                if ( e.getErrCode() != DRM_WSMayRetry ) {
                    throw;
                }
                // It is retryable
                oauth_attempt ++;
                if ( short_retry_period == 0 ) {
                    // No retry
                    throw;
                }
                if ( long_retry_period == 0 ) {
                     wait_duration = short_duration;
                } else {
                    if ( ( deadline - TClock::now() ) <= long_duration )
                        wait_duration = short_duration;
                    else
                        wait_duration = long_duration;
                }
                Warning( "Attempt #{} to obtain a new OAuth2 token failed with message: {}. New attempt planned in {} seconds",
                        oauth_attempt, e.what(), wait_duration.count()/1000000000 );
                // Wait a bit before retrying
                sleepOrExit( wait_duration );
            }
            if ( !token_valid ) continue;

            // Get new license
            try {
                return getDrmWSClient().requestLicense( request_json, deadline );
            } catch ( const Exception& e ) {
                oauth_attempt = 0;
                if ( e.getErrCode() == DRM_WSTimedOut ) {
                    // Reached timeout
                    Throw( DRM_WSError, "Timeout on License request after {} attempts", lic_attempt );
                }
                if ( e.getErrCode() != DRM_WSMayRetry ) {
                    throw;
                }
                // It is retryable
                lic_attempt ++;
                if ( short_retry_period == 0 ) {
                    // No retry
                    throw;
                }
                // Evaluate the next retry
                if ( long_retry_period == 0 ) {
                     wait_duration = short_duration;
                } else {
                    if ( ( deadline - TClock::now() ) <= long_duration )
                        wait_duration = short_duration;
                    else
                        wait_duration = long_duration;
                }
                Warning( "Attempt #{} to obtain a new License failed with message: {}. New attempt planned in {} seconds",
                        lic_attempt, e.what(), wait_duration.count()/1000000000 );
                // Wait a bit before retrying
                sleepOrExit( wait_duration );
            }
        }
    }

    void setLicense( const Json::Value& license_json ) {

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        Debug( "Provisioning license #{} on DRM controller", mLicenseCounter );

        std::string dna = mHeaderJsonRequest["dna"].asString();
        std::string licenseKey, licenseTimer;

        try {
            Json::Value metering_node = JVgetRequired( license_json, "metering", Json::objectValue );
            Json::Value license_node = JVgetRequired( license_json, "license", Json::objectValue );
            Json::Value dna_node = JVgetRequired( license_node, dna.c_str(), Json::objectValue );

            /// Get session ID received from web service
            if ( mSessionID.empty() ) {
                /// Save new Session ID
                mSessionID = JVgetRequired( metering_node, "sessionId", Json::stringValue ).asString();
                Debug( "Saving session ID: {}", mSessionID );
            } else {
                /// Verify Session ID
                checkSessionIDFromWS( license_json );
            }

            /// Extract license key and license timer from web service response
            if ( mLicenseCounter == 0 )
                licenseKey = JVgetRequired( dna_node, "key", Json::stringValue ).asString();
            if ( !isNodeLockedMode() )
                licenseTimer = JVgetRequired( dna_node, "licenseTimer", Json::stringValue ).asString();
            mLicenseDuration = JVgetRequired( metering_node, "timeoutSecond", Json::uintValue ).asUInt();
            if ( mLicenseDuration == 0 )
                Warning( "'timeoutSecond' field sent by License WS must not be 0" );

        } catch( const Exception &e ) {
            if ( e.getErrCode() != DRM_BadFormat )
                throw;
            Throw( DRM_WSRespError, "Malformed response from License Web Service: {}", e.what() );
        }

        if ( mLicenseCounter == 0 ) {
            // Load key
            bool activationDone( false );
            uint8_t activationErrorCode;
            checkDRMCtlrRet( getDrmController().activate( licenseKey, activationDone, activationErrorCode ) );
            if ( activationErrorCode ) {
                Throw( DRM_CtlrError, "Failed to activate license on DRM controller, activationErr: 0x{:x}",
                      activationErrorCode );
            }
            if ( !activationDone )
                Throw( DRM_CtlrError, "Failed to activate license on DRM controller, activationDone: {}",
                      activationDone );
            Debug( "Wrote license key of session ID: {}", mSessionID );
        }

        // Load license timer
        if ( !isNodeLockedMode() ) {
            bool licenseTimerEnabled( false );
            checkDRMCtlrRet(
                    getDrmController().loadLicenseTimerInit( licenseTimer, licenseTimerEnabled ) );
            if ( !licenseTimerEnabled ) {
                Throw( DRM_CtlrError,
                      "Failed to load license timer on DRM controller, licenseTimerEnabled: 0x{:x}",
                      licenseTimerEnabled );
            }
            Debug( "Wrote license timer #{} of session ID {} for a duration of {} seconds",
                    mLicenseCounter, mSessionID, mLicenseDuration );

            bool activationCodesTransmitted( false );
            TClock::duration timeSpan;
            double mseconds( 0.0 );
            TClock::time_point timeStart = TClock::now();

            while( mseconds < ACTIVATIONCODE_TRANSMISSION_TIMEOUT_MS ) {
                checkDRMCtlrRet(
                        getDrmController().readActivationCodesTransmittedStatusRegister( activationCodesTransmitted ) );
                timeSpan = TClock::now() - timeStart;
                mseconds = 1000.0 * double( timeSpan.count() ) * TClock::period::num / TClock::period::den;
                if ( activationCodesTransmitted ) {
                    Debug( "License #{} transmitted after {} ms", mLicenseCounter, mseconds );
                    break;
                }
                Debug( "License #{} not transmitted yet after {} ms", mLicenseCounter, mseconds );
            }
            if ( !activationCodesTransmitted ) {
                Unreachable( "DRM Controller could not transmit Licence #{} to activators", mLicenseCounter ); //LCOV_EXCL_LINE
            }
        }

        // Check DRM Controller has switched to the right license mode
        bool is_nodelocked = isDrmCtrlInNodelock();
        bool is_metered = isDrmCtrlInMetering();
        if ( is_nodelocked && is_metered )
            Unreachable( "DRM Controller cannot be in both Node-Locked and Metering/Floating license modes. " ); //LCOV_EXCL_LINE
        if ( !isNodeLockedMode() ) {
            if ( !is_metered )
                Throw( DRM_CtlrError, "DRM Controller failed to switch to Metering license mode" );
            else
                Debug( "DRM Controller is in Metering license mode" );
        } else {
            if ( !is_nodelocked )
                Throw( DRM_CtlrError, "DRM Controller failed to switch to Node-Locked license mode" );
            else
                Debug( "DRM Controller is in Node-Locked license mode" );
        }
        Debug( "Provisioned license #{} on DRM controller", mLicenseCounter );
        mLicenseCounter ++;
    }

    Json::Value postHealth( const Json::Value& request_json, const TClock::time_point& deadline,
            const uint32_t& retry_period = 0 ) {

        TClock::duration retry_duration = std::chrono::seconds( retry_period );
        bool token_valid(false);
        uint32_t oauth_attempt = 0;
        uint32_t lic_attempt = 0;

        while ( 1 ) {
            token_valid = false;
            // Get valid OAUth2 token
            try {
                getDrmWSClient().requestOAuth2token( deadline );
                token_valid = true;
            } catch ( const Exception& e ) {
                lic_attempt = 0;
                if ( e.getErrCode() == DRM_WSTimedOut ) {
                    // Reached timeout
                    Warning( "Timeout on Authentication request after {} attempts", oauth_attempt );
                    return Json::nullValue;
                }
                if ( e.getErrCode() != DRM_WSMayRetry ) {
                    Error( "Health request error: {}", e.what() );
                    return Json::nullValue;
                }
                // It is retryable
                oauth_attempt ++;
                if ( retry_period == 0 ) {
                    // No retry
                    return Json::nullValue;
                }
                Warning( "Attempt #{} to obtain a new OAuth2 token failed with message: {}. New attempt planned in {} seconds",
                        oauth_attempt, e.what(), retry_duration.count()/1000000000 );
                // Wait a bit before retrying
                sleepOrExit( retry_duration );
            }
            if ( !token_valid ) continue;

            // Get new license
            try {
                return getDrmWSClient().requestHealth( request_json, deadline );
            } catch ( const Exception& e ) {
                oauth_attempt = 0;
                if ( e.getErrCode() == DRM_WSTimedOut ) {
                    // Reached timeout
                    Warning( "Timeout on Health request after {} attempts", lic_attempt );
                    return Json::nullValue;
                }
                if ( e.getErrCode() != DRM_WSMayRetry ) {
                    Error( "Health request error: {}", e.what() );
                    return Json::nullValue;
                }
                // It is retryable
                lic_attempt ++;
                if ( retry_period == 0 ) {
                    // No retry
                    return Json::nullValue;
                }
                // Perform retry
                Warning( "Attempt #{} to send a new Health request failed with message: {}. New attempt planned in {} seconds",
                        lic_attempt, e.what(), retry_duration.count()/1000000000 );
                // Wait a bit before retrying
                sleepOrExit( retry_duration );
            }
        }
    }

    Json::Value performHealth( const uint32_t retry_timeout, const uint32_t retry_sleep) {
        // Get next data from DRM Controller
        Json::Value request_json = getMeteringHealth();
        // Check session ID
        checkSessionIDFromDRM( request_json );
        if ( request_json["meteringFile"].empty() )
            Unreachable( "Received an empty metering file from DRM Controller" );  //LCOV_EXCL_LINE
        // Compute retry period
        TClock::time_point retry_deadline = TClock::now() + std::chrono::seconds( retry_timeout );
        // Post next data to server
        return postHealth( request_json, retry_deadline, retry_sleep );
    }

    std::string getDesignHash() {
        std::string drmVersion, dna, mailboxReadOnly;
        std::vector<std::string> vlnvFile;
        std::hash<std::string> hasher;

        getDesignInfo( drmVersion, dna, vlnvFile, mailboxReadOnly );
        std::string design = dna + drmVersion;
        for( const std::string& vlnv: vlnvFile )
            design += vlnv;
        std::string hash = fmt::format( "{:016X}", hasher( design ) );
        Debug( "Hash for HW design is {}", hash );
        return hash;
    }

    void createNodelockedLicenseRequestFile() {
        // Create hash name based on design info
        std::string designHash = getDesignHash();
        mNodeLockRequestFilePath = mNodeLockLicenseDirPath + path_sep + designHash + ".req";
        mNodeLockLicenseFilePath = mNodeLockLicenseDirPath + path_sep + designHash + ".lic";
        Debug( "Created hash name based on design info: {}", designHash );
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
        Debug( "License request file saved on: {}", mNodeLockRequestFilePath );
    }

    void installNodelockedLicense() {
        Json::Value license_json;
            std::ifstream ifs;

        Debug ( "Looking for local node-locked license file: {}", mNodeLockLicenseFilePath );

        // Check if license file exists
        if ( isFile( mNodeLockLicenseFilePath ) ) {
            try {
                // Try to load the local license file
                license_json = parseJsonFile( mNodeLockLicenseFilePath );
                Debug( "Parsed Node-locked License file: {}", license_json .toStyledString() );
            } catch( const Exception& e ) {
                Throw( e.getErrCode(), "Invalid local license file {} because {}. "
                     "If this machine is connected to the License server network, rename the file and retry. "
                     "Otherwise request a new Node-Locked license from your supplier.",
                     mNodeLockLicenseFilePath, e.what() );
            }
        } else {
            /// No license has been found locally, request one to License WS:
            /// - Clear Session IS
            Debug( "Clearing session ID: {}", mSessionID );
            mSessionID = std::string("");
            /// - Create WS access
            mWsClient.reset( new DrmWSClient( mConfFilePath, mCredFilePath ) );
            /// - Read request file
            try {
                Json::Value request_json = parseJsonFile( mNodeLockRequestFilePath );
                Debug( "Parsed Node-locked License Request file: {}", request_json .toStyledString() );
                /// - Send request to web service and receive the new license
                TClock::time_point deadline =
                        TClock::now() + std::chrono::seconds( mWSRequestTimeout );
                license_json = getLicense( request_json, deadline, mWSRetryPeriodShort );
                /// - Save the license to file
                saveJsonToFile( mNodeLockLicenseFilePath, license_json );
                Debug( "Requested and saved new node-locked license file: {}", mNodeLockLicenseFilePath );
            } catch( const Exception& e ) {
                Throw( e.getErrCode(), "Failed to request license file: {}", e.what() );
            }
        }
        /// Install the license
        setLicense( license_json );
        Info( "Installed node-locked license successfully" );
    }

    void determineFrequencyDetectionMethod() {
        uint32_t reg;

        if ( mBypassFrequencyDetection ) {
            Debug( "Frequency detection sequence is bypassed." );
            return;
        }

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );
        int ret = readDrmAddress( REG_FREQ_DETECTION_VERSION, reg );
        if ( ret != 0 ) {
            Unreachable( "Failed to read DRM frequency detection version register, errcode = {}. ", ret ); //LCOV_EXCL_LINE
        }
        if ( reg == FREQ_DETECTION_VERSION_EXPECTED ) {
            // Use Method 1
            Debug( "Use dedicated counter to compute DRM frequency (method 1)" );
            mIsFreqDetectionMethod1 = true;
        } else {
            // Use Method 2
            Debug( "Use license timer counter to compute DRM frequency (method 2)" );
            mIsFreqDetectionMethod1 = false;
        }
    }

    void detectDrmFrequencyMethod1() {
        int ret;
        uint32_t counter;
        TClock::duration wait_duration = std::chrono::milliseconds( mFrequencyDetectionPeriod );

        if ( mBypassFrequencyDetection ) {
            return;
        }

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        // Start detection counter
        ret = writeDrmAddress( REG_FREQ_DETECTION_COUNTER, 0 );
        if ( ret != 0 )
            Unreachable( "Failed to start DRM frequency detection counter, errcode = {}. ", ret ); //LCOV_EXCL_LINE

        // Wait a fixed period of time
        sleepOrExit( wait_duration );

        // Sample counter
        ret = readDrmAddress( REG_FREQ_DETECTION_COUNTER, counter );
        if ( ret != 0 ) {
            Unreachable( "Failed to read DRM frequency detection counter register, errcode = {}. ", ret ); //LCOV_EXCL_LINE
        }

        if ( counter == 0xFFFFFFFF )
            Throw( DRM_BadFrequency, "Frequency auto-detection failed: frequency_detection_period parameter ({} ms) is too long.",
                   mFrequencyDetectionPeriod );

        // Compute estimated DRM frequency
        //mFrequencyCurr = (int32_t)(std::ceil((double)counter / mFrequencyDetectionPeriod / 1000));
        int32_t measured_frequency = (int32_t)((double)counter / mFrequencyDetectionPeriod / 1000);
        Debug( "Frequency detection counter after {:f} ms is 0x{:08x}  => estimated frequency = {} MHz",
            (double)mFrequencyDetectionPeriod/1000, counter, measured_frequency );

        checkDrmFrequency( measured_frequency );
    }

    void detectDrmFrequencyMethod2() {
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

    int32_t detectDrmFrequencyFromLicenseTimer() {
        TClock::time_point timeStart, timeEnd;
        uint64_t counterStart, counterEnd;
        TClock::duration wait_duration = std::chrono::milliseconds( mFrequencyDetectionPeriod );
        int max_attempts = 3;

        std::lock_guard<std::recursive_mutex> lock( mDrmControllerMutex );

        Debug( "Detecting DRM frequency in {} ms", mFrequencyDetectionPeriod );

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
        mFrequencyCurr = measuredFrequency;
        if ( precisionError >= mFrequencyDetectionThreshold ) {
            Throw( DRM_BadFrequency,
                   "Estimated DRM frequency ({} MHz) differs from the value ({} MHz) defined in the configuration file '{}' by more than {}%: From now on the estimated frequency will be used.",
                    mFrequencyCurr, mFrequencyInit, mConfFilePath, mFrequencyDetectionThreshold, mFrequencyCurr);
        }
        Debug( "Estimated DRM frequency = {} MHz, config frequency = {} MHz: gap = {}%",
                mFrequencyCurr, mFrequencyInit, precisionError );
    }

    template< class Clock, class Duration >
    void sleepOrExit( const std::chrono::time_point<Clock, Duration> &timeout_time ) {
        std::unique_lock<std::mutex> lock( mThreadExitMtx );
        bool isExitRequested = mThreadExitCondVar.wait_until( lock, timeout_time,
                [ this ]{ return mThreadExit; } );
        if ( isExitRequested )
            Throw( DRM_Exit, "Exit requested" );
    }

    template< class Rep, class Period >
    void sleepOrExit( const std::chrono::duration<Rep, Period> &rel_time ) {
        std::unique_lock<std::mutex> lock( mThreadExitMtx );
        bool isExitRequested = mThreadExitCondVar.wait_for( lock, rel_time,
                [ this ]{ return mThreadExit; } );
        if ( isExitRequested )
            Throw( DRM_Exit, "Exit requested" );
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

        if ( mThreadKeepAlive.valid() ) {
            Warning( "Licensing thread already started" );
            return;
        }

        mThreadKeepAlive = std::async( std::launch::async, [ this ]() {
            Debug( "Starting background thread which maintains licensing" );
            try {

                /// Detecting DRM controller frequency if needed
                if ( !mIsFreqDetectionMethod1 )
                    detectDrmFrequencyMethod2();

                bool go_sleeping( false );

                /// Starting license request loop
                while( 1 ) {
                    if ( isStopRequested() )
                        break;
                    {
                        Debug( "Waiting metering access mutex from licensing thread" );
                        std::lock_guard<std::mutex> lockMetering( mMeteringAccessMutex );
                        Debug( "Acquired metering access mutex from licensing thread" );

                        // Check DRM licensing queue
                        if ( !isReadyForNewLicense() ) {
                            go_sleeping = true;

                        } else {
                            go_sleeping = false;
                            Debug( "Requesting new license #{} now", mLicenseCounter );
                            Json::Value request_json = getMeteringRunning();

                            /// Retry Web Service request loop
                            TClock::time_point polling_deadline = TClock::now() + std::chrono::seconds( mLicenseDuration );

                            /// Attempt to get the next license
                            Json::Value license_json = getLicense( request_json, polling_deadline, mWSRetryPeriodShort, mWSRetryPeriodLong );

                            /// New license has been received: now send it to the DRM Controller
                            setLicense( license_json );
                        }
                    }
                    Debug( "Released metering access mutex from licensing thread" );
                    if ( go_sleeping ) {
                        // DRM licensing queue is full, wait until current license expires
                        uint32_t licenseTimeLeft = getCurrentLicenseTimeLeft();
                        TClock::duration wait_duration = std::chrono::seconds( licenseTimeLeft + 1 );
                        Debug( "License thread sleeping {} seconds before checking DRM Controller readiness", licenseTimeLeft );
                        sleepOrExit( wait_duration );
                    }
                }
                Debug( "Released metering access mutex from licensing thread" );

            } catch( const Exception& e ) {
                if ( e.getErrCode() != DRM_Exit ) {
                    Error( e.what() );
                    f_asynch_error( std::string( e.what() ) );
                }
            } catch( const std::exception &e ) {
                std::string errmsg = fmt::format( "[errCode={}] Unexpected error: {}", DRM_ExternFail, e.what() );
                Error( errmsg );
                f_asynch_error( errmsg );
            }
            Debug( "Exiting background thread which maintains licensing" );
        });
    }

    void startHealthContinuityThread() {

        if ( mThreadHealth.valid() ) {
            Warning( "Asynchronous metering thread already started" );
            return;
        }

        mThreadHealth = std::async( std::launch::async, [ this ]() {
            Debug( "Starting background thread which checks health" );
            try {
                uint32_t retry_sleep = mWSRetryPeriodShort;
                uint32_t retry_timeout = mWSRequestTimeout;
                mHealthCounter = 0;
                /// Starting async metering post loop
                while( 1 ) {

                    /// Sleep until it's time to collect the next metering data
                    TClock::time_point wakeup_time = TClock::now() + std::chrono::seconds( mHealthPeriod );
                    Debug( "Health thread sleeping {} seconds before gathering new metering", mHealthPeriod );
                    sleepOrExit( wakeup_time );

                    /// Collect the next metering data and send them to the Health Web Service
                    Debug( "Health thread collecting new metering data" );
                    Json::Value response_json = performHealth( retry_timeout, retry_sleep );

                    if ( response_json != Json::nullValue ) {
                        /// Extract asynchronous metering parameters from response
                        Json::Value metering_node = JVgetOptional( response_json, "metering", Json::objectValue, Json::nullValue );
                        uint32_t healthPeriod = JVgetOptional( metering_node, "healthPeriod", Json::uintValue, mHealthPeriod ).asUInt();
                        uint32_t healthRetryTimeout = JVgetOptional( metering_node, "healthRetry", Json::uintValue, mHealthRetryTimeout ).asUInt();
                        uint32_t healthRetrySleep = JVgetOptional( metering_node, "healthRetrySleep", Json::uintValue, mHealthRetrySleep ).asUInt();

                        /// Reajust async metering thread if needed
                        if ( ( healthPeriod != mHealthPeriod ) || ( healthRetryTimeout != mHealthRetryTimeout)
                                || ( healthRetrySleep != mHealthRetrySleep) ) {
                            mHealthPeriod = healthPeriod;
                            mHealthRetryTimeout = healthRetryTimeout;
                            mHealthRetrySleep = healthRetrySleep;
                            Debug( "Updating Health parameters with new values: healthPeriod={}, healthRetry={}, healthRetrySleep={}",
                                mHealthPeriod, mHealthRetryTimeout, mHealthRetrySleep );
                            if ( mHealthPeriod == 0 ) {
                                Warning( "Health thread is disabled" );
                                break;
                            }
                            if ( mHealthRetryTimeout == 0 ) {
                                retry_timeout = mWSRequestTimeout;
                                retry_sleep = 0;
                                Debug( "Health retry is disabled" );
                            } else {
                                retry_timeout = mHealthRetryTimeout;
                                retry_sleep = mHealthRetrySleep;
                                Debug( "Health retry is enabled" );
                            }
                        } else {
                            Debug( "Keep same Health parameters: healthPeriod={}, healthRetry={}, healthRetrySleep={}",
                                mHealthPeriod, mHealthRetryTimeout, mHealthRetrySleep );
                        }
                    } else {
                        Debug( "Keep same Health parameters: healthPeriod={}, healthRetry={}, healthRetrySleep={}",
                            mHealthPeriod, mHealthRetryTimeout, mHealthRetrySleep );
                    }
                }
            } catch( const Exception& e ) {
                if ( e.getErrCode() != DRM_Exit ) {
                    Error( e.what() );
                    f_asynch_error( std::string( e.what() ) );
                }
            } catch( const std::exception &e ) {
                Error( e.what() );
                f_asynch_error( std::string( e.what() ) );
            }
            Debug( "Exiting background thread which checks health" );
        });
    }

    void stopThread() {
        if ( ( mThreadKeepAlive.valid() == 0 ) && ( mThreadHealth.valid() == 0 ) ) {
            Debug( "Background threads are not running" );
            return;
        }
        {
            std::lock_guard<std::mutex> lock( mThreadExitMtx );
            Debug( "Set Stop flag for threads" );
            mThreadExit = true;
        }
        mThreadExitCondVar.notify_all();
        if ( mThreadKeepAlive.valid() )
            mThreadKeepAlive.get();     // Wait until the License thread ends
        if ( mThreadHealth.valid() )
            mThreadHealth.get();     // Wait until the Health thread ends
        Debug( "Background threads stopped" );
        {
            std::lock_guard<std::mutex> lock( mThreadExitMtx );
            Debug( "Clear Stop flag for threads" );
            mThreadExit = false;
        }
    }

    void startSession() {
        {
            Debug( "Waiting metering access mutex from startSession" );
            std::lock_guard<std::mutex> lockMetering( mMeteringAccessMutex );
            Debug( "Acquired metering access mutex from startSession" );

            if ( !isReadyForNewLicense() )
                Unreachable( "To start a new session the DRM Controller shall be ready to accept a new license" ); //LCOV_EXCL_LINE

            mLicenseCounter = 0;

            // Build start request message for new license
            Json::Value request_json = getMeteringStart();

            // Send request and receive new license
            Json::Value license_json = getLicense( request_json, mWSRequestTimeout, mWSRetryPeriodShort );
            setLicense( license_json );

            /// Extract asynchronous health parameters from response
            Json::Value metering_node = JVgetOptional( license_json, "metering", Json::objectValue, Json::nullValue );
            mHealthPeriod = JVgetOptional( metering_node, "healthPeriod", Json::uintValue, mHealthPeriod ).asUInt();
            mHealthRetryTimeout = JVgetOptional( metering_node, "healthRetry", Json::uintValue, mHealthRetryTimeout ).asUInt();
            mHealthRetrySleep = JVgetOptional( metering_node, "healthRetrySleep", Json::uintValue, mHealthRetrySleep ).asUInt();

            // Check if an error occurred
            checkDRMCtlrRet( getDrmController().waitNotTimerInitLoaded( 5 ) );
        }
        Debug( "Released metering access mutex from startSession" );
        Info( "New DRM session started." );
    }

    void resumeSession() {
        {
            Debug( "Waiting metering access mutex from resumeSession" );
            std::lock_guard<std::mutex> lockMetering( mMeteringAccessMutex );
            Debug( "Acquired metering access mutex from resumeSession" );

            if ( isReadyForNewLicense() ) {
                // Create JSON license request
                Json::Value request_json = getMeteringRunning();

                // Send license request to web service
                Json::Value license_json = getLicense( request_json, mWSRequestTimeout, mWSRetryPeriodShort );

                // Provision license on DRM controller
                setLicense( license_json );
            }
        }
        Debug( "Released metering access mutex from resumeSession" );
        Info( "DRM session resumed." );
    }

    void stopSession() {
        Json::Value request_json;

        // Stop background thread
        stopThread();

        {
            // Get and send metering data to web service
            Debug( "Waiting metering access mutex from stopSession" );
            std::lock_guard<std::mutex> lockMetering( mMeteringAccessMutex );
            Debug( "Acquired metering access mutex from stopSession" );
            request_json = getMeteringStop();

            // Send last metering information
            Json::Value license_json = getLicense( request_json, mWSRequestTimeout, mWSRetryPeriodShort );
            checkSessionIDFromWS( license_json );
            Debug( "Session ID {} stopped and last metering data uploaded", mSessionID );
        }
        Debug( "Released metering access mutex from stopSession" );

        /// Clear Session IS
        Debug( "Clearing session ID: {}", mSessionID );
        mSessionID = std::string("");

        Info( "DRM session stopped." );
        mSecurityStop = false;
    }

    void pauseSession() {
        stopThread();
        mSecurityStop = false;
        performHealth(mWSRequestTimeout, 0);
        Info( "DRM session paused." );
    }

    ParameterKey findParameterKey( const std::string& key_string ) const {
        for ( auto const& it : mParameterKeyMap ) {
            if ( key_string == it.second ) {
                return it.first;
            }
        }
        Throw( DRM_BadArg, "Cannot find parameter: {}", key_string );
    }

    std::string findParameterString( const ParameterKey key_id ) const {
        std::map<ParameterKey, std::string>::const_iterator it;
        it = mParameterKeyMap.find( key_id );
        if ( it == mParameterKeyMap.end() )
            Throw( DRM_BadArg, "Cannot find parameter with ID: {}", key_id );
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
    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path,
          ReadRegisterCallback f_user_read_register,
          WriteRegisterCallback f_user_write_register,
          AsynchErrorCallback f_user_asynch_error )
        : Impl( conf_file_path, cred_file_path )
    {
        Debug( "Entering Impl public constructor" );
        if ( !f_user_read_register )
            Throw( DRM_BadArg, "Read register callback function must not be NULL" );
        if ( !f_user_write_register )
            Throw( DRM_BadArg, "Write register callback function must not be NULL" );
        if ( !f_user_asynch_error )
            Throw( DRM_BadArg, "Asynchronous error callback function must not be NULL" );
        f_read_register = f_user_read_register;
        f_write_register = f_user_write_register;
        f_asynch_error = f_user_asynch_error;
        initDrmInterface();
        Debug( "Exiting Impl public constructor" );
    }

    ~Impl() {
        Debug( "Entering Impl destructor" );
        if ( mSecurityStop && isSessionRunning() ) {
            Debug( "Security stop triggered: stopping current session" );
            stopSession();
        } else {
            stopThread();
        }
        unlockDrmToInstance();
        uninitLog();
        Debug( "Exiting Impl destructor" );
    }

    // Non copyable non movable as we create closure with "this"
    Impl( const Impl& ) = delete;
    Impl( Impl&& ) = delete;

    void activate( const bool& resume_session_request = false ) {
        TRY
            Debug( "Calling 'activate' with 'resume_session_request'={}", resume_session_request );

            bool isRunning = isSessionRunning();

            if ( isNodeLockedMode() ) {
                // Install the node-locked license
                installNodelockedLicense();
                return;
            }
            if ( isDrmCtrlInNodelock() ) {
                Throw( DRM_BadUsage, "DRM Controller is locked in Node-Locked licensing mode: "
                                    "To use other modes you must reprogram the FPGA device." );
            }
            if ( isRunning && resume_session_request ) {
                resumeSession();
            } else {
                if ( isRunning && !resume_session_request ) {
                    Debug( "Session is already running but resume flag is {}: stopping this pending session",
                            resume_session_request );
                    try {
                        stopSession();
                    } catch( const Exception &e ) {
                        Debug( "Failed to stop pending session: {}", e.what() );
                    }
                }
                startSession();
            }
            mThreadExit = false;
            startLicenseContinuityThread();
            if ( mHealthPeriod )
                startHealthContinuityThread();
            mSecurityStop = true;
        CATCH_AND_THROW
    }

    void deactivate( const bool& pause_session_request = false ) {
        TRY
            Debug( "Calling 'deactivate' with 'pause_session_request'={}", pause_session_request );

            if ( isNodeLockedMode() ) {
                return;
            }
            if ( !isSessionRunning() ) {
                Debug( "No session is currently running" );
                return;
            }
            if ( pause_session_request )
                pauseSession();
            else
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
                        int32_t logVerbosity = static_cast<int>( sLogConsoleVerbosity );
                        json_value[key_str] = logVerbosity;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                logVerbosity );
                        break;
                    }
                    case ParameterKey::log_format: {
                        json_value[key_str] = sLogConsoleFormat;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                sLogConsoleFormat );
                        break;
                    }
                    case ParameterKey::log_file_verbosity: {
                        int32_t logVerbosity = static_cast<int>( sLogFileVerbosity );
                        json_value[key_str] = logVerbosity;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                logVerbosity );
                        break;
                    }
                    case ParameterKey::log_file_format: {
                        json_value[key_str] = sLogFileFormat;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                sLogFileFormat );
                        break;
                    }
                    case ParameterKey::log_file_path: {
                        json_value[key_str] = sLogFilePath;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                sLogFilePath );
                        break;
                    }
                    case ParameterKey::log_file_type: {
                        json_value[key_str] = (uint32_t)sLogFileType;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               sLogFileType );
                        break;
                    }
                    case ParameterKey::log_file_rotating_num: {
                        json_value[key_str] = (uint32_t)sLogFileRotatingNum;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               sLogFileRotatingNum );
                        break;
                    }
                    case ParameterKey::log_file_rotating_size: {
                        json_value[key_str] = (uint32_t)sLogFileRotatingSize;
                        Debug( "Get value of parameter '{}' (ID={}): {} KB", key_str, key_id,
                               sLogFileRotatingSize );
                        break;
                    }
                    case ParameterKey::license_type: {
                        auto it = LicenseTypeStringMap.find( mLicenseType );
                        if ( it == LicenseTypeStringMap.end() )
                            Unreachable( "License_type '{}' is missing in LicenseTypeStringMap. ",
                                (uint32_t)mLicenseType ); //LCOV_EXCL_LINE
                        std::string license_type_str = it->second;
                        json_value[key_str] = license_type_str;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                license_type_str );
                        break;
                    }
                    case ParameterKey::license_duration: {
                        json_value[key_str] = mLicenseDuration;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                mLicenseDuration );
                        break;
                    }
                    case ParameterKey::num_activators: {
                        uint32_t nbActivators = 0;
                        getNumActivator( nbActivators );
                        json_value[key_str] = nbActivators;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                nbActivators );
                        break;
                    }
                    case ParameterKey::session_id: {
                        json_value[key_str] = mSessionID;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                                mSessionID );
                        break;
                    }
                    case ParameterKey::session_status: {
                        bool status = isSessionRunning();
                        json_value[key_str] = status;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               status );
                        break;
                    }
                    case ParameterKey::license_status: {
                        bool status = isLicenseActive();
                        json_value[key_str] = status;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               status );
                        break;
                    }
                    case ParameterKey::metered_data: {
#if ((JSONCPP_VERSION_MAJOR ) >= 1 and ((JSONCPP_VERSION_MINOR) > 7 or ((JSONCPP_VERSION_MINOR) == 7 and JSONCPP_VERSION_PATCH >= 5)))
                        uint64_t meteringData = 0;
#else
                        // No "int64_t" support with JsonCpp < 1.7.5
                        unsigned long long meteringData = 0;
#endif
                        Json::Value json_request = getMeteringHealth();
                        std::string meteringFileStr = json_request["meteringFile"].asString();
                        if  ( meteringFileStr.size() ) {
                            std::string meteringDataStr = meteringFileStr.substr( 80, 16 );
                            errno = 0;
                            meteringData = strtoull( meteringDataStr.c_str(), nullptr, 16 );
                            if ( errno ) {
                                Throw( DRM_CtlrError, "Could not convert string '{}' to unsigned long long.",
                                    meteringDataStr );
                            }
                        }
                        json_value[key_str] = meteringData;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               meteringData );
                        break;
                    }
                    case ParameterKey::nodelocked_request_file: {
                        if ( mLicenseType != eLicenseType::NODE_LOCKED ) {
                            json_value[key_str] = std::string("Not applicable");
                            Warning( "Parameter only available with Node-Locked licensing" );
                        } else {
                            json_value[key_str] = mNodeLockRequestFilePath;
                            Debug( "Get value of parameter '{}' (ID={}): Node-locked license request file is saved in {}",
                                    key_str, key_id, mNodeLockRequestFilePath );
                        }
                        break;
                    }
                    case ParameterKey::page_ctrlreg:
                    case ParameterKey::page_vlnvfile:
                    case ParameterKey::page_licfile:
                    case ParameterKey::page_tracefile:
                    case ParameterKey::page_meteringfile:
                    case ParameterKey::page_mailbox: {
                        std::string str = getDrmPage( key_id - ParameterKey::page_ctrlreg );
                        json_value[key_str] = str;
                        Debug( "Get value of parameter '{}' (ID={})", key_str, key_id );
                        Info( str );
                        break;
                    }
                    case ParameterKey::hw_report: {
                        std::string str = getDrmReport();
                        json_value[key_str] = str;
                        Debug( "Get value of parameter '{}' (ID={})", key_str, key_id );
                        Info( "Print HW report:\n{}", str );
                        break;
                    }
                    case ParameterKey::drm_frequency: {
                        json_value[key_str] = mFrequencyCurr;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
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
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               status );
                        break;
                    }
                    case ParameterKey::bypass_frequency_detection: {
                        json_value[key_str] = mBypassFrequencyDetection;
                        Debug( "Get value of parameter '{}' (ID={}): Method {}", key_str, key_id,
                               mBypassFrequencyDetection );
                        break;
                    }
                    case ParameterKey::frequency_detection_method: {
                        int32_t method_index = 2;
                        if ( mIsFreqDetectionMethod1 )
                            method_index = 1;
                        json_value[key_str] = method_index;
                        Debug( "Get value of parameter '{}' (ID={}): Method {}", key_str, key_id,
                               method_index );
                        break;
                    }
                    case ParameterKey::frequency_detection_threshold: {
                        json_value[key_str] = mFrequencyDetectionThreshold;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               mFrequencyDetectionThreshold );
                        break;
                    }
                    case ParameterKey::frequency_detection_period: {
                        json_value[key_str] = mFrequencyDetectionPeriod;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               mFrequencyDetectionPeriod );
                        break;
                    }
                    case ParameterKey::product_info: {
                        json_value[key_str] = mHeaderJsonRequest["product"];
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               mHeaderJsonRequest["product"].toStyledString() );
                        break;
                    }
                    case ParameterKey::token_string: {
                        std::string token_str = getDrmWSClient().getTokenString();
                        json_value[key_str] = token_str;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               token_str );
                        break;
                    }
                    case ParameterKey::token_validity: {
                        uint32_t validity = getDrmWSClient().getTokenValidity();
                        json_value[key_str] = validity ;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               validity  );
                        break;
                    }
                    case ParameterKey::token_time_left: {
                        uint32_t time_left = getDrmWSClient().getTokenTimeLeft();
                        json_value[key_str] = time_left;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               time_left );
                        break;
                    }
                    case ParameterKey::mailbox_size: {
                        uint32_t mbSize = getUserMailboxSize();
                        json_value[key_str] = mbSize;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               mbSize );
                        break;
                    }
                    case ParameterKey::mailbox_data: {
                        uint32_t mbSize = getUserMailboxSize();
                        std::vector<uint32_t> data_array = readMailbox( eMailboxOffset::MB_USER, mbSize );
                        for( const auto& val: data_array )
                            json_value[key_str].append( val );
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               json_value[key_str].toStyledString() );
                        break;
                    }
                    case ParameterKey::ws_retry_period_long: {
                        json_value[key_str] = mWSRetryPeriodLong;
                        Debug( "Get value of parameter '", key_str,
                                "' (ID=", key_id, "): ", mWSRetryPeriodLong );
                        break;
                    }
                    case ParameterKey::ws_retry_period_short: {
                        json_value[key_str] = mWSRetryPeriodShort;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               mWSRetryPeriodShort );
                        break;
                    }
                    case ParameterKey::ws_request_timeout: {
                        json_value[key_str] = mWSRequestTimeout ;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               mWSRequestTimeout  );
                        break;
                    }
                    case ParameterKey::log_message_level: {
                        int32_t msgLevel = static_cast<int>( mDebugMessageLevel );
                        json_value[key_str] = msgLevel;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               msgLevel );
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = readMailbox( eMailboxOffset::MB_CUSTOM_FIELD );
                        json_value[key_str] = customField;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               customField );
                        break;
                    }
                    case ParameterKey ::list_all: {
                        Json::Value list = list_parameter_key();
                        json_value[key_str] = list;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               list.toStyledString() );
                        break;
                    }
                    case ParameterKey::dump_all: {
                        Json::Value list = dump_parameter_key();
                        json_value[key_str] = list;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               list.toStyledString() );
                        break;
                    }
                    case ParameterKey::hdk_compatibility: {
                        std::string hdk_limit = fmt::format( "{}.{}", HDK_COMPATIBILITY_LIMIT_MAJOR, HDK_COMPATIBILITY_LIMIT_MINOR );
                        json_value[key_str] = hdk_limit;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               hdk_limit );
                        break;
                    }
                    case ParameterKey::health_period: {
                        json_value[key_str] = mHealthPeriod;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id, mHealthPeriod );
                        break;
                    }
                    case ParameterKey::health_retry: {
                        json_value[key_str] = mHealthRetryTimeout;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id, mHealthRetryTimeout );
                        break;
                    }
                    case ParameterKey::health_retry_sleep: {
                        json_value[key_str] = mHealthRetrySleep;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id, mHealthRetrySleep );
                        break;
                    }
                    case ParameterKey::ParameterKeyCount: {
                        uint32_t count = static_cast<int>( ParameterKeyCount );
                        json_value[key_str] = count;
                        Debug( "Get value of parameter '{}' (ID={}): {}", key_str, key_id,
                               count );
                        break;
                    }
                    default: {
                        Throw( DRM_BadArg, "Parameter '{}' cannot be read", key_str );
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
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                                verbosityInt );
                        break;
                    }
                    case ParameterKey::log_format: {
                        std::string logFormat = (*it).asString();
                        sLogger->sinks()[0]->set_pattern( logFormat );
                        sLogConsoleFormat = logFormat;
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                                sLogConsoleFormat );
                        break;
                    }
                    case ParameterKey::log_file_verbosity: {
                        int32_t verbosityInt = (*it).asInt();
                        sLogFileVerbosity = static_cast<spdlog::level::level_enum>( verbosityInt );
                        sLogger->sinks()[1]->set_level( sLogFileVerbosity );
                        if ( sLogFileVerbosity < sLogger->level() )
                            sLogger->set_level( sLogFileVerbosity );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               verbosityInt);
                        break;
                    }
                    case ParameterKey::log_file_format: {
                        sLogFileFormat = (*it).asString();
                        if ( sLogger->sinks().size() > 1 ) {
                            sLogger->sinks()[1]->set_pattern( sLogFileFormat );
                        }
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               sLogFileFormat );
                        break;
                    }
                    case ParameterKey::frequency_detection_threshold: {
                        mFrequencyDetectionThreshold = (*it).asDouble();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               mFrequencyDetectionThreshold );
                        break;
                    }
                    case ParameterKey::frequency_detection_period: {
                        mFrequencyDetectionPeriod = (*it).asUInt();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               mFrequencyDetectionPeriod );
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = (*it).asUInt();
                        writeMailbox( eMailboxOffset::MB_CUSTOM_FIELD, customField );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               customField );
                        break;
                    }
                    case ParameterKey::mailbox_data: {
                        if ( !(*it).isArray() )
                            Throw( DRM_BadArg, "Value must be an array of integers" );
                        std::vector<uint32_t> data_array;
                        for( Json::ValueConstIterator itr = (*it).begin(); itr != (*it).end(); itr++ )
                            data_array.push_back( (*itr).asUInt() );
                        writeMailbox( eMailboxOffset::MB_USER, data_array );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               (*it).toStyledString());
                        break;
                    }
                    case ParameterKey::ws_retry_period_long: {
                        uint32_t retry_period = (*it).asUInt();
                        if ( retry_period <= mWSRetryPeriodShort )
                            Throw( DRM_BadArg,
                                    "ws_retry_period_long ({}) must be greater than ws_retry_period_short ({})",
                                    retry_period, mWSRetryPeriodShort );
                        mWSRetryPeriodLong = retry_period;
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               mWSRetryPeriodLong );
                        break;
                    }
                    case ParameterKey::ws_retry_period_short: {
                        uint32_t retry_period = (*it).asUInt();
                        if ( mWSRetryPeriodLong <= retry_period )
                            Throw( DRM_BadArg,
                                    "ws_retry_period_long ({}) must be greater than ws_retry_period_short ({})",
                                    mWSRetryPeriodLong, retry_period );
                        mWSRetryPeriodShort = retry_period;
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               mWSRetryPeriodShort );
                        break;
                    }
                    case ParameterKey::ws_request_timeout: {
                        mWSRequestTimeout  = (*it).asUInt();
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               mWSRequestTimeout  );
                        if ( mWSRequestTimeout == 0 )
                            Throw( DRM_BadArg, "ws_request_timeout must not be 0");
                        break;
                    }
                    case ParameterKey::trigger_async_callback: {
                        std::string custom_msg = (*it).asString();
                        Exception e( DRM_Debug, custom_msg );
                        f_asynch_error( e.what() );
                        Debug( "Set parameter '{}' (ID={}) to value: {}", key_str, key_id,
                               custom_msg );
                        break;
                    }
                    case ParameterKey::log_message_level: {
                        int32_t message_level = (*it).asInt();
                        if ( ( message_level < spdlog::level::trace)
                          || ( message_level > spdlog::level::off) )
                            Throw( DRM_BadArg, "log_message_level ({}) is out of range [{:d}:{:d}]",
                                    message_level, (int32_t)spdlog::level::trace, (int32_t)spdlog::level::off );
                        mDebugMessageLevel = static_cast<spdlog::level::level_enum>( message_level );
                        Debug( "Set parameter '{}' (ID={}) to value {}", key_str, key_id,
                                message_level );
                        break;
                    }
                    case ParameterKey::log_message: {
                        std::string custom_msg = (*it).asString();
                        SPDLOG_LOGGER_CALL( sLogger, (spdlog::level::level_enum)mDebugMessageLevel, custom_msg);
                        break;
                    }
                    default:
                        Throw( DRM_BadArg, "Parameter '{}' cannot be overwritten", key_str );
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
    IMPL_GET_BODY
    if ( json_value[key_str].isString() )
        return json_value[key_str].asString();
    return json_value[key_str].toStyledString();
}

template<> bool DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asBool();
}

template<> int32_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asInt();
}

template<> uint32_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asUInt();
}

template<> int64_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asInt64();
}

template<> uint64_t DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asUInt64();
}

template<> float DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asFloat();
}

template<> double DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asDouble();
}

#define IMPL_SET_BODY \
    Json::Value json_value; \
    std::string key_str = findParameterString( key_id ); \
    json_value[key_str] = value; \
    set( json_value );


template<> void DrmManager::Impl::set( const ParameterKey key_id, const std::string& value ) {
    IMPL_SET_BODY
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const bool& value ) {
    IMPL_SET_BODY
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const int32_t& value ) {
    IMPL_SET_BODY
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const uint32_t& value ) {
    IMPL_SET_BODY
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const int64_t& value ) {
    Json::Value json_value;
    std::string key_str = findParameterString( key_id );
    json_value[key_str] = Json::Int64( value );
    set( json_value );
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const uint64_t& value ) {
    Json::Value json_value;
    std::string key_str = findParameterString( key_id );
    json_value[key_str] = Json::UInt64( value );
    set( json_value );
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const float& value ) {
    IMPL_SET_BODY
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const double& value ) {
    IMPL_SET_BODY
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


void DrmManager::activate( const bool& resume_session ) {
    pImpl->activate( resume_session );
}

void DrmManager::deactivate( const bool& pause_session ) {
    pImpl->deactivate( pause_session );
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
