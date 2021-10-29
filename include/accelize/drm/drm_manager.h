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

/** \brief Accelize DRM C++ Library
*/

#ifndef _H_ACCELIZE_DRM_MANAGER
#define _H_ACCELIZE_DRM_MANAGER

#include <string>
#include <functional>
#include <json/json.h>
#include <cstring>

#include "accelize/drmc/common.h"
#include "accelize/drm/version.h"
#include "pnr/provencore.h"


//! Accelize interfaces and implementations
namespace Accelize {

//! DRM specific interface and implementation
namespace DRM {

/** Enum class listing the parameters accessible from User code.
 Some have ready only access while others have read and write access.
*/
enum ParameterKey {
    /// Include the Parameter ID
#   define PARAMETERKEY_ITEM(id) id,
#   include "accelize/drm/ParameterKey.def"
#   undef PARAMETERKEY_ITEM
    ParameterKeyCount
} DRM_EXPORT;


/** \brief Return API version.
*/
const char* getApiVersion() DRM_EXPORT;


/** \brief Manage Accelize DRM by handling communication with DRM controller IP
    and Accelize Web service.
*/
class DRM_EXPORT DrmManager {
private:
    class Impl; //!< Internal representation
    //std::unique_ptr<Impl> pImpl; //!< Internal representation
    Impl* pImpl; //!< Internal representation
    
    /* File descriptor attached to device /dev/trustzone */
    static pnc_session_t *s_pnc_session;

    /* Virtual address and size of the mapped [tzfd] file */
    static uint32_t *s_pnc_tzvaddr;
    static size_t s_pnc_tzsize;
    
    /* DRM Controller page offset in shared memory */
    static uint32_t s_pnc_page_offset;
    
    static const std::string DRM_SELF_TEST_ERROR_MESSAGE;
    static const std::string DRM_CONNECTION_ERROR_MESSAGE;                        
    static const std::string DRM_DOC_LINK;

    static const char* SDK_SLEEP_IN_MICRO_SECONDS;

public:

    /** \brief FPGA read register callback function.
        The register offset is relative to first register of DRM controller.

        \param[in] register_offset : Register offset relative to DRM controller
        IP base address.
        \param[in] returned_data : Pointer to an integer that will contain the
        value of the corresponding register.

        \warning This function must be thread-safe in case of concurrency on the
        register bus.
    */
    typedef std::function<int32_t/*errcode*/ (uint32_t /*register offset*/, uint32_t* /*returned data*/)> ReadRegisterCallback;

    /** \brief FPGA write register callback function.
        The register offset is relative to first register of DRM controller.

        \param[in] register_offset : Register offset relative to DRM controller
        IP base address.
        \param[in] data_to_write : Data to write in register.

        \warning This function must be thread-safe in case of concurrency on the
        register bus.
    */
    typedef std::function<int32_t/*errcode*/ (uint32_t /*register offset*/, uint32_t /*data to write*/)> WriteRegisterCallback;

    /** \brief Asynchronous Error handling callback function.
        This function is called in case of asynchronous error during operation.

        \param[in] error_message : Error message.
    */
    typedef std::function<void (const std::string&/*error message*/)> AsynchErrorCallback;

    DrmManager() = delete; //!< No default constructor

    /** \brief Instantiate and initialize a DRM manager.

        \see ReadRegisterCallback WriteRegisterCallback AsynchErrorCallback

        \param[in] conf_file_path : Path to the DRM configuration JSON file.
        \param[in] cred_file_path : Path to the user Accelize credential JSON file.
        \param[in] read_register : FPGA read register callback function.
        \param[in] write_register : FPGA write register callback function.
        \param[in] async_error : Asynchronous Error handling callback function.
    */
    DrmManager( const std::string& conf_file_path,
                const std::string& cred_file_path,
                ReadRegisterCallback read_register,
                WriteRegisterCallback write_register,
                AsynchErrorCallback async_error );

    DrmManager(const DrmManager&) = delete; //!< Non-copyable

    DrmManager(DrmManager&&); //!< Support move

    ~DrmManager(); //!< Destructor

    /** \brief Activate DRM session.

        This function activate/unlocks the hardware by unlocking the protected
        IPs in the FPGA and opening a DRM session.

        If a session is still pending the behavior depends on
        "resume_session_request" argument. If true the session is reused.
        Otherwise the session is closed and a new session is created.

        This function will start a thread that keeps the hardware unlocked by
        automatically updating the license when necessary.

        Any error during this thread execution will be reported asynchronously
        through the "async_error" function, if provided in constructor.

        When this function returns and the license is valid,
        the protected IPs are guaranteed to be unlocked.

        \param[in] resume_session_request : If true, the pending session is
        reused. If no pending session is found, create a new one. If
        false and a pending session is found, close it and create a new
        one. Default to False.
    */
    void activate( const bool& resume_session_request = false );

    /** \brief Deactivate DRM session.

        This function deactivates/locks the hardware back and close the session
        unless the "pause_session_request" argument is True. In this case,
        the session is kept opened for later use.

        This function will join the thread keeping the hardware unlocked.

        When the function returns, the hardware are guaranteed to be locked.

        \param[in] pause_session_request : If true, the current session is kept
        open for later usage. Otherwise, the current session is closed.
        Default to false.
    */
    void deactivate( const bool& pause_session_request = false );

    /** \brief Get information from the DRM system.

        This function gives access to the internal parameter of the DRM system.

        \param[in,out] json_value : Json::Value object containing all the
        parameters requested. When the function returns this object is
        overwritten with the values corresponding to the parameters so that the
        user application can parse it to access to them. For instance,
            Json::Value json_value;
            json_value["CUSTOM_FIELD"]=json::nullValue;
            json_value["NUM_ACTIVATORS"]=json::nullValue;
    */
    void get( Json::Value& json_value ) const;

    /** \brief Get information from the DRM system.

        This function gives access to the internal parameter of the DRM system.

        \param[in,out] json_string : JSON formatted string listing the parameter
        names requested as key and Json::nullValue as value. For instance,
            std::string json_string = "{\"NUM_ACTIVATORS\": null, \"SESSION_ID\": null}";
    */
    void get( std::string& json_string ) const;

    /** \brief Get information from the DRM system.

        This function gives access to the internal parameter of the DRM system.

        \param[in] key_id : Unique identifier of the parameter to access;
        available IDs are listed in #ParameterKey.

        \return the value of the corresponding parameter.
    */
    template<typename T> T get( const ParameterKey key_id ) const;

    /** \brief Set information of the DRM system.

        This function overwrites an internal parameter of the DRM system.

        \param[in] json_value : Json::Value object listing the parameter with
        its new value. For instance,
            Json::Value json_value;
            json_value["CUSTOM_FIELD"]=0x12345678;
    */
    void set( const Json::Value& json_value );

    /** \brief Set information of the DRM system.

        This function overwrites an internal parameter of the DRM system.

        \param[in] json_string : JSON formatted string listing the parameter
        names and corresponding values. For instance,
            std::string json_string = "{\"CUSTOM_FIELD\": 0x12345678}";
    */
    void set( const std::string& json_string );

    /** \brief Set information of the DRM system.

        This function overwrites an internal parameter of the DRM system.

        \param[in] key_id : Unique identifier of the parameter to access;
        available IDs are listed in #ParameterKey.
        \param[in] value : Value to overwrite with.
    */
    template<typename T> void set( const ParameterKey key_id, const T& value );

};

}
}

#endif // _H_ACCELIZE_DRM_MANAGER
