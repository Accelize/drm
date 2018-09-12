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

#include "accelize/drm/session_manager.h"
#include "ws_client.h"
#include "log.h"
#include "utils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "HAL/DrmControllerOperations.hpp"
#pragma GCC diagnostic pop

namespace Accelize {
namespace DRMLib {

class MeteringSessionManager::Impl {
protected:
    //compisition
    std::unique_ptr<MeteringWSClient> mWsClient;
    std::unique_ptr<DrmControllerLibrary::DrmControllerOperations> mDrmController;

    //params
    MeteringSessionManager::ReadReg32ByOffsetHandler f_read32_offset;
    MeteringSessionManager::WriteReg32ByOffsetHandler f_write32_offset;
    MeteringSessionManager::ReadReg32ByNameHandler f_read32_name;
    MeteringSessionManager::WriteReg32ByNameHandler f_write32_name;
    MeteringSessionManager::ErrorCallBackHandler f_error_cb;

    //state
    // queue with licenses deadlines
    // use steady clock to be monotonic (were are not affected by clock adjustements)
    // Front is end of current license, Back is last license timepoint (after this point, no license are in use = session is dead)
    typedef std::chrono::steady_clock Clock;
    std::deque<std::chrono::time_point<Clock>> license_end_timepoint;
    const unsigned int max_number_licenses = 2;

    //session state
    bool sessionStarted{false};
    std::string sessionID;
    std::string udid;
    std::string boardType;

    // thread to maintain alive
    std::future<void> threadKeepAlive;
    std::mutex threadKeepAlive_mtx;
    std::condition_variable threadKeepAlive_cv;
    bool threadKeepAlive_stop{false};

    Impl(ErrorCallBackHandler f_error_cb)
        : f_error_cb(f_error_cb)
    {
    }

    Impl(const std::string& conf_file_path, const std::string& cred_file_path, ErrorCallBackHandler f_error_cb)
        : f_error_cb(f_error_cb)
    {
        Json::Reader reader;

        Json::Value conf_json;
        std::ifstream conf_fd(conf_file_path);
        if(!reader.parse(conf_fd, conf_json))
            Throw(DRMBadFormat, "Cannot parse ", conf_file_path, " : ", reader.getFormattedErrorMessages());
        Json::Value conf_server_json = JVgetRequired(conf_json, "webservice", Json::objectValue);

        Json::Value conf_design = JVgetOptional(conf_json, "design", Json::objectValue);
        if(!conf_design.isNull()) {
            udid = JVgetOptional(conf_design, "udid", Json::stringValue, "").asString();
            boardType = JVgetOptional(conf_design, "boardType", Json::stringValue, "").asString();
        }

        Json::Value credentials;
        std::ifstream cred_fd(cred_file_path);
        if(!reader.parse(cred_fd, credentials))
            Throw(DRMBadFormat, "Cannot parse ", cred_file_path, " : ", reader.getFormattedErrorMessages());

        mWsClient.reset(new MeteringWSClient(conf_server_json, credentials));
    }

    DrmControllerLibrary::DrmControllerOperations& getDrmController() {
        Assert(mDrmController);
        return *mDrmController;
    }

    MeteringWSClient& getMeteringWSClient() {
        if(!mWsClient)
            Throw(DRMBadUsage, "Metering Session Manager constructed with no WebService configuration");
        return *mWsClient;
    }

    static uint32_t drm_register_name_to_offset(const std::string& regName) {
        if(regName == "DrmPageRegister") return 0;
        if(regName.substr(0, 15) == "DrmRegisterLine")
            return std::stoi(regName.substr(15))*4+4;
        Unreachable();
    }

    unsigned int drm_read_register(const std::string& regName, unsigned int& value) {
        unsigned int ret = 0;
        if(f_read32_name) {
            ret = f_read32_name(regName, &value);
        } else if(f_read32_offset) {
            ret = f_read32_offset(drm_register_name_to_offset(regName), &value);
        } else {
            Unreachable();
        }
        if(ret != 0) {
            Error("Error in read register callback, errcode = ", ret);
            return -1;
        } else {
            return 0;
        }
    }

    unsigned int drm_write_register(const std::string& regName, unsigned int value) {
        unsigned int ret = 0;
        if(f_write32_name) {
            ret = f_write32_name(regName, value);
        } else if(f_write32_offset) {
            ret = f_write32_offset(drm_register_name_to_offset(regName), value);
        } else {
            Unreachable();
        }
        if(ret != 0) {
            Error("Error in write register callback, errcode = ", ret);
            return -1;
        } else {
            return 0;
        }
    }

    void checkDRMCtlrRet(unsigned int errcode) {
        if(errcode)
            Throw(DRMExternFail, "Error in DRMCtlrLib function call : ", errcode);
    }

    void getMeteringHead(Json::Value& json_output) {
        unsigned int             nbOfDetectedIps;
        std::string              drmVersion, dna;
        std::vector<std::string> vlnvFile;

        // Get common info
        checkDRMCtlrRet( getDrmController().extractDrmVersion( drmVersion ) );
        checkDRMCtlrRet( getDrmController().extractDna( dna ) );
        checkDRMCtlrRet( getDrmController().extractVlnvFile( nbOfDetectedIps, vlnvFile ) );

        // Put it in Json output val
        json_output["lgdnVersion"] = drmVersion;
        json_output["dna"]         = dna;
        for(unsigned int i=0; i<vlnvFile.size(); i++) {
            std::string i_str = std::to_string(i);
            json_output["vlnvFile"][i_str]["vendor"]  = std::string("x") + vlnvFile[i].substr(0,4);
            json_output["vlnvFile"][i_str]["library"] = std::string("x") + vlnvFile[i].substr(4,4);
            json_output["vlnvFile"][i_str]["name"]    = std::string("x") + vlnvFile[i].substr(8,4);
            json_output["vlnvFile"][i_str]["version"] = std::string("x") + vlnvFile[i].substr(12,4);
        }

        // Application section
        if(udid.empty())
            Throw(DRMBadUsage, "Please set udid in configuration");
        if(boardType.empty())
            Throw(DRMBadUsage, "Please set boardType in configuration");
        json_output["udid"]      = udid;
        json_output["boardType"] = boardType;
    }

    void getMeteringStart(Json::Value& json_output) {
        unsigned int             numberOfDetectedIps;
        std::string              saasChallenge;
        std::vector<std::string> meteringFile;
        checkDRMCtlrRet( getDrmController().initialization( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "open";
    }

    void getMeteringStop(Json::Value& json_output) {
        unsigned int             numberOfDetectedIps;
        std::string              saasChallenge;
        std::vector<std::string> meteringFile;
		checkDRMCtlrRet( getDrmController().endSessionAndExtractMeteringFile( numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "close";
    }

    void getMeteringWait(unsigned int timeOut, Json::Value& json_output) {
        unsigned int             numberOfDetectedIps;
        std::string              saasChallenge;
        std::vector<std::string> meteringFile;
        checkDRMCtlrRet( getDrmController().extractMeteringFile( timeOut, numberOfDetectedIps, saasChallenge, meteringFile ) );
        json_output["saasChallenge"] = saasChallenge;
        json_output["meteringFile"]  = std::accumulate(meteringFile.begin(), meteringFile.end(), std::string(""));
        json_output["request"] = "running";
    }

    bool isReadyForNewLicense() {
        bool ret;
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        checkDRMCtlrRet( getDrmController().readStatusRegister(DrmControllerLibrary::mDrmStatusLicenseTimerInitLoaded,
                                                      DrmControllerLibrary::mDrmStatusMaskLicenseTimerInitLoaded,
                                                      ret));
        return !ret;
    }

    void setLicense(const Json::Value& json_license) {
        Debug("Installing next license on DRM controller");

        // get DNA
        std::string dna;
        checkDRMCtlrRet( getDrmController().extractDna( dna ) );

        // Get license/license timer from webservice response
        std::string license      = json_license["license"][dna]["key"].asString();
        std::string licenseTimer = json_license["license"][dna]["licenseTimer"].asString();

        if( license.empty() )
            Throw(DRMWSRespError, "Malformed response from Accelize WebService : license key is empty");
        if( licenseTimer.empty() )
            Throw(DRMWSRespError, "Malformed response from Accelize WebService : LicenseTimer is empty");

        // Activate
        bool activationDone = false;
        unsigned char activationErrorCode;
        checkDRMCtlrRet( getDrmController().activate( license, activationDone, activationErrorCode ) );
        if( activationErrorCode ) {
            Throw(DRMCtlrError, "Failed to activate license on DRM controller, activationErr: 0x", std::hex, activationErrorCode);
        }

        // Load license timer
        bool licenseTimerEnabled = false;
        checkDRMCtlrRet( getDrmController().loadLicenseTimerInit( licenseTimer, licenseTimerEnabled ) );
        if ( !licenseTimerEnabled ) {
            Throw(DRMCtlrError, "Failed to load license timer on DRM controller, licenseTimerEnabled: 0x", std::hex, licenseTimerEnabled);
        }

        if(license_end_timepoint.empty())
            license_end_timepoint.push_back(Clock::now() + std::chrono::seconds(json_license["metering"]["timeoutSecond"].asUInt()));
        else
            license_end_timepoint.push_back(license_end_timepoint.back() + std::chrono::seconds(json_license["metering"]["timeoutSecond"].asUInt()));

        dump_drm_hw_report();
    }

    void clearLicense() {
        static const std::string nullLicense(352, '0');
        checkDRMCtlrRet( getDrmController().writeLicenseFilePageRegister() );
        checkDRMCtlrRet( getDrmController().writeLicenseFileRegister(nullLicense) );
        Assert(!isLicense());
    }

    bool isLicense() {
        checkDRMCtlrRet( getDrmController().writeRegistersPageRegister() );
        unsigned char err;
        checkDRMCtlrRet( getDrmController().readErrorRegister( DrmControllerLibrary::mDrmActivationErrorPosition,
                                                         DrmControllerLibrary::mDrmActivationErrorMask,
                                                         err));
        return (err == DrmControllerLibrary::mDrmErrorNoError);
    }

    void dump_drm_hw_report() {
        std::stringstream ss;
        getDrmController().printHwReport(ss);
        Debug("Dump DRM controller : ", ss.str());
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
        if(threadKeepAlive.valid())
            Error("Thread already started");

        threadKeepAlive = std::async(std::launch::async, [this](){
            try{
                while(1) {
                    bool wrong_board_type_detection = false;

                    while(!isReadyForNewLicense()) {
                        wrong_board_type_detection = true; //if we wait, we know when license expire, if we dont wait, the license might be expired from long time ago
                        if(thread_wait_for_is_stop(std::chrono::seconds(1)))
                            return;
                    }

                    if(thread_notwait_is_stop())
                        return;

                    Assert(license_end_timepoint.size()<=max_number_licenses);
                    if(license_end_timepoint.size()==max_number_licenses) {
                        if(wrong_board_type_detection) {
                            auto diff = license_end_timepoint.front()-Clock::now();
                            auto diff_abs = diff >= diff.zero() ? diff : -diff;
                            if( diff_abs > std::chrono::seconds(2) )
                                Warning("Wrong board type detection, please check your configuration");
                        }
                        license_end_timepoint.pop_front();
                    }

                    Json::Value json_req;
                    Debug("Get metering from session on DRM controller");
                    getMeteringHead(json_req);
                    try{
                        getMeteringWait(1, json_req);
                    } catch(const DrmControllerLibrary::DrmControllerTimeOutException& e) {
                        Unreachable(); //we have checked isReadyForNewLicense, should not block
                    }

                    json_req["sessionId"] = sessionID;
                    Json::Value json_license = getMeteringWSClient().getLicense(json_req);
                    setLicense(json_license);

                    if(!sessionID.empty() && sessionID!=json_license["metering"]["sessionId"].asString()) {
                        Error("sessionID has changed since begininning of sessions (old=", sessionID, " new=", json_license["metering"]["sessionId"].asString(),")");
                    }
                    sessionID = json_license["metering"]["sessionId"].asString();

                    unsigned int licenseTimeout = json_license["metering"]["timeoutSecond"].asUInt();
                    Info("Uploaded metering for sessionId ", sessionID, " and set next license with duration of ", std::dec, licenseTimeout, " seconds");

                    if(thread_notwait_is_stop())
                        return;
                }
            } catch(const std::exception& e) {
                Error(e.what());
                f_error_cb(std::string(e.what()));
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
        threadKeepAlive.wait();
    }

public:
    Impl(const std::string& conf_file_path, const std::string& cred_file_path, ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
        : Impl(conf_file_path, cred_file_path, f_error_cb)
    {
        f_read32_offset = f_drm_read32;
        f_write32_offset = f_drm_write32;
        init();
    }
    Impl(const std::string& conf_file_path, const std::string& cred_file_path, ReadReg32ByNameHandler f_drm_read32, WriteReg32ByNameHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
        : Impl(conf_file_path, cred_file_path, f_error_cb)
    {
        f_read32_name = f_drm_read32;
        f_write32_name = f_drm_write32;
        init();
    }
    Impl(ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
        : Impl(f_error_cb)
    {
        f_read32_offset = f_drm_read32;
        f_write32_offset = f_drm_write32;
        init();
    }
    Impl(ReadReg32ByNameHandler f_drm_read32, WriteReg32ByNameHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
        : Impl(f_error_cb)
    {
        f_read32_name = f_drm_read32;
        f_write32_name = f_drm_write32;
        init();
    }
    ~Impl() {
        thread_stop();
    }

    //non copyable non movable as we create closure with "this"
    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;

    void setUdid(const std::string& _udid){
        udid = _udid;
    }

    void setBoardType(const std::string& _boardType){
        boardType = _boardType;
    }

    void init() {
        if(mDrmController)
            return;
        // create instance
        mDrmController.reset( new DrmControllerLibrary::DrmControllerOperations(
            std::bind(&MeteringSessionManager::Impl::drm_read_register, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&MeteringSessionManager::Impl::drm_write_register, this, std::placeholders::_1, std::placeholders::_2 )
            ));
    }

    void auto_start_session() {
        // Detect if session was previously stopped
        if(!isLicense())
            start_session();
        else
            resume_session();
    }

    void start_session() {
        if(sessionStarted)
            Throw(DRMBadArg, "Error : session already started");

        Json::Value json_req;
        Info("Starting metering session...");
        getMeteringHead(json_req);
        getMeteringStart(json_req);

        Json::Value json_license = getMeteringWSClient().getLicense(json_req);
        setLicense(json_license);
        sessionID = json_license["metering"]["sessionId"].asString();

        unsigned int licenseTimeout = json_license["metering"]["timeoutSecond"].asUInt();
        Info("Started new metering session with sessionId ", sessionID, " and set first license with duration of ", std::dec, licenseTimeout, " seconds");

        sessionStarted = true;

        thread_start();
    }

    void stop_session() {
        Info("Stopping metering session...");
        thread_stop();

        Json::Value json_req;
        Debug("Get last metering data from session on DRM controller");
        getMeteringHead(json_req);
        getMeteringStop(json_req);
        json_req["sessionId"] = sessionID;
        getMeteringWSClient().getLicense(json_req);
        clearLicense();
        Info("Stopped metering session with sessionId ", sessionID, " and uploaded last metering data");

        sessionStarted = false;
    }

    void resume_session() {
        Info("Resuming metering session...");

        if(isReadyForNewLicense()) {
            Json::Value json_req;

            getMeteringHead(json_req);
            try{
                getMeteringWait(1, json_req);
            } catch(const DrmControllerLibrary::DrmControllerTimeOutException& e) {
                Unreachable(); //we have checked isReadyForNewLicense, should not block
            }

            json_req["sessionId"] = sessionID;
            Json::Value json_license = getMeteringWSClient().getLicense(json_req);
            setLicense(json_license);

            sessionID = json_license["metering"]["sessionId"].asString();
            unsigned int licenseTimeout = json_license["metering"]["timeoutSecond"].asUInt();
            Info("Resumed metering session with sessionId ", sessionID, " and set first license with duration of ", std::dec, licenseTimeout, " seconds");
        }

        sessionStarted = true;
        thread_start();
    }

    void pause_session() {
        Info("Pausing metering session...");
        thread_stop();

        sessionStarted = false;
    }

    void dump_drm_hw_report(std::ostream& os) {
        getDrmController().printHwReport(os);
    }
};

MeteringSessionManager::MeteringSessionManager(const std::string& conf_file_path, const std::string& cred_file_path, ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
    : pImpl(new Impl(conf_file_path, cred_file_path, f_drm_read32, f_drm_write32, f_error_cb))
{
}

MeteringSessionManager::MeteringSessionManager(const std::string& conf_file_path, const std::string& cred_file_path, ReadReg32ByNameHandler f_drm_read32, WriteReg32ByNameHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
    : pImpl(new Impl(conf_file_path, cred_file_path, f_drm_read32, f_drm_write32, f_error_cb))
{
}

MeteringSessionManager::MeteringSessionManager(ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
    : pImpl(new Impl(f_drm_read32, f_drm_write32, f_error_cb))
{
}

MeteringSessionManager::MeteringSessionManager(ReadReg32ByNameHandler f_drm_read32, WriteReg32ByNameHandler f_drm_write32, ErrorCallBackHandler f_error_cb)
    : pImpl(new Impl(f_drm_read32, f_drm_write32, f_error_cb))
{
}

MeteringSessionManager::~MeteringSessionManager() {
    delete pImpl;
}

void MeteringSessionManager::start_session() {
    pImpl->start_session();
}

void MeteringSessionManager::auto_start_session() {
    pImpl->auto_start_session();
}

void MeteringSessionManager::resume_session() {
    pImpl->resume_session();
}

void MeteringSessionManager::pause_session() {
    pImpl->pause_session();
}

void MeteringSessionManager::stop_session() {
    pImpl->stop_session();
}

void MeteringSessionManager::dump_drm_hw_report(std::ostream& os) {
    pImpl->dump_drm_hw_report(os);
}

void MeteringSessionManager::setUdid(const std::string& udid){
    pImpl->setUdid(udid);
}

void MeteringSessionManager::setBoardType(const std::string& boardType){
    pImpl->setBoardType(boardType);
}

}
}
