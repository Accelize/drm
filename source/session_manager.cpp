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
#include <cstddef>
#include <memory>
#include <jsoncpp/json/json.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <future>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <typeinfo>
#include <sys/types.h>
#include <sys/stat.h>


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
        mLastErrorMsg = e.what(); \
        throw; \
    }


namespace Accelize {
namespace DRM {


const char* getApiVersion() {
    return DRMLIB_VERSION;
}


class DRM_LOCAL DrmManager::Impl {

protected:
    //helper typedef
    typedef std::chrono::steady_clock Clock; // use steady clock to be monotonic (were are not affected by clock adjustements)

    enum class LicenseType: uint8_t {OTHERS=0, NODE_LOCKED};

    std::map<LicenseType, std::string> mLicenseTypeString = {
            {LicenseType::OTHERS     , "Floating/Metering"},
            {LicenseType::NODE_LOCKED, "Node-Locked"}
    };

#ifdef _WIN32
    const char path_sep = '\\';
# else
    const char path_sep = '/';
#endif

    std::string mLastErrorMsg;

    //composition
    std::unique_ptr<DrmWSClient> mWsClient;
    std::unique_ptr<DrmControllerLibrary::DrmControllerOperations> mDrmController;
    std::mutex mDrmControllerMutex;

    //function callbacks
    DrmManager::ReadRegisterCallback f_read_register;
    DrmManager::WriteRegisterCallback f_write_register;
    DrmManager::AsynchErrorCallback f_asynch_error;

    // Parameters
    std::string mConfFilePath;
    std::string mCredFilePath;
    std::chrono::seconds minimum_license_duration;

    // Node-Locked parameters
    std::string mNodeLockLicenseDirPath;
    std::string mNodeLockRequestFilePath;
    std::string mNodeLockLicenseFilePath;

    // License type
    LicenseType mLicenseType = LicenseType::OTHERS;

    // Design param
    double mDrmFrequency = 125.0;

    //state
    // of licenses on DRM controller

    Clock::duration next_license_duration = Clock::duration::zero();
    bool next_license_duration_exact=false;
    Clock::duration current_license_duration = Clock::duration::zero();
    bool current_license_duration_exact=false;
    Clock::time_point sync_license_timepoint;
    bool sync_license_timepoint_exact=false;

    //session state
    bool sessionStarted{false};
    std::string mSessionID;
    std::string mUDID;
    std::string mBoardType;

    Json::Value mHeaderJsonRequest;

    // thread to maintain alive
    std::future<void> threadKeepAlive;
    std::mutex threadKeepAlive_mtx;
    std::condition_variable threadKeepAlive_cv;
    bool threadKeepAlive_stop{false};

    const std::map<ParameterKey, std::string> mParameterKeyMap = {
    #   define PARAMETERKEY_ITEM(id) {id, #id},
    #   include "accelize/drm/ParameterKey.def"
    #   undef PARAMETERKEY_ITEM
        {ParameterKeyCount, "ParameterKeyCount"}
    };


    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path )
    {
        Json::Reader reader;
        Json::Value conf_json;

        mConfFilePath = conf_file_path;
        mCredFilePath = cred_file_path;

        parse_configuration(conf_file_path, reader, conf_json);

        // Design configuration
        Json::Value conf_design = JVgetOptional(conf_json, "design", Json::objectValue);
        if (!conf_design.empty()) {
            mUDID = JVgetOptional(conf_design, "udid", Json::stringValue, "").asString();
            mBoardType = JVgetOptional(conf_design, "boardType", Json::stringValue, "").asString();
        }

        // DRM configuration
        Json::Value conf_drm = JVgetOptional(conf_json, "drm", Json::objectValue);
        if (!conf_drm.empty()) {
            // Get DRM license type
            Json::Value license_type_json = JVgetOptional(conf_drm, "mode", Json::stringValue);
            if ( !license_type_json.empty() && (license_type_json.asString() == "nodelocked") ){
                // If this is a node-locked license, get the license path
                Json::Value license_dir_json = JVgetOptional(conf_drm, "license_dir", Json::stringValue);
                if (license_dir_json.empty())
                    Throw(DRM_BadFormat, "Missing parameter 'license_dir' in crendential file ", cred_file_path);
                mNodeLockLicenseDirPath = license_dir_json.asString();
                mLicenseType = LicenseType::NODE_LOCKED;
                Debug("Detected Node-locked license");
                return;
            }
            // Get DRM frequency
            Json::Value drm_frequency_json = JVgetOptional(conf_drm, "frequency", Json::realValue);
            if ( !drm_frequency_json.isNull() ){
                mDrmFrequency = drm_frequency_json.asDouble();
            }
        }
        Debug("Detected floating/metered license");

        // Web Server
        mWsClient.reset(new DrmWSClient(conf_file_path, cred_file_path));
    }

    void init() {
        if (mDrmController)
            return;

        // create instance
        mDrmController.reset( new DrmControllerLibrary::DrmControllerOperations(
            std::bind(&DrmManager::Impl::drm_read_register, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&DrmManager::Impl::drm_write_register, this, std::placeholders::_1, std::placeholders::_2 )
        ));

        // Check compatibility of the DRM Version with Algodone version
        checkCompatibilityWithDRM();

        // Save header information
        mHeaderJsonRequest = getMeteringHeader();

        // If node-locked license is requested, create license request file
        if ( mLicenseType == LicenseType::NODE_LOCKED ) {
            // Check license directory exists
            if ( !dirExists( mNodeLockLicenseDirPath ) )
                Throw( DRM_BadUsage, "License directory path '", mNodeLockLicenseDirPath, "' specified in configuration file '", mConfFilePath, "' is not existing in the file system");
            // Create license request file
            create_nodelocked_license_request_file();
        }
    }

    bool dirExists( const std::string& dir_path ) {
        struct stat info;
        if ( stat( dir_path.c_str(), &info ) != 0 )
            return false;
        if(info.st_mode & S_IFDIR)
            return true;
        return false;
    }

    void parse_configuration(const std::string &file_path, Json::Reader &reader, Json::Value &json_value) {
        std::ifstream conf_fd(file_path);
        if (!conf_fd.good()) {
            Throw(DRM_BadUsage, "Cannot find JSON file: ", file_path);
        }
        if(!reader.parse(conf_fd, json_value))
            Throw(DRM_BadFormat, "Cannot parse ", file_path, " : ", reader.getFormattedErrorMessages());
        conf_fd.close();
    }

    DrmControllerLibrary::DrmControllerOperations& getDrmController() {
        Assert(mDrmController);
        return *mDrmController;
    }

    DrmWSClient& getDrmWSClient() {
        if(!mWsClient)
            Throw(DRM_BadUsage, "DRM Session Manager constructed with no WebService configuration");
        return *mWsClient;
    }

    static uint32_t drm_register_name_to_offset(const std::string& regName) {
        if(regName == "DrmPageRegister")
            return 0;
        if(regName.substr(0, 15) == "DrmRegisterLine")
            return std::stoi(regName.substr(15))*4+4;
        Unreachable();
    }

    unsigned int drm_read_register(const std::string& regName, unsigned int& value) {
        unsigned int ret = 0;
        if (!f_read_register)
            Unreachable();
        ret = f_read_register(drm_register_name_to_offset(regName), &value);
        if (ret != 0) {
            Error("Error in read register callback, errcode = ", ret);
            return -1;
        }
        Debug2("Read DRM register @", regName, " = 0x", std::hex, value, std::dec);
        return 0;
    }

    unsigned int drm_write_register(const std::string& regName, unsigned int value) {
        unsigned int ret = 0;
        if (!f_write_register)
            Unreachable();
        ret = f_write_register(drm_register_name_to_offset(regName), value);
        if(ret != 0) {
            Error("Error in write register callback, errcode = ", ret);
            return -1;
        }
        Debug2("Write DRM register @", regName, " = 0x", std::hex, value, std::dec);
        return 0;
    }

    void checkDRMCtlrRet(unsigned int errcode) {
        if(errcode)
            Throw(DRM_ExternFail, "Error in DRMCtlrLib function call : ", errcode);
    }

    // Check compatibility of the DRM Version with Algodone version
    void checkCompatibilityWithDRM() {
        std::string drmVersion;
        checkDRMCtlrRet( getDrmController().extractDrmVersion( drmVersion ) );
        uint32_t drmVersionNum = DrmControllerLibrary::DrmControllerDataConverter::hexStringToBinary(drmVersion)[0];
        std::string drmVersionDot = DrmControllerLibrary::DrmControllerDataConverter::binaryToVersionString(drmVersionNum);
        uint8_t drmMajor = (drmVersionNum >> 16) & 0xFF;
        uint8_t drmMinor = (drmVersionNum >> 8) & 0xFF;
        if (drmMajor < RETROCOMPATIBLITY_LIMIT_MAJOR) {
            Throw(DRM_CtlrError, "This DRM Lib ", getApiVersion()," is not compatible with the DRM HDK version ", drmVersionDot,": To be compatible HDK version shall be > or equal to ", RETROCOMPATIBLITY_LIMIT_MAJOR,".",RETROCOMPATIBLITY_LIMIT_MINOR,".0");
        } else if (drmMinor < RETROCOMPATIBLITY_LIMIT_MINOR) {
            Throw(DRM_CtlrError, "This DRM Lib ", getApiVersion()," is not compatible with the DRM HDK version ", drmVersionDot,": To be compatible HDK version shall be > or equal to ", RETROCOMPATIBLITY_LIMIT_MAJOR,".",RETROCOMPATIBLITY_LIMIT_MINOR,".0");
        }
        Debug("DRM Version = ", drmVersionDot);
    }

    void checkSessionID(Json::Value license_json) {
        std::string ws_sessionID = license_json["metering"]["sessionId"].asString();
        if ( !mSessionID.empty() && (mSessionID != ws_sessionID) ) {
            Throw(DRM_Fatal, "Session ID mismatch: received '", ws_sessionID, "' from WS but expect '", mSessionID, "'");
        }
    }

    void getNumActivator( unsigned int& value ) {
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readNumberOfDetectedIpsStatusRegister(value) );
    }

    uint64_t getMeteringData() {
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;
        Debug2("Get metering data from session on DRM controller");
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().initialization( numberOfDetectedIps, saasChallenge, meteringFile ) );
        std::string meteringDataStr = meteringFile[2].substr(16, 16);
        errno = 0;
        uint64_t meteringData = strtoull(meteringDataStr.c_str(), NULL, 16);
        if (errno)
            Throw(DRM_BadUsage, "Could not convert string '", meteringDataStr, "' to unsigned long long.");
        return meteringData;
    }

    // Get common info
    void getDesignInfo(std::string &drmVersion,
                       std::string &dna,
                       std::vector<std::string> &vlnvFile) {
        uint32_t nbOfDetectedIps;
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet(getDrmController().extractDrmVersion( drmVersion ) );
        checkDRMCtlrRet(getDrmController().extractDna( dna ) );
        checkDRMCtlrRet(getDrmController().extractVlnvFile( nbOfDetectedIps, vlnvFile ) );
    }

    Json::Value getMeteringHeader() {
        Json::Value json_output;
        std::string drmVersion;
        std::string dna;
        std::vector<std::string> vlnvFile;

        // Application section
        if(mUDID.empty())
            Throw(DRM_BadUsage, "Please set udid in configuration");
        if(mBoardType.empty())
            Throw(DRM_BadUsage, "Please set boardType in configuration");
        json_output["udid"] = mUDID;
        json_output["boardType"] = mBoardType;
        json_output["mode"] = (uint8_t)mLicenseType;
        json_output["drm_frequency"] = mDrmFrequency;

        // Get information from DRM Controller
        getDesignInfo(drmVersion, dna, vlnvFile);

        // Put it in Json output val
        json_output["drmlibVersion"] = getApiVersion();
        json_output["lgdnVersion"] = drmVersion;
        json_output["dna"] = dna;
        for( unsigned int i=0; i<vlnvFile.size(); i++ ) {
            std::string i_str = std::to_string(i);
            json_output["vlnvFile"][i_str]["vendor"]  = std::string("x") + vlnvFile[i].substr(0,4);
            json_output["vlnvFile"][i_str]["library"] = std::string("x") + vlnvFile[i].substr(4,4);
            json_output["vlnvFile"][i_str]["name"]    = std::string("x") + vlnvFile[i].substr(8,4);
            json_output["vlnvFile"][i_str]["version"] = std::string("x") + vlnvFile[i].substr(12,4);
        }
        return json_output;
    }

    Json::Value getMeteringStart() {
        Json::Value json_output(mHeaderJsonRequest);
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug("Build web request to create new session");

        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().initialization( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "open";
        json_output["mode"] = (uint8_t)mLicenseType;
        return json_output;
    }

    Json::Value getMeteringWait(unsigned int timeOut) {
        Json::Value json_output(mHeaderJsonRequest);
        uint32_t numberOfDetectedIps;
        std::string saasChallenge;
        std::vector<std::string> meteringFile;

        Debug("Build web request to maintain current session");

        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().extractMeteringFile( timeOut, numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        mSessionID = meteringFile[0].substr(0, 16);
        json_output["sessionId"] = mSessionID;
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

        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().endSessionAndExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        mSessionID = meteringFile[0].substr(0, 16);
        json_output["sessionId"] = mSessionID;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "close";
        return json_output;
    }

    bool isReadyForNewLicense() {
        bool ret(false);
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readLicenseTimerInitLoadedStatusRegister(ret) );
        return !ret;
    }

    void setLicense(const Json::Value& license_json) {
        Debug("Installing next license on DRM controller");
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);

        // get DNA
        std::string dna;
        checkDRMCtlrRet( getDrmController().extractDna( dna ) );

        // Extract license and license timer from webservice response
        std::string license      = license_json["license"][dna]["key"].asString();
        std::string licenseTimer = license_json["license"][dna]["licenseTimer"].asString();

        if( license.empty() )
            Throw(DRM_WSRespError, "Malformed response from Accelize WebService : license key is empty");
        if ( (mLicenseType != LicenseType::NODE_LOCKED) && licenseTimer.empty() )
            Throw(DRM_WSRespError, "Malformed response from Accelize WebService : LicenseTimer is empty");

        // Activate
        bool activationDone = false;
        unsigned char activationErrorCode;
        checkDRMCtlrRet( getDrmController().activate( license, activationDone, activationErrorCode ) );
        if( activationErrorCode ) {
            Throw(DRM_CtlrError, "Failed to activate license on DRM controller, activationErr: 0x", std::hex, activationErrorCode, std::dec);
        }

        // Load license timer
        if (mLicenseType != LicenseType::NODE_LOCKED) {
            bool licenseTimerEnabled = false;
            checkDRMCtlrRet( getDrmController().loadLicenseTimerInit(licenseTimer, licenseTimerEnabled));
            if (!licenseTimerEnabled) { Throw(DRM_CtlrError,
                      "Failed to load license timer on DRM controller, licenseTimerEnabled: 0x",
                      std::hex, licenseTimerEnabled, std::dec);
            }

//            WarningAssertGreaterEqual(license_json["metering"]["timeoutSecond"].asUInt(), minimum_license_duration.count());
            next_license_duration = std::chrono::seconds(license_json["metering"]["timeoutSecond"].asUInt());
            next_license_duration_exact = true;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (getLogLevel() >= eLogLevel::DEBUG2) {
            Info("Print HW report:\n", get_drm_report());
        }
    }

    bool isLicense_no_lock() {
        bool isLicenseEmpty(false);
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readLicenseTimerCountEmptyStatusRegister(isLicenseEmpty) );
        return !isLicenseEmpty;
    }

    bool isSessionRunning() {
        bool sessionRunning(false);
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readSessionRunningStatusRegister(sessionRunning) );
        return sessionRunning;
    }

    static const size_t InitialFNV = 2166136261U;
    static const size_t FNVMultiple = 16777619;

    /* Fowler / Noll / Vo (FNV) Hash */
    std::string customHash( const std::string &s ) {
        uint64_t hash = InitialFNV;

        for(size_t i = 0; i < s.length(); i++)
        {
            hash = hash ^ (s[i]);       /* xor  the low 8 bits */
            hash = hash * FNVMultiple;  /* multiply by the magic number */
        }
        std::stringstream ss;
        ss << std::hex << hash;
        return ss.str();
    }

    std::string hashDesignInfo() {
        std::stringstream ss;
        std::string drmVersion;
        std::string dna;
        std::vector<std::string> vlnvFile;

        getDesignInfo(drmVersion, dna, vlnvFile);
        ss << dna;
        ss << drmVersion;
        for(unsigned int i=0; i<vlnvFile.size(); i++)
            ss << vlnvFile[i];

        return customHash( ss.str() );
    }

    void create_nodelocked_license_request_file() {
        Json::StyledWriter json_writer;
        Json::FastWriter json_writer2;
        // Build request for node-locked license
        Json::Value request_json = getMeteringStart();
        Debug("License request JSON: ", request_json.toStyledString());
        // Create hash name based on design info
        std::string designHash = hashDesignInfo();
        mNodeLockRequestFilePath = mNodeLockLicenseDirPath + path_sep + designHash + ".req";
        mNodeLockLicenseFilePath = mNodeLockLicenseDirPath + path_sep + designHash + ".lic";
        Debug("Creating hash name based on design info: ", designHash);
        // - Save license request to file
        std::ofstream ofs( mNodeLockRequestFilePath );
        ofs << json_writer.write( request_json );
        ofs.close();
        if (!ofs.good())
            Throw(DRM_ExternFail, "Unable to write node-locked license request to file: ", mNodeLockRequestFilePath);
        ofs.open( mNodeLockRequestFilePath + ".fastwriter.req" );
        ofs << json_writer2.write( request_json );
        ofs.close();
        Debug("License request file saved on: ", mNodeLockRequestFilePath);
    }

    void install_nodelocked_license() {
        Json::Value license_json;
        std::ifstream ifs;

        Debug("Looking for local node-locked license file: ", mNodeLockLicenseFilePath);

        ifs.open( mNodeLockLicenseFilePath );
        if ( ifs.good() ) {
            // A license file has already been installed locally; read its content
            ifs >> license_json;
            ifs.close();
            ifs.close();
            if ( !ifs.good() )
                Throw( DRM_ExternFail, "Cannot parse license file ", mNodeLockLicenseFilePath );
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
            ifs.close();
            if ( ifs.good() ) {
                Throw( DRM_ExternFail, "Failed to parse license request file ", mNodeLockRequestFilePath );
            }
            // - Send request to web service and receive the new license
            license_json = getDrmWSClient().getLicense( request_json );
            // - Save license to file
            std::ofstream ofs( mNodeLockLicenseFilePath );
            Json::StyledWriter json_writer;
            ofs << json_writer.write( license_json );
            ofs.close();
            if (!ofs.good())
                Throw(DRM_ExternFail, "Unable to write newly received node-locked license to file: ", mNodeLockLicenseFilePath);
            Debug("Requested and saved new node-locked license file: ", mNodeLockLicenseFilePath);
        }
        // Install the license
        setLicense( license_json );
        Info("Installed node-locked license successfully");
    }

    uint32_t get_custom_field() {
        uint32_t roSize, rwSize;
        std::vector<uint32_t> ro_data, rw_data;
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().readMailboxFileRegister( roSize, rwSize, ro_data, rw_data) );
        Debug("Read from Read/Write mailbox: rwSize=", rwSize, " roSize=", roSize);
        return rw_data[0];
    }

    void set_custom_field( uint32_t value ) {
        uint32_t rwSize;
        std::vector<uint32_t> rw_data;
        rw_data.push_back( value );
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        checkDRMCtlrRet( getDrmController().writeMailBoxFilePageRegister() );
        checkDRMCtlrRet( getDrmController().writeMailboxFileRegister( rw_data, rwSize) );
        Debug("Wrote to Read/Write mailbox: rwSize=", rwSize);
    }

    std::string get_drm_page( int page_index ) {
        std::stringstream ss;
        uint32_t value;
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        drm_write_register("DrmPageRegister", page_index);
        ss << "DRM Page " << page_index << " registry:\n";
        for(uint32_t r=0; r < NB_MAX_REGISTER; r++) {
            f_read_register(r*4, &value);
            ss << "\tRegister @0x" << std::setfill('0') << std::setw(2) << std::hex << (r*4) << ": ";
            ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
            ss << " (" << std::setfill(' ') << std::setw(10) << std::dec << value << ")\n";
        }
        return ss.str();
    }

    std::string get_drm_report() {
        std::stringstream ss;
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        getDrmController().printHwReport(ss);
        return ss.str();
    }

    template< class Clock, class Duration >
    bool thread_wait_until_is_stop(const std::chrono::time_point<Clock, Duration>& timeout_time) {
        std::unique_lock<std::mutex> lk(threadKeepAlive_mtx);
        return threadKeepAlive_cv.wait_until(lk, timeout_time, [this]{return threadKeepAlive_stop;});
    }

    template< class Rep, class Period >
    bool thread_wait_for_is_stop( const std::chrono::duration<Rep, Period>& rel_time ) {
        std::unique_lock<std::mutex> lk(threadKeepAlive_mtx);
        return threadKeepAlive_cv.wait_for(lk, rel_time, [this]{return threadKeepAlive_stop;});
    }

    bool thread_notwait_is_stop() {
        std::lock_guard<std::mutex> lk(threadKeepAlive_mtx);
        return threadKeepAlive_stop;
    }

    void thread_start() {
        if(threadKeepAlive.valid()) {
            Warning("Thread already started");
            return;
        }

        threadKeepAlive = std::async(std::launch::async, [this](){
            try{
                while(1) {
                    bool wbtd_expected_time_exact = false; // wbtd = wrong board type detection
                    Clock::time_point wbtd_expected_time;
                    bool waited_readiness = false;
                    Clock::time_point time_of_synchro;

                    if(next_license_duration_exact && sync_license_timepoint_exact) {
                        wbtd_expected_time = sync_license_timepoint + next_license_duration;
                        wbtd_expected_time_exact = true;
                    }

                    while(!isReadyForNewLicense()) {
                        waited_readiness = true;
                        if(thread_wait_for_is_stop(std::chrono::seconds(1)))
                            return;
                    }
                    if(waited_readiness)
                        time_of_synchro = Clock::now();

                    // Step the license model
                    {
                        if(waited_readiness) {
                            sync_license_timepoint = time_of_synchro;
                            sync_license_timepoint_exact = true;
                        } else {
                            sync_license_timepoint += current_license_duration;
                            sync_license_timepoint_exact = sync_license_timepoint_exact && current_license_duration_exact;
                        }
                        current_license_duration = next_license_duration;
                        current_license_duration_exact = next_license_duration_exact;
                    }

                    // Wrong board type detection
                    if(waited_readiness && wbtd_expected_time_exact) {
                        auto diff = wbtd_expected_time - time_of_synchro;
                        auto diff_abs = diff >= diff.zero() ? diff : -diff;
                        if( diff_abs > std::chrono::seconds(2) )
                            Warning("Wrong board type detection, please check your configuration");
                    }
                    if(thread_notwait_is_stop()) {
                        Debug2("It is actually stopped: exit thread");
                        return;
                    }

                    Json::Value request_json;
                    Debug("Get metering data from session on DRM controller");
                    try {
                        request_json = getMeteringWait(1);
                    } catch(const DrmControllerLibrary::DrmControllerTimeOutException& e) {
                        Unreachable(); //we have checked isReadyForNewLicense, should not block
                    }

                    //request_json["sessionId"] = mSessionID;

                    Json::Value license_json;

                    //Clock::time_point min_polling_deadline = sync_license_timepoint + current_license_duration - std::chrono::milliseconds(200); /* we can try polling until this deadline */
                    Clock::time_point min_polling_deadline = Clock::now()+std::chrono::seconds(180); /* temporary wrokaround required for floating mode */
                    unsigned int retry_index = 0;
                    Clock::time_point start_of_retry = Clock::now();
                    while(1) { /* Retry WS request */
                        Json::Value retrytime_json;

                        if(min_polling_deadline <= Clock::now()) {
                            Throw(DRM_WSError, "Failed to obtain license from WS on time, the protected IP may have been locked");
                        }

                        if(thread_notwait_is_stop())
                            return;

                        std::string retry_msg;
                        try {
                            license_json = getDrmWSClient().getLicense( request_json, min_polling_deadline );
                            break;
                        } catch(const Exception& e) {
                            if(e.getErrCode() != DRM_WSMayRetry)
                                throw;
                            else {
                                retrytime_json = Json::Value(10);
                                retry_msg = e.what();
                            }
                        }

                        retry_index++;
                        Clock::duration retry_wait_duration;
                        if ( !retrytime_json.empty() ) {
                            retry_wait_duration = std::chrono::seconds( retrytime_json.asUInt() );
                            min_polling_deadline += 2*retry_wait_duration;
                        } else {
                            if((Clock::now() - start_of_retry) <= std::chrono::seconds(60))
                                retry_wait_duration = std::chrono::seconds(2);
                            else
                                retry_wait_duration = std::chrono::seconds(30);
                        }
                        if((Clock::now() + retry_wait_duration + std::chrono::seconds(2)) >= min_polling_deadline)
                            retry_wait_duration = min_polling_deadline - Clock::now() - std::chrono::seconds(2);
                        if(retry_wait_duration <= decltype(retry_wait_duration)::zero())
                            retry_wait_duration = std::chrono::seconds(0);

                        Warning("Failed to obtain license from WS (", retry_msg, "), will retry in ", std::chrono::duration_cast<std::chrono::seconds>(retry_wait_duration).count(), " seconds");

                        if(thread_wait_for_is_stop(retry_wait_duration))
                            return;
                    }

                    setLicense(license_json);
                    checkSessionID(license_json);

                    unsigned int licenseTimeout = license_json["metering"]["timeoutSecond"].asUInt();
                    Info("Uploaded metering data for session ID ", mSessionID, " and set next license with duration of ", licenseTimeout, " seconds");

                    if(thread_notwait_is_stop())
                        return;
                }
            } catch(const std::exception& e) {
                Error(e.what());
                f_asynch_error(std::string(e.what()));
            }
        });
    }

    void thread_stop() {
        if(!threadKeepAlive.valid())
            return;
        {
            std::lock_guard<std::mutex> lk(threadKeepAlive_mtx);
            threadKeepAlive_stop = true;
        }
        threadKeepAlive_cv.notify_all();
        threadKeepAlive.get();
    }

    void start_session() {

        Info("Starting a new metering session...");

        // Build request message for new license
        Json::Value request_json = getMeteringStart();

        // Send request and receive new license
        Json::Value license_json = getDrmWSClient().getLicense(request_json);
        setLicense(license_json);

        // Save session ID locally and into web service request header for later request needs
        mSessionID = license_json["metering"]["sessionId"].asString();
        mHeaderJsonRequest["sessionId"] = mSessionID;

        //init license model
        sync_license_timepoint = Clock::now();
        sync_license_timepoint_exact = true;
        current_license_duration = decltype(current_license_duration)::zero();
        current_license_duration_exact = true;

        unsigned int licenseTimeout = license_json["metering"]["timeoutSecond"].asUInt();
        Info("Started new DRM session with session ID ", mSessionID, " and set first license with duration of ", licenseTimeout, " seconds");

        sessionStarted = true;
        threadKeepAlive_stop = false;
        thread_start();
    }

    void resume_session() {

        Info("Resuming DRM session...");

        if ( isReadyForNewLicense() ) {
            Json::Value request_json(mHeaderJsonRequest);
            // Get metering data
            request_json = getMeteringWait(1);

            // Send license request to web service
            Json::Value license_json = getDrmWSClient().getLicense( request_json );

            // Check session ID
            checkSessionID( license_json );

            // Install license on DRM controller
            setLicense( license_json );

            sync_license_timepoint = Clock::now();
            sync_license_timepoint_exact = false;
            current_license_duration_exact = false;

            unsigned int licenseTimeout = license_json["metering"]["timeoutSecond"].asUInt();
            Info("Resumed DRM session with session ID ", mSessionID, " and set first license with duration of ", licenseTimeout, " seconds");

        } else {
            next_license_duration = minimum_license_duration;
            next_license_duration_exact = false;
            sync_license_timepoint = Clock::now();
            sync_license_timepoint_exact = false;
            current_license_duration_exact = false;
        }

        sessionStarted = true;
        threadKeepAlive_stop = false;
        thread_start();
    }

    void stop_session() {

        if (mLicenseType == LicenseType::NODE_LOCKED){
            return;
        }

        if ( !isSessionRunning() ) {
            Debug("No session is currently running");
            return;
        }

        Info("Stopping DRM session...");
        thread_stop();

        // Get and send metering data to web service
        Json::Value request_json = getMeteringStop();

        // Send request and receive answer.
        Json::Value license_json = getDrmWSClient().getLicense( request_json );
        checkSessionID( license_json );
        Info("Stopped metering session with session ID ", mSessionID, " and uploaded last metering data");

        sessionStarted = false;
        mSessionID = std::string("");
    }

    void pause_session() {

        Info("Pausing DRM session...");
        thread_stop();

        sessionStarted = false;
    }

    ParameterKey findParameterKey( const std::string& key_string ) const {
        for (auto const& it : mParameterKeyMap) {
            if (key_string.compare(it.second) == 0)
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

    void dump_drm_hw_report(std::ostream& os) {
        std::lock_guard<std::mutex> lk(mDrmControllerMutex);
        getDrmController().printHwReport(os);
    }

public:
    Impl( const std::string& conf_file_path,
          const std::string& cred_file_path,
          ReadRegisterCallback f_user_read_register,
          WriteRegisterCallback f_user_write_register,
          AsynchErrorCallback f_user_asynch_error )
        : Impl(conf_file_path, cred_file_path)
    {
        f_read_register = f_user_read_register;
        f_write_register = f_user_write_register;
        f_asynch_error = f_user_asynch_error;
        init();
    }

    ~Impl() {
        thread_stop();
    }

    // Non copyable non movable as we create closure with "this"
    Impl( const Impl& ) = delete;
    Impl( Impl&& ) = delete;

    void activate( const bool& resume_session_request = false ) {
        TRY
            Debug("Calling 'activate' with 'resume_session_request'=", resume_session_request);

            if (mLicenseType == LicenseType::NODE_LOCKED) {
                install_nodelocked_license();
                return;
            }

            if ( isSessionRunning() && !resume_session_request ) {
                stop_session();
            }
            if ( isSessionRunning() )
                resume_session();
            else
                start_session();

        CATCH_AND_THROW
    }

    void deactivate( const bool& pause_session_request = false ) {
        TRY
            Debug("Calling 'deactivate' with 'pause_session_request'=", pause_session_request);

            if (mLicenseType == LicenseType::NODE_LOCKED) {
                return;
            }

            if ( pause_session_request )
                pause_session();
            else
                stop_session();

        CATCH_AND_THROW
    }

    void get( Json::Value& json_value ) {
        TRY
            Debug("Get parameter request input: ", json_value.toStyledString());
            for (const std::string& key_str : json_value.getMemberNames()) {
                const ParameterKey key_id = findParameterKey( key_str );
                switch(key_id) {
                    case ParameterKey::license_type: {
                        std::string license_type_str = mLicenseTypeString[ mLicenseType ];
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, "): ", license_type_str);
                        json_value[key_str] = license_type_str;
                        break;
                    }
                    case ParameterKey::num_activators: {
                        uint32_t nbActivators = 0;
                        getNumActivator(nbActivators);
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, "): ", nbActivators);
                        json_value[key_str] = nbActivators;
                        break;
                    }
                    case ParameterKey::session_id: {
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, "): ", mSessionID);
                        json_value[key_str] = mSessionID;
                        break;
                    }
                    case ParameterKey::metering_data: {
                        unsigned long long metering_data = getMeteringData();
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, "): ", metering_data);
                        json_value[key_str] = metering_data;
                        break;
                    }
                    case ParameterKey::strerror: {
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, "): ", mLastErrorMsg);
                        json_value[key_str] = mLastErrorMsg;
                        mLastErrorMsg.clear();
                        break;
                    }
                    case ParameterKey::nodelocked_request_file: {
                        if ( mLicenseType != LicenseType::NODE_LOCKED ) {
                            Warning("Parameter only available with Node-Locked licensing");
                            json_value[key_str] = std::string("Not applicable");
                        } else {
                            Debug("Get value of parameter '", key_str, "' (ID=", key_id,
                                  "): Node-locked license request file is saved in ", mNodeLockRequestFilePath);
                            json_value[key_str] = mNodeLockRequestFilePath;
                        }
                        break;
                    }
                    case ParameterKey::page_ctrlreg:
                    case ParameterKey::page_vlnvfile:
                    case ParameterKey::page_licfile:
                    case ParameterKey::page_tracefile:
                    case ParameterKey::page_meteringfile:
                    case ParameterKey::page_mailbox: {
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, ")");
                        std::string str = get_drm_page(key_id - ParameterKey::page_ctrlreg);
                        json_value[key_str] = str;
                        Info(str);
                        break;
                    }
                    case ParameterKey::hw_report: {
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, ")");
                        std::string str = get_drm_report();
                        json_value[key_str] = str;
                        Info("Print HW report:\n", str);
                        break;
                    }
                    case ParameterKey::custom_field: {
                        uint32_t customField = get_custom_field();
                        Debug("Get value of parameter '", key_str, "' (ID=", key_id, "): ", customField);
                        json_value[key_str] = customField;
                        break;
                    }
                    default: {
                        Throw(DRM_BadArg, "Cannot find parameter with ID: ", key_id);
                        break;
                    }
                }
            }
        CATCH_AND_THROW
        Debug("Get parameter request output: ", json_value.toStyledString());
    }

    void get( std::string& json_string ) {
        TRY
            Json::Value root;
            Json::Reader reader;
            if ( !reader.parse( json_string, root ) )
                Throw(DRM_BadFormat, "Cannot parse JSON string: ", reader.getFormattedErrorMessages());
            get(root);
            Json::FastWriter fastWriter;
            json_string = fastWriter.write(root);
        CATCH_AND_THROW
    }

    template<typename T> T get( const ParameterKey /*key_id*/ ) { return nullptr; }

    void set( const Json::Value& json_value ) {
        TRY
            Debug("Set parameter request: ", json_value.toStyledString());
            for( Json::Value::const_iterator it = json_value.begin() ; it != json_value.end() ; it++ ) {
                std::string key_str = it.key().asString();
                const ParameterKey key_id = findParameterKey( key_str );
                switch( key_id ) {
                    case custom_field: {
                        uint32_t customField = (*it).asUInt();
                        set_custom_field( customField );
                        Debug("Set parameter '", key_str, "' (ID=", key_id, ") to value: ", customField);
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
                Throw(DRM_BadFormat, "Cannot parse JSON string: ", reader.getFormattedErrorMessages());
            set(root);
        CATCH_AND_THROW
    }

    template<typename T> void set( const ParameterKey key_id, const T& value ) {}

};

/*************************************/
// DrmManager::Impl class definition
/*************************************/

#define IMPL_GET_BODY \
    Json::Value json_value; \
    std::string key_str = findParameterString( key_id ); \
    json_value[key_str] = Json::nullValue; \
    get( json_value );

template<> Json::Value DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value;
}

template<> std::string DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asString();
}

template<> bool DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asBool();
}

template<> int32_t DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asInt();
}

template<> uint32_t DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asUInt();
}

template<> int64_t DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asInt64();
}

template<> uint64_t DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asUInt64();
}

template<> float DrmManager::Impl::get( const ParameterKey key_id ) {
    IMPL_GET_BODY
    return json_value[key_str].asFloat();
}

template<> double DrmManager::Impl::get( const ParameterKey key_id ) {
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
    pImpl->deactivate();
    delete pImpl;
    pImpl = nullptr;
}


void DrmManager::activate( const bool& resume_session ) {
    pImpl->activate( resume_session );
}

void DrmManager::deactivate( const bool& pause_session ) {
    pImpl->deactivate( pause_session );
}

void DrmManager::get( Json::Value& json_value ) {
    pImpl->get( json_value );
}

void DrmManager::get( std::string& json_string ) {
    pImpl->get( json_string );
}

template<typename T> T DrmManager::get( const ParameterKey key ) {
    return pImpl->get<T>( key );
}

template<> Json::Value DrmManager::get( const ParameterKey key ) { return pImpl->get<Json::Value>( key ); }
template<> std::string DrmManager::get( const ParameterKey key ) { return pImpl->get<std::string>( key ); }
template<> bool DrmManager::get( const ParameterKey key ) { return pImpl->get<bool>( key ); }
template<> int32_t DrmManager::get( const ParameterKey key ) { return pImpl->get<int32_t>( key ); }
template<> uint32_t DrmManager::get( const ParameterKey key ) { return pImpl->get<uint32_t>( key ); }
template<> int64_t DrmManager::get( const ParameterKey key ) { return pImpl->get<int64_t>( key ); }
template<> uint64_t DrmManager::get( const ParameterKey key ) { return pImpl->get<uint64_t>( key ); }
template<> float DrmManager::get( const ParameterKey key ) { return pImpl->get<float>( key ); }
template<> double DrmManager::get( const ParameterKey key ) { return pImpl->get<double>( key ); }

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
