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

#include "accelize/drm/session_manager.h"
#include "accelize/drm/version.h"
#include "ws_client.h"
#include "log.h"
#include "utils.h"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "DrmControllerDataConverter.hpp"
#include "HAL/DrmControllerOperations.hpp"
#pragma GCC diagnostic pop

#define NB_MAX_REGISTER 32

#define TRY try {

#define CATCH_AND_THROW \
    } catch (const std::exception &e) { \
        throw; \
    }


namespace Accelize {
namespace DRM {


const char* getApiVersion() {
    return DRMLIB_VERSION;
}


class DRM_LOCAL DrmManager::Impl {

protected:

    // Design constants
    const uint32_t cRetryDeadline = 5;
    uint32_t cFrequencyDetectionPeriod = 100;  // in milliseconds
    double cFrequencyDetectionThreshold = 2.0;      // Error in percentage

    //helper typedef
    typedef std::chrono::steady_clock TClock; /// Shortcut type def to steady clock which is monotonic (so unaffected by clock adjustments)

    enum class LicenseType: uint8_t {OTHERS=0, NODE_LOCKED};

    enum class MailboxOffset {MB_LOCK_DRM=0, MB_CUSTOM_FIELD, MB_USER};

    std::map<LicenseType, std::string> LicenseTypeStringMap = {
            {LicenseType::OTHERS     , "Floating/Metering"},
            {LicenseType::NODE_LOCKED, "Node-Locked"}
    };

#ifdef _WIN32
    const char path_sep = '\\';
# else
    const char path_sep = '/';
#endif

    bool mSecurityStop;

    //composition
    std::unique_ptr<DrmWSClient> mWsClient;
    std::unique_ptr<DrmControllerLibrary::DrmControllerOperations> mDrmController;
    mutable std::recursive_mutex mDrmControllerMutex;

    //function callbacks
    DrmManager::ReadRegisterCallback f_read_register;
    DrmManager::WriteRegisterCallback f_write_register;
    DrmManager::AsynchErrorCallback f_asynch_error;

    // Parameters
    std::string mConfFilePath;
    std::string mCredFilePath;

    // Node-Locked parameters
    std::string mNodeLockLicenseDirPath;
    std::string mNodeLockRequestFilePath;
    std::string mNodeLockLicenseFilePath;

    // License related properties
    uint32_t mRetryDeadline;    ///< Time in seconds before the license expired during which no more retry is authorized
    LicenseType mLicenseType = LicenseType::OTHERS;
    uint32_t mLicenseCounter;
    TClock::time_point mLicenseExpirationDate;
    uint32_t mLicenseDuration;

    // Design parameters
    int32_t mFrequencyInit;
    int32_t mFrequencyCurr;
    uint32_t mFrequencyDetectionPeriod = 100;  // in milliseconds
    double mFrequencyDetectionThreshold = 2.0;      // Error in percentage

    // Session state
    std::string mSessionID;
    std::string mUDID;
    std::string mBoardType;

    // Web service communication
    Json::Value mHeaderJsonRequest;

    // thread to maintain alive
    std::future<void> mThreadKeepAlive;
    std::mutex mThreadKeepAliveMtx;
    std::condition_variable mThreadKeepAliveCondVar;
    bool mThreadStopRequest{false};

    const std::map<ParameterKey, std::string> mParameterKeyMap = {
    #   define PARAMETERKEY_ITEM(id) {id, #id},
    #   include "accelize/drm/ParameterKey.def"
    #   undef PARAMETERKEY_ITEM
        {ParameterKeyCount, "ParameterKeyCount"}
    };


    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path )
    {
        Json::Value conf_json;

        mSecurityStop = false;

        mLicenseCounter = 0;
        mLicenseDuration = 0;

        mConfFilePath = conf_file_path;
        mCredFilePath = cred_file_path;

        // Parse configuration file
        conf_json = parseConfiguration( conf_file_path );

        try {

            Json::Value param_lib = JVgetOptional( conf_json, "parameter_key", Json::objectValue );
            if (!param_lib.empty()) {
                mRetryDeadline = JVgetOptional( param_lib, "retry_deadline", Json::uintValue, cRetryDeadline).asUInt();
                mFrequencyDetectionPeriod = JVgetOptional( param_lib, "frequency_detection_period", Json::uintValue,
                        cFrequencyDetectionPeriod).asUInt();
                mFrequencyDetectionThreshold = JVgetOptional( param_lib, "frequency_detection_threshold", Json::uintValue,
                        cFrequencyDetectionThreshold).asDouble();
            } else {
                mRetryDeadline = cRetryDeadline;
                mFrequencyDetectionPeriod = cFrequencyDetectionPeriod;
                mFrequencyDetectionThreshold = cFrequencyDetectionThreshold;
            }

            // Design configuration
            Json::Value conf_design = JVgetOptional( conf_json, "design", Json::objectValue );
            if (!conf_design.empty()) {
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
                mLicenseType = LicenseType::NODE_LOCKED;
                Debug( "Detected Node-locked license" );
            } else {
                Debug( "Detected floating/metered license" );
                // Get DRM frequency
                Json::Value conf_drm = JVgetRequired( conf_json, "drm", Json::objectValue );
                mFrequencyInit = JVgetRequired( conf_drm, "frequency_mhz", Json::intValue ).asUInt();
                mFrequencyCurr = mFrequencyInit;
                // Instantiate Web Service client
                mWsClient.reset( new DrmWSClient(conf_file_path, cred_file_path) );
            }

        } catch(Exception &e) {
            if (e.getErrCode() != DRM_BadFormat)
                throw;
            Throw(DRM_BadFormat, "Error in configuration file '", conf_file_path, "': ", e.what());
        }
    }

    void initDrmInterface() {
        if (mDrmController)
            return;

        // create instance
        mDrmController.reset( new DrmControllerLibrary::DrmControllerOperations(
            std::bind(&DrmManager::Impl::readDrmRegister, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&DrmManager::Impl::writeDrmRegister, this, std::placeholders::_1, std::placeholders::_2 )
        ));

        // Try to lock the DRM controller to this instance, return an error is already locked.
        lockDrmToInstance();

        // Save header information
        mHeaderJsonRequest = getMeteringHeader();

        // If node-locked license is requested, create license request file
        if ( mLicenseType == LicenseType::NODE_LOCKED ) {
            // Check license directory exists
            if ( !dirExists( mNodeLockLicenseDirPath ) )
                Throw( DRM_BadArg, "License directory path '", mNodeLockLicenseDirPath,
                        "' specified in configuration file '", mConfFilePath, "' is not existing on file system");
            // Create license request file
            createNodelockedLicenseRequestFile();
        }
    }

    bool dirExists( const std::string& dir_path ) {
        struct stat info;
        if ( stat( dir_path.c_str(), &info ) != 0 )
            return false;
        if ( info.st_mode & S_IFDIR )
            return true;
        return false;
    }

    uint32_t getMailboxSize() const {
        uint32_t roSize, rwSize;
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileSizeRegister( roSize, rwSize ) );
        Debug2("Read Mailbox size: ", rwSize);
        return rwSize;
    }

    uint32_t readMailbox( const MailboxOffset offset ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( index >= rwData.size() )
            Throw( DRM_BadArg, "Index ", index, " overflows the Mailbox memory: max index is ", rwData.size()-1 );
        Debug( "Read '", rwData[index], "' in Mailbox at index ", index );
        return rwData[index];
    }

    std::vector<uint32_t> readMailbox( const MailboxOffset offset, const uint32_t& nb_elements ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( (uint32_t)index >= rwData.size() )
            Throw( DRM_BadArg, "Index ", index, " overflows the Mailbox memory: max index is ", rwData.size()-1 );
        if ( index + nb_elements > rwData.size() )
            Throw( DRM_BadArg, "Trying to write over Mailbox memory: ", rwData.size() );

        auto first = rwData.cbegin() + index;
        auto last = rwData.cbegin() + index + nb_elements;
        std::vector<uint32_t> value_vec( first, last );
        Debug( "Read ", value_vec.size(), " elements in Mailbox from index ", index);
        return value_vec;
    }

    void writeMailbox( const MailboxOffset offset, const uint32_t& value ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lockk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );

        if ( index >= rwData.size() )
            Throw( DRM_BadArg, "Index ", index, " overflows the Mailbox memory: max index is ", rwData.size()-1 );
        rwData[index] = value;
        checkDRMCtlrRet( getDrmController().writeMailboxFileRegister( rwData, rwSize ) );
        Debug( "Wrote '", value, "' in Mailbox at index ", index );
    }

    void writeMailbox( const MailboxOffset offset, const std::vector<uint32_t> &value_vec ) const {
        auto index = (uint32_t)offset;
        uint32_t roSize, rwSize;
        std::vector<uint32_t> roData, rwData;

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, roData, rwData) );
        if ( index >= rwData.size() )
            Throw( DRM_BadArg, "Index ", index, " overflows the Mailbox memory: max index is ", rwData.size()-1 );
        if ( index + value_vec.size() > rwData.size() )
            Throw( DRM_BadArg, "Trying to write over Mailbox memory: ", rwData.size() );
        std::copy( std::begin(value_vec), std::end(value_vec), std::begin(rwData) + index );
        checkDRMCtlrRet( getDrmController().writeMailboxFileRegister( rwData, rwSize ) );
        Debug( "Wrote ", value_vec.size(), " elements in Mailbox from index ", index );
    }

    DrmControllerLibrary::DrmControllerOperations& getDrmController() const {
        Assert(mDrmController);
        return *mDrmController;
    }

    DrmWSClient& getDrmWSClient() {
        if ( !mWsClient )
            Throw( DRM_BadUsage, "DRM Session Manager constructed with no WebService configuration" );
        return *mWsClient;
    }

    static uint32_t getDrmRegisterOffset(const std::string& regName) {
        if ( regName == "DrmPageRegister" )
            return 0;
        if ( regName.substr(0, 15) == "DrmRegisterLine" )
            return (uint32_t)std::stoul(regName.substr(15))*4+4;
        Unreachable( "Unsupported regName argument: " + regName );
    }

    unsigned int readDrmRegister(const std::string& regName, unsigned int& value) const {
        int ret = 0;
        ret = f_read_register( getDrmRegisterOffset(regName), &value );
        if (ret != 0) {
            Error("Error in read register callback, errcode = ", ret);
            return (uint32_t)-1;
        }
        Debug2("Read DRM register @", regName, " = 0x", std::hex, value, std::dec);
        return 0;
    }

    unsigned int writeDrmRegister(const std::string& regName, unsigned int value) const {
        int ret = 0;
        ret = f_write_register( getDrmRegisterOffset(regName), value );
        if(ret != 0) {
            Error("Error in write register callback, errcode = ", ret);
            return (uint32_t)-1;
        }
        Debug2("Write DRM register @", regName, " = 0x", std::hex, value, std::dec);
        return 0;
    }

    void checkDRMCtlrRet(unsigned int errcode) const {
        if ( errcode )
            Throw(DRM_ExternFail, "Error in DRMCtlrLib function call : ", errcode);
    }

    void lockDrmToInstance() {
        return;
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        uint32_t isLocked = readMailbox( MailboxOffset::MB_LOCK_DRM );
        if ( isLocked )
            Throw(DRM_BadUsage, "Another instance of the DRM Manager is currently owning the HW");
        writeMailbox( MailboxOffset::MB_LOCK_DRM, 1 );
        Debug("DRM Controller is now locked to this object instance");
    }

    void unlockDrmToInstance() {
        return;
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        uint32_t isLocked = readMailbox( MailboxOffset::MB_LOCK_DRM );
        if ( isLocked ) {
            writeMailbox( MailboxOffset::MB_LOCK_DRM, 0 );
            Debug("DRM Controller is now unlocked to this object instance");
        }
    }

    // Check compatibility of the DRM Version with Algodone version
    void checkDrmCompatibility( const std::string& drmVersion) {
        uint32_t drmVersionNum = DrmControllerLibrary::DrmControllerDataConverter::hexStringToBinary(drmVersion)[0];
        std::string drmVersionDot = DrmControllerLibrary::DrmControllerDataConverter::binaryToVersionString(drmVersionNum);
        auto drmMajor = (uint8_t)((drmVersionNum >> 16) & 0xFF);
        auto drmMinor = (uint8_t)((drmVersionNum >> 8) & 0xFF);
        if (drmMajor < RETROCOMPATIBLITY_LIMIT_MAJOR) {
            Throw(DRM_CtlrError, "This DRM Lib ", getApiVersion()," is not compatible with the DRM HDK version ",
                    drmVersionDot,": To be compatible HDK version shall be > or equal to ",
                    RETROCOMPATIBLITY_LIMIT_MAJOR,".",RETROCOMPATIBLITY_LIMIT_MINOR,".0");
        } else if (drmMinor < RETROCOMPATIBLITY_LIMIT_MINOR) {
            Throw(DRM_CtlrError, "This DRM Lib ", getApiVersion()," is not compatible with the DRM HDK version ",
                    drmVersionDot,": To be compatible HDK version shall be > or equal to ",
                    RETROCOMPATIBLITY_LIMIT_MAJOR,".",RETROCOMPATIBLITY_LIMIT_MINOR,".0");
        }
        Debug("DRM Version = ", drmVersionDot);
    }

    void checkSessionID(Json::Value license_json) {
        std::string ws_sessionID = license_json["metering"]["sessionId"].asString();
        if ( !mSessionID.empty() && (mSessionID != ws_sessionID) ) {
            Throw(DRM_Fatal, "Session ID mismatch: received '", ws_sessionID, "' from WS but expect '", mSessionID, "'");
        }
    }

    void getNumActivator( uint32_t& value ) const {
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readNumberOfDetectedIpsStatusRegister(value) );
    }

    uint64_t getTimerCounterValue() const {
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        uint32_t licenseTimerCounterMsb(0), licenseTimerCounterLsb(0);
        uint64_t licenseTimerCounter;
        checkDRMCtlrRet( getDrmController().sampleLicenseTimerCounter( licenseTimerCounterMsb, licenseTimerCounterLsb ) );
        licenseTimerCounter = licenseTimerCounterMsb;
        licenseTimerCounter <<= 32;
        licenseTimerCounter += licenseTimerCounterLsb;
        return licenseTimerCounter;
    }

    std::string getDrmPage( uint32_t page_index ) const {
        std::stringstream ss;
        uint32_t value;
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        writeDrmRegister("DrmPageRegister", page_index);
        ss << "DRM Page " << page_index << " registry:\n";
        for(uint32_t r=0; r < NB_MAX_REGISTER; r++) {
            f_read_register(r*4, &value);
            ss << "\tRegister @0x" << std::setfill('0') << std::setw(2) << std::hex << (r*4) << ": ";
            ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
            ss << " (" << std::setfill(' ') << std::setw(10) << std::dec << value << ")\n";
        }
        return ss.str();
    }

    std::string getDrmReport() const {
        std::stringstream ss;
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        getDrmController().printHwReport(ss);
        return ss.str();
    }

    Json::Value getJsonFromString( const std::string& json_string ) const {
        Json::Value json_object;
        Json::Reader reader;
        if ( !reader.parse( json_string, json_object ) )
            Throw(DRM_BadFormat, "Cannot parse JSON string:\n", json_string, "\nBecause: ", reader.getFormattedErrorMessages());
        return json_object;
    }

    uint64_t getMeteringData() const {
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;
        uint64_t meteringData = 0;

        Debug2("Get metering data from session on DRM controller");

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().asynchronousExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        std::string meteringDataStr = meteringFile[2].substr(16, 16);
        errno = 0;
        meteringData = strtoull(meteringDataStr.c_str(), nullptr, 16);
        if (errno)
            Throw(DRM_BadUsage, "Could not convert string '", meteringDataStr, "' to unsigned long long.");
        return meteringData;
    }

    // Get common info
    void getDesignInfo(std::string &drmVersion,
                       std::string &dna,
                       std::vector<std::string> &vlnvFile,
                       std::string &mailboxReadOnly ) {
        uint32_t nbOfDetectedIps;
        uint32_t readOnlyMailboxSize, readWriteMailboxSize;
        std::vector<uint32_t> readOnlyMailboxData, readWriteMailboxData;

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet(getDrmController().extractDrmVersion( drmVersion ) );
        checkDRMCtlrRet(getDrmController().extractDna( dna ) );
        checkDRMCtlrRet(getDrmController().extractVlnvFile( nbOfDetectedIps, vlnvFile ) );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( readOnlyMailboxSize, readWriteMailboxSize,
                                                                     readOnlyMailboxData, readWriteMailboxData ) );
        Debug( "Mailbox sizes: read-only=", readOnlyMailboxSize, ", read-write=", readWriteMailboxSize );
        if ( readOnlyMailboxSize ) {
            readOnlyMailboxData.push_back(0);
            mailboxReadOnly = std::string( (char*)readOnlyMailboxData.data() );
        }
        else
            mailboxReadOnly = std::string("");
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
        json_output["mode"] = (uint8_t) mLicenseType;
        json_output["drm_frequency_init"] = mFrequencyInit;

        // Get information from DRM Controller
        getDesignInfo(drmVersion, dna, vlnvFile, mailboxReadOnly);

        // Check compatibility of the DRM Version with Algodone version
        checkDrmCompatibility(drmVersion);

        // Fulfill DRM section
        json_output["drmlibVersion"] = getApiVersion();
        json_output["lgdnVersion"] = drmVersion;
        json_output["dna"] = dna;
        for (uint32_t i = 0; i < vlnvFile.size(); i++) {
            std::string i_str = std::to_string(i);
            json_output["vlnvFile"][i_str]["vendor"] = std::string("x") + vlnvFile[i].substr(0, 4);
            json_output["vlnvFile"][i_str]["library"] = std::string("x") + vlnvFile[i].substr(4, 4);
            json_output["vlnvFile"][i_str]["name"] = std::string("x") + vlnvFile[i].substr(8, 4);
            json_output["vlnvFile"][i_str]["version"] = std::string("x") + vlnvFile[i].substr(12, 4);
        }

        if ( !mailboxReadOnly.empty() ) {
            try {
                json_output["product"] = getJsonFromString(mailboxReadOnly);
            } catch ( const Exception &e ) {
                if (e.getErrCode() == DRM_BadFormat)
                    Throw(DRM_BadFormat, "Failed to parse Read-Only Mailbox in DRM Controller");
                throw;
            }
        }
        return json_output;
    }

    Json::Value getMeteringStart() {
        Json::Value json_output(mHeaderJsonRequest);
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug("Build web request to create new session");

        mLicenseCounter = 0;

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().initialization( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "open";
        json_output["drm_frequency"] = mFrequencyCurr;
        json_output["mode"] = (uint8_t)mLicenseType;

        return json_output;
    }

    Json::Value getMeteringWait( uint32_t timeOut ) {
        Json::Value json_output(mHeaderJsonRequest);
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug("Build web request to maintain current session");

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        //DEPRECATED => checkDRMCtlrRet( getDrmController().extractMeteringFile( timeOut, numberOfDetectedIps, saasChallenge, meteringFile ) );
        checkDRMCtlrRet( getDrmController().waitNotTimerInitLoaded( timeOut ) );
        checkDRMCtlrRet( getDrmController().synchronousExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        mSessionID = meteringFile[0].substr(0, 16);
        json_output["sessionId"] = mSessionID;
        json_output["drm_frequency"] = mFrequencyCurr;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "running";
        return json_output;
    }

    Json::Value getMeteringStop() {
        Json::Value json_output(mHeaderJsonRequest);
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug("Build web request to stop current session");

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().endSessionAndExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        json_output["sessionId"] = mSessionID;
        json_output["drm_frequency"] = mFrequencyCurr;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "close";
        return json_output;
    }

    bool isSessionRunning()const  {
        bool sessionRunning(false);
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readSessionRunningStatusRegister(sessionRunning) );
        Debug( "DRM session running state: ", sessionRunning );
        return sessionRunning;
    }

    bool isReadyForNewLicense() const {
        bool ret(false);
        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readLicenseTimerInitLoadedStatusRegister(ret) );
        Debug( "DRM readiness to receive a new license: ", !ret );
        return !ret;
    }

    void setLicense(const Json::Value& license_json) {

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);

        Debug("Installing next license on DRM controller");

        /// Get session ID received from web service
        if ( mSessionID.empty() )
            /// Save new Session ID
            mSessionID = license_json["metering"]["sessionId"].asString();
        else
            /// Verify Session ID
            checkSessionID( license_json );

        // get DNA
        std::string dna;
        checkDRMCtlrRet( getDrmController().extractDna( dna ) );

        // Extract license and license timer from webservice response
        std::string licenseKey   = license_json["license"][dna]["key"].asString();
        std::string licenseTimer = license_json["license"][dna]["licenseTimer"].asString();

        if ( licenseKey.empty() )
            Throw(DRM_WSRespError, "Malformed response from Accelize WebService : license key is empty");
        if ( (mLicenseType != LicenseType::NODE_LOCKED) && licenseTimer.empty() )
            Throw(DRM_WSRespError, "Malformed response from Accelize WebService : LicenseTimer is empty");

        // Activate
        bool activationDone = false;
        uint8_t activationErrorCode;
        checkDRMCtlrRet( getDrmController().activate( licenseKey, activationDone, activationErrorCode ) );
        if( activationErrorCode ) {
            Throw(DRM_CtlrError, "Failed to activate license on DRM controller, activationErr: 0x",
                    std::hex, activationErrorCode, std::dec);
        }

        // Load license timer
        if (mLicenseType != LicenseType::NODE_LOCKED) {
            bool licenseTimerEnabled = false;
            checkDRMCtlrRet( getDrmController().loadLicenseTimerInit(licenseTimer, licenseTimerEnabled));
            if ( !licenseTimerEnabled ) {
                Throw(DRM_CtlrError, "Failed to load license timer on DRM controller, licenseTimerEnabled: 0x",
                      std::hex, licenseTimerEnabled, std::dec);
            }

            mLicenseCounter ++;

            mLicenseDuration = license_json["metering"]["timeoutSecond"].asUInt();
            Debug( "Set license #", mLicenseCounter, " of session ID ", mSessionID, " for a duration of ",
                   mLicenseDuration, " seconds" );
        }

        if (getLogLevel() >= eLogLevel::DEBUG2) {
            Info("Print HW report:\n", getDrmReport());
        }
    }

    std::string getDesignHash() {
        std::hash<std::string> hasher;
        std::stringstream design_ss;
        std::stringstream hash_ss;
        std::string drmVersion;
        std::string dna;
        std::vector<std::string> vlnvFile;
        std::string mailboxReadOnly;

        getDesignInfo(drmVersion, dna, vlnvFile, mailboxReadOnly);
        design_ss << dna;
        design_ss << drmVersion;
        for(const std::string& vlnv: vlnvFile)
            design_ss << vlnv;

        hash_ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << hasher( design_ss.str() );
        Debug( "Hash for HW design is ", hash_ss.str() );
        return hash_ss.str();
    }

    void createNodelockedLicenseRequestFile() {
        // Build request for node-locked license
        Json::Value request_json = getMeteringStart();
        Debug("License request JSON: ", request_json.toStyledString());
        // Create hash name based on design info
        std::string designHash = getDesignHash();
        mNodeLockRequestFilePath = mNodeLockLicenseDirPath + path_sep + designHash + ".req";
        mNodeLockLicenseFilePath = mNodeLockLicenseDirPath + path_sep + designHash + ".lic";
        Debug("Creating hash name based on design info: ", designHash);
        // - Save license request to file
        std::ofstream ofs( mNodeLockRequestFilePath );
        Json::StyledWriter json_writer;
        ofs << json_writer.write( request_json );
        if (!ofs.good())
            Throw(DRM_ExternFail, "Unable to write node-locked license request to file: ", mNodeLockRequestFilePath);
        ofs.close();
        Debug("License request file saved on: ", mNodeLockRequestFilePath);
    }

    void installNodelockedLicense() {
        Json::Value license_json;
        std::ifstream ifs;

        Debug("Looking for local node-locked license file: ", mNodeLockLicenseFilePath);

        ifs.open( mNodeLockLicenseFilePath );
        if ( ifs.good() ) {
            // A license file has already been installed locally; read its content
            ifs >> license_json;
            if ( !ifs.good() )
                Throw( DRM_ExternFail, "Cannot parse license file ", mNodeLockLicenseFilePath );
            ifs.close();
            Debug("Found and loaded local node-locked license file: ", mNodeLockLicenseFilePath);
        } else {
            // No license has been found locally, request one:
            // - Create access to license web service
            mWsClient.reset( new DrmWSClient( mConfFilePath, mCredFilePath ) );
            // - Read request file
            Json::Value request_json;
            ifs.open( mNodeLockRequestFilePath );
            if ( !ifs.is_open() ) {
                Throw( DRM_ExternFail, "Failed to access license request file ", mNodeLockRequestFilePath );
            }
            ifs >> request_json;
            if ( !ifs.good() ) {
                Throw( DRM_ExternFail, "Failed to parse license request file ", mNodeLockRequestFilePath );
            }
            ifs.close();
            // - Send request to web service and receive the new license
            license_json = getDrmWSClient().getLicense( request_json );
            // - Save license to file
            std::ofstream ofs( mNodeLockLicenseFilePath );
            Json::StyledWriter json_writer;
            ofs << json_writer.write( license_json );
            if (!ofs.good())
                Throw(DRM_ExternFail, "Unable to write newly received node-locked license to file: ", mNodeLockLicenseFilePath);
            ofs.close();
            Debug("Requested and saved new node-locked license file: ", mNodeLockLicenseFilePath);
        }
        // Install the license
        setLicense( license_json );
        Info("Installed node-locked license successfully");
    }

    void detectDrmFrequency() {
        TClock::time_point timeStart, timeEnd;
        uint64_t counterStart, counterEnd;
        TClock::duration wait_duration = std::chrono::milliseconds(mFrequencyDetectionPeriod);

        std::lock_guard<std::recursive_mutex> lock(mDrmControllerMutex);

        Debug("Detecting DRM frequency");
        Debug( "Detection period is ", mFrequencyDetectionPeriod, " ms" );

        counterStart = getTimerCounterValue();
        timeStart = TClock::now();

        //usleep( mFrequencyDetectionPeriod * 1000 );
        /// Check if a stop has been requested till now
        if ( isStopRequested( wait_duration ) )
            return;

        counterEnd = getTimerCounterValue();
        timeEnd = TClock::now();
        if (counterEnd == 0 )
            Unreachable( "Frequency auto-detection failed: license timeout counter is already 0" );
        Debug( "Start time = ", timeStart.time_since_epoch().count(), " / Counter start = ", counterStart );
        Debug( "End time = ", timeEnd.time_since_epoch().count(), " / Counter end = ", counterEnd );

        TClock::duration timeSpan = timeEnd - timeStart;
        double seconds = double(timeSpan.count()) * TClock::period::num / TClock::period::den;
        auto ticks = (uint32_t)(counterStart - counterEnd);
        Debug( "Duration = ", seconds, " s   /   ticks = ", ticks );

        // Estimate DRM frequency
        auto measuredFrequency = (int32_t)(std::ceil((double)ticks / seconds / 1000000));
        double precisionError = 100.0 * abs(measuredFrequency - mFrequencyCurr) / mFrequencyCurr ; // At that point mFrequencyCurr = mFrequencyInit
        if ( precisionError >= mFrequencyDetectionThreshold ) {
            mFrequencyCurr = measuredFrequency;
            Throw( DRM_BadFrequency, "Detected DRM frequency (",mFrequencyCurr," MHz) differs from the value (",
                   mFrequencyInit," MHz) defined in the configuration file '",
                   mConfFilePath,"' by more than ", mFrequencyDetectionThreshold, "%: From now on the considered frequency is ", mFrequencyCurr, " MHz");
        } else {
            Debug( "Detected frequency = ", measuredFrequency, " MHz, config frequency = ", mFrequencyInit,
                  " MHz: gap = ", precisionError, "%" );
        }
    }

    template< class Clock, class Duration >
    bool isStopRequested(const std::chrono::time_point<Clock, Duration> &timeout_time) {
        std::unique_lock<std::mutex> lk(mThreadKeepAliveMtx);
        return mThreadKeepAliveCondVar.wait_until(lk, timeout_time, [this]{return mThreadStopRequest;});
    }

    template< class Rep, class Period >
    bool isStopRequested(const std::chrono::duration<Rep, Period> &rel_time) {
        std::unique_lock<std::mutex> lk(mThreadKeepAliveMtx);
        return mThreadKeepAliveCondVar.wait_for(lk, rel_time, [this]{return mThreadStopRequest;});
    }

    bool isStopRequested() {
        std::lock_guard<std::mutex> lk(mThreadKeepAliveMtx);
        return mThreadStopRequest;
    }

    TClock::time_point getCurrentLicenseExpirationDate() {
        TClock::time_point now = TClock::now();
        uint64_t counterCurr = getTimerCounterValue();
        auto secondLeft = (uint32_t)std::ceil((double)counterCurr / mFrequencyCurr / 1000000);
        return now + std::chrono::seconds( secondLeft );

    }

    void startThread() {

        if(mThreadKeepAlive.valid()) {
            Warning("Thread already started");
            return;
        }

        Debug( "Starting background thread which maintains licensing" );

        mThreadKeepAlive = std::async(std::launch::async, [this](){
            try{
                /// Detecting DRM controller frequency
                detectDrmFrequency();

                /// Check if a stop has been requested till now
                if (isStopRequested() )
                    return;

                /// Starting license request loop
                while(1) {

                    if ( !isReadyForNewLicense() ) {

                        TClock::duration wait_duration = getCurrentLicenseExpirationDate() - TClock::now() + std::chrono::seconds(1);
                        long wait_seconds = std::chrono::duration_cast<std::chrono::seconds>( wait_duration ).count();
                        Debug( "Sleeping for ", wait_seconds, " seconds before checking if DRM Controller is ready to received a new license");

                        if ( isStopRequested( wait_duration ) )
                            return;

                        continue;
                    }

                    Debug( "A new license request is starting now: ", TClock::now().time_since_epoch().count() );

                    Json::Value request_json = getMeteringWait(1);
                    Json::Value license_json;
                    std::string retry_msg;
                    uint32_t retry_counter = 0;

                    if ( mLicenseDuration == 0 ) {
                        Unreachable( "mLicenseDuration is 0" );
                    }

                    /// Retry Web Service request loop
                    TClock::time_point polling_deadline = TClock::now() + std::chrono::seconds( mLicenseDuration )
                            - std::chrono::seconds( mRetryDeadline );
                    while(1) {

                        if (isStopRequested() )
                            return;

                        /// Verify deadline time is not hit yet
                        if ( polling_deadline <= TClock::now() ) {
                            Throw(DRM_WSError, "Failed to obtain license from Web Service on time, the protected IP may have been locked");
                        }

                        /// Attempt to get the next license
                        try {
                            license_json = getDrmWSClient().getLicense( request_json, polling_deadline );
                            break;
                        } catch(const Exception& e) {
                            if ( e.getErrCode() == DRM_WSMayRetry ) throw;
                                retry_msg = e.what();
                            }
                        /// Attempt failed, computing the sleep period before next retry. It depends on the time left.
                        TClock::duration wait_before_retry;
                        if ( polling_deadline - TClock::now() <= std::chrono::seconds(60) )
                            wait_before_retry = std::chrono::seconds(2);
                        else
                            wait_before_retry = std::chrono::seconds(30);
                        long wait_time = std::chrono::duration_cast<std::chrono::seconds>(wait_before_retry).count();
                        Warning( "Attempt #", retry_counter, " to obtain new license from Web Service failed with message: ", retry_msg,
                                ". New attempt planned in ", wait_time, " seconds");

                        /// Sleep until a stop is requested or timeout is hit
                        if (isStopRequested(wait_before_retry) )
                            return;
                    }

                    /// Check if stop has been requested
                    if (isStopRequested() )
                        return;

                    /// New license has been received: now send it to the DRM Controller
                    setLicense(license_json);
                }
            } catch(const std::exception& e) {
                Error(e.what());
                f_asynch_error( std::string( e.what() ) );
            }
        });
    }

    void stopThread() {
        if ( !mThreadKeepAlive.valid() ) {
            Debug("Background thread was not running");
            return;
        }
        {
            std::lock_guard<std::mutex> lk(mThreadKeepAliveMtx);
            mThreadStopRequest = true;
        }
        mThreadKeepAliveCondVar.notify_all();
        mThreadKeepAlive.get();
        Debug( "Background thread stopped" );
    }

    void startSession() {

        Info("Starting a new metering session...");

        // Build request message for new license
        Json::Value request_json = getMeteringStart();

        // Send request and receive new license
        Json::Value license_json = getDrmWSClient().getLicense(request_json);
        setLicense(license_json);

        // Save session ID locally and into web service request header for later request needs
        {
            std::lock_guard<std::mutex> lk(mThreadKeepAliveMtx);
            mThreadStopRequest = false;
        }
        startThread();
    }

    void resumeSession() {

        Info("Resuming DRM session...");

        if ( isReadyForNewLicense() ) {

            // Create JSON license request
            Json::Value request_json = getMeteringWait(1);

            // Send license request to web service
            Json::Value license_json = getDrmWSClient().getLicense( request_json );

            // Install license on DRM controller
            setLicense( license_json );
        }
        {
            std::lock_guard<std::mutex> lk(mThreadKeepAliveMtx);
            mThreadStopRequest = false;
        }
        startThread();
    }

    void stopSession() {

        if (mLicenseType == LicenseType::NODE_LOCKED){
            return;
        }

        Info("Stopping DRM session...");

        stopThread();

        // Get and send metering data to web service
        Json::Value request_json = getMeteringStop();

        // Send request and receive answer.
        Json::Value license_json = getDrmWSClient().getLicense( request_json );
        checkSessionID( license_json );
        Info("Session ID ", mSessionID, " stopped and last metering data uploaded");

        /// Clear Session IS
        mSessionID = std::string("");
    }

    void pauseSession() {
        Info("Pausing DRM session...");
        stopThread();
        mSecurityStop = false;
    }

    ParameterKey findParameterKey( const std::string& key_string ) const {
        for (auto const& it : mParameterKeyMap) {
            if (key_string == it.second)
                return it.first;
        }
        Throw(DRM_BadArg, "Cannot find parameter: ", key_string);
    }

    std::string findParameterString( const ParameterKey key_id ) const {
        std::map<ParameterKey, std::string>::const_iterator it;
        it = mParameterKeyMap.find(key_id);
        if ( it == mParameterKeyMap.end() )
            Throw(DRM_BadArg, "Cannot find parameter with id: ", key_id);
        return it->second;
    }

    Json::Value list_parameter_key() const {
        Json::Value node;
        for(int i=0; i<ParameterKey::ParameterKeyCount; i++) {
            ParameterKey e = static_cast<ParameterKey>(i);
            std::string keyStr = findParameterString(e);
            node.append( keyStr );
        }
        return node;
    }

    Json::Value dump_parameter_key() const {
        Json::Value node;
        for(int i=0; i<ParameterKey::dump_all; i++) {
            ParameterKey e = static_cast<ParameterKey>(i);
            std::string keyStr = findParameterString(e);
            node[ keyStr ] = Json::nullValue;
        }
        get(node);
        return node;
    }


public:
    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path,
          ReadRegisterCallback f_user_read_register,
          WriteRegisterCallback f_user_write_register,
          AsynchErrorCallback f_user_asynch_error )
        : Impl(conf_file_path, cred_file_path)
    {
        if (!f_user_read_register)
            Throw( DRM_BadArg, "Read register callback function must not be NULL" );
        if (!f_user_write_register)
            Throw( DRM_BadArg, "Write register callback function must not be NULL" );
        if (!f_user_asynch_error)
            Throw( DRM_BadArg, "Error handling callback function must not be NULL" );
        f_read_register = f_user_read_register;
        f_write_register = f_user_write_register;
        f_asynch_error = f_user_asynch_error;
        initDrmInterface();
    }

    ~Impl() {
        if ( isSessionRunning() ) {
            if (mSecurityStop) {
                Debug("Security stop triggered: stopping current session");
                stopSession();
            } else {
                Debug("Security stop not triggered: current session is kept open");
            }
        }
        stopThread();
        unlockDrmToInstance();
    }

    // Non copyable non movable as we create closure with "this"
    Impl( const Impl& ) = delete;
    Impl( Impl&& ) = delete;

    void activate( const bool& resume_session_request = false ) {
        TRY
            Debug("Calling 'activate' with 'resume_session_request'=", resume_session_request);

            if (mLicenseType == LicenseType::NODE_LOCKED) {
                installNodelockedLicense();
                return;
            }
            mSecurityStop = true;
            bool isRunning = isSessionRunning();
            if ( isRunning && resume_session_request ) {
                resumeSession();
            } else {
                if ( isRunning && !resume_session_request ) {
                    Debug("Session is already running but resume flag is ", resume_session_request,
                          ": stopping this pending session");
                    try {
                        stopSession();
                    } catch (const Exception &e) {
                        Debug("Failed to stop pending session: ", e.what());
                    }
                }
                startSession();
            }
        CATCH_AND_THROW
    }

    void deactivate( const bool& pause_session_request = false ) {
        TRY
            Debug("Calling 'deactivate' with 'pause_session_request'=", pause_session_request);

            if (mLicenseType == LicenseType::NODE_LOCKED) {
                return;
            }
            if ( !isSessionRunning() ) {
                Debug("No session is currently running");
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
            Debug( "Get parameter request input: ", json_value.toStyledString() );
            for (const std::string& key_str : json_value.getMemberNames()) {
                const ParameterKey key_id = findParameterKey( key_str );
                switch(key_id) {
                    case ParameterKey::license_type: {
                        auto it = LicenseTypeStringMap.find( mLicenseType );
                        if ( it == LicenseTypeStringMap.end() )
                            Unreachable( "Missing parameter key: license_type" );
                        std::string license_type_str = it->second;
                        json_value[key_str] = license_type_str;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", license_type_str );
                        break;
                    }
                    case ParameterKey::license_duration: {
                        json_value[key_str] = mLicenseDuration;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mLicenseDuration );
                        break;
                    }
                    case ParameterKey::num_activators: {
                        uint32_t nbActivators = 0;
                        getNumActivator(nbActivators);
                        json_value[key_str] = nbActivators;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", nbActivators );
                        break;
                    }
                    case ParameterKey::session_id: {
                        json_value[key_str] = mSessionID;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mSessionID );
                        break;
                    }
                    case ParameterKey::session_status: {
                        json_value[key_str] = isSessionRunning();
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", json_value[key_str] );
                        break;
                    }
                    case ParameterKey::metered_data: {
#if ((JSONCPP_VERSION_MAJOR ) >= 1 and ((JSONCPP_VERSION_MINOR) > 7 or ((JSONCPP_VERSION_MINOR) == 7 and JSONCPP_VERSION_PATCH >= 5)))
                        uint64_t metered_data = getMeteringData();
#else
                        // No "int64_t" support with JsonCpp < 1.7.5
                        unsigned long long metered_data = getMeteringData();
#endif
                        json_value[key_str] = metered_data;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", metered_data );
                        break;
                    }
                    case ParameterKey::nodelocked_request_file: {
                        if ( mLicenseType != LicenseType::NODE_LOCKED ) {
                            json_value[key_str] = std::string("Not applicable");
                            Warning( "Parameter only available with Node-Locked licensing" );
                        } else {
                            json_value[key_str] = mNodeLockRequestFilePath;
                            Debug( "Get value of parameter '", key_str, "' (ID=", key_id,
                                  "): Node-locked license request file is saved in ", mNodeLockRequestFilePath );
                        }
                        break;
                    }
                    case ParameterKey::page_ctrlreg:
                    case ParameterKey::page_vlnvfile:
                    case ParameterKey::page_licfile:
                    case ParameterKey::page_tracefile:
                    case ParameterKey::page_meteringfile:
                    case ParameterKey::page_mailbox: {
                        std::string str = getDrmPage(key_id - ParameterKey::page_ctrlreg);
                        json_value[key_str] = str;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, ")" );
                        Info(str);
                        break;
                    }
                    case ParameterKey::hw_report: {
                        std::string str = getDrmReport();
                        json_value[key_str] = str;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, ")" );
                        Info( "Print HW report:\n", str );
                        break;
                    }
                    case ParameterKey::drm_frequency: {
                        json_value[key_str] = mFrequencyCurr;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mFrequencyCurr );
                        break;
                    }
                    case ParameterKey ::frequency_detection_threshold: {
                        json_value[key_str] = mFrequencyDetectionThreshold;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mFrequencyDetectionThreshold );
                        break;
                    }
                    case ParameterKey ::frequency_detection_period: {
                        json_value[key_str] = mFrequencyDetectionPeriod;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mFrequencyDetectionPeriod );
                        break;
                    }
                    case ParameterKey ::product_id: {
                        json_value[key_str] = mHeaderJsonRequest["product"];
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mHeaderJsonRequest["product"].toStyledString() );
                        break;
                    }
                    case ParameterKey ::mailbox_size: {
                        uint32_t mbSize = getMailboxSize() - (uint32_t)MailboxOffset::MB_USER;
                        json_value[key_str] = mbSize;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mbSize );
                        break;
                    }
                    case ParameterKey ::list_all: {
                        json_value[key_str] = list_parameter_key();
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", json_value[key_str].toStyledString() );
                        break;
                    }
                    case ParameterKey ::dump_all: {
                        json_value[key_str] = dump_parameter_key();
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", json_value[key_str].toStyledString() );
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = readMailbox( MailboxOffset::MB_CUSTOM_FIELD );
                        json_value[key_str] = customField;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", customField );
                        break;
                    }
                    case ParameterKey ::mailbox_data: {
                        uint32_t mbSize = getMailboxSize() - (uint32_t)MailboxOffset::MB_USER;
                        std::vector<uint32_t> data_array = readMailbox( MailboxOffset::MB_USER, mbSize );
                        for( const auto& val: data_array )
                            json_value[key_str].append(val);
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", json_value[key_str].toStyledString() );
                        break;
                    }
                    case ParameterKey ::retry_deadline: {
                        json_value[key_str] = mRetryDeadline;
                        Debug( "Get value of parameter '", key_str, "' (ID=", key_id, "): ", mRetryDeadline );
                        break;
                    }
                    default: {
                        Throw( DRM_BadArg, "Cannot find parameter with ID: ", key_id );
                        break;
                    }
                }
            }
        CATCH_AND_THROW
        Debug("Get parameter request output: ", json_value.toStyledString());
    }

    void get( std::string& json_string ) const {
        TRY
            Json::Value root;
            Json::Reader reader;
            if ( !reader.parse( json_string, root ) )
                Throw(DRM_BadFormat, "Cannot parse JSON string argument provided to 'get' function:\n",
                        json_string, "\nBecause: ", reader.getFormattedErrorMessages());
            get(root);
            Json::StyledWriter json_writer;
            json_string = json_writer.write(root);
        CATCH_AND_THROW
    }

    template<typename T> T get( const ParameterKey /*key_id*/ ) const {}

    void set( const Json::Value& json_value ) {
        TRY
            Debug("Set parameter request: ", json_value.toStyledString());
            for( Json::ValueConstIterator it = json_value.begin() ; it != json_value.end() ; it++ ) {
                std::string key_str = it.key().asString();
                const ParameterKey key_id = findParameterKey( key_str );
                switch( key_id ) {
                    case ParameterKey ::frequency_detection_threshold: {
                        mFrequencyDetectionThreshold = (*it).asDouble();
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to value: ", mFrequencyDetectionThreshold );
                        break;
                    }
                    case ParameterKey ::frequency_detection_period: {
                        mFrequencyDetectionPeriod = (*it).asUInt();
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to value: ", mFrequencyDetectionPeriod );
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = (*it).asUInt();
                        writeMailbox( MailboxOffset::MB_CUSTOM_FIELD, customField );
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to value: ", customField );
                        break;
                    }
                    case ParameterKey ::mailbox_data: {
                        if ( !(*it).isArray() )
                            Throw( DRM_BadUsage, "Value must be an array of integer values" );
                        std::vector<uint32_t> data_array;
                        for( Json::ValueConstIterator itr = (*it).begin(); itr != (*it).end(); itr++ )
                            data_array.push_back( (*itr).asUInt() );
                        writeMailbox( MailboxOffset::MB_USER, data_array );
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to value: ", (*it).toStyledString());
                        break;
                    }
                    case ParameterKey ::retry_deadline: {
                        mRetryDeadline = (*it).asUInt();
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to value: ", mRetryDeadline );
                        break;
                    }
                    case ParameterKey::trigger_async_callback: {
                        std::string custom_msg = (*it).asString();
                        Exception e(DRM_Debug, custom_msg);
                        f_asynch_error( e.what() );
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to value: ", custom_msg );
                        break;
                    }
                    case ParameterKey::bad_authentication_token: {
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to random value" );
                        getDrmWSClient().useBadOAuth2Token( "BadToken" );
                        break;
                    }
                    case ParameterKey::bad_product_id: {
                        Debug( "Set parameter '", key_str, "' (ID=", key_id, ") to random value" );
                        mHeaderJsonRequest["product"]["name"] = "BAD_NAME_JUST_FOR_TEST";
                        break;
                    }
                    default:
                        Throw(DRM_BadArg, "Parameter '", key_str, "' cannot be overwritten");
                }
            }
        CATCH_AND_THROW
    }

    void set( const std::string& json_string ) {
        TRY
            Json::Value root;
            Json::Reader reader;
            if ( !reader.parse( json_string, root ) )
                Throw(DRM_BadFormat, "Cannot parse JSON string argument provided to 'set' function:\n",
                        json_string, "\nBecause: ", reader.getFormattedErrorMessages());
            set(root);
        CATCH_AND_THROW
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

template<> Json::Value DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value;
}

template<> std::string DrmManager::Impl::get( const ParameterKey key_id ) const {
    IMPL_GET_BODY
    return json_value[key_str].asString();
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


template<> void DrmManager::Impl::set( const ParameterKey key_id, const Json::Value& value ) {
    IMPL_SET_BODY
}

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
    json_value[key_str] = Json::Int64(value);
    set( json_value );
}

template<> void DrmManager::Impl::set( const ParameterKey key_id, const uint64_t& value ) {
    Json::Value json_value;
    std::string key_str = findParameterString( key_id );
    json_value[key_str] = Json::UInt64(value);
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
    : pImpl(new Impl(conf_file_path, cred_file_path, read_register, write_register, async_error)) {
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

template<> Json::Value DrmManager::get( const ParameterKey key ) const { return pImpl->get<Json::Value>( key ); }
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

template<> void DrmManager::set( const ParameterKey key, const Json::Value& value ) { pImpl->set<Json::Value>( key, value ); }
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
