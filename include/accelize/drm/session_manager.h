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

#ifndef _H_ACCELIZE_DRM_MANAGER
#define _H_ACCELIZE_DRM_MANAGER

#include <string>
#include <functional>
#include <jsoncpp/json/json.h>
#include <cstring>


#include "accelize/drmc/common.h"
#include "accelize/drm/version.h"

//! Accelize interfaces and implementations
namespace Accelize {

//! DRM Controller specific interface and implementation
namespace DRM {

/// Enum class listing the parameters accessible from User code.
/// Some have ready only access while others have read and write access.
enum ParameterKey {
    /** Include the Parameter ID*/
#   define PARAMETERKEY_ITEM(id) id,
#   include "accelize/drm/ParameterKey.def"
#   undef PARAMETERKEY_ITEM
    ParameterKeyCount
} DRM_EXPORT;


const char* getApiVersion() DRM_EXPORT;

/** \brief manages a session of DRM on a DRM controller.

The session of DRM life-cycle is:
- 1. starting a session : from a locked DRM, install the first license from
  Accelize WebService which unlocks the protected IPs.
- 2. keep protected IPs unlocked by regularly supplying licenses and uploading DRM :
  this operation will be done in a dedicated thread managed by this class. It will involve
  communication with the DRM controller and the Accelize WebService.
- 3. May be paused : the DRM is no longer supplied with new licenses.
  Between pause(3) and resume(4) the end of license time maybe reached and therefore the protected IPs
  may be locked.
- 4. May be resumed after paused : the manager makes sure that the protected IPs is unlocked back again.
- 5. stop a session : uploads last metering data to the Accelize WebService and locks the protected IPs.
  After stop the session is correctly ended on the Accelize Webservice.

*/
class DRM_EXPORT DrmManager {
private:
    class Impl; //!< Internal representation
    //std::unique_ptr<Impl> pImpl; //!< Internal representation
    Impl* pImpl; //!< Internal representation

public:

    /** \brief Read callback function by offset
        Offset is relative to first register of DRM controller
        \warning This function must be thread-safe in case of concurrency on the register bus
    */
    typedef std::function<int/*errcode*/ (uint32_t /*register offset*/, uint32_t* /*returned data*/)> ReadRegisterCallback;

    /** \brief Write callback function by offset
        Offset is relative to first register of DRM controller
        \warning This function must be thread-safe in case of concurrency on the register bus
    */
    typedef std::function<int/*errcode*/ (uint32_t /*register offset*/, uint32_t /*data to write*/)> WriteRegisterCallback;

    /** \brief Asynchronous Error callback function.
        This function is called in case of asynchronous error during operation
    */
    typedef std::function<void (const std::string&/*error message*/)> AsynchErrorCallback;


    DrmManager() = delete; //!< No default constructor

    /** \brief Constructor with callback functions and web service initialization
       Constructs a DrmManager with callback functions and file required to initialize the web service. \see ReadRegisterCallback WriteRegisterCallback AsynchErrorCallback
       \param[in] conf_file_path : Path to json configuration file with configuration of the manager (Accelize WebService URL, Board type, Udid, ...)
       \param[in] cred_file_path : Path to json file containing your accelize.com Account credentials (client_id, client_private)
       \param[in] f_read_register : Callback function for reading DRM 32bit registers by offset
       \param[in] f_write_register : Callback function for writing DRM 32bit registers by offset
       \param[in] f_asynch_error : Callback function in case of asynchronous error
    */
    DrmManager( const std::string& conf_file_path,
                const std::string& cred_file_path,
                ReadRegisterCallback f_read_register,
                WriteRegisterCallback f_write_register,
                AsynchErrorCallback f_asynch_error );

    DrmManager(const DrmManager&) = delete; //!< Non-copyable

    DrmManager(DrmManager&&); //!< Support move

    ~DrmManager(); //!< Destructor

    /** \brief Activate DRM session
       This function activate/unlocks the hardware by unlocking the protected IPs in the FPGA and opening a DRM session.
       If a session is still pending the behavior depends on \p resume_session argument. If true the session is reused.
       Otherwise the session is closed and a new session is created.
       This function starts a std::thread that keeps the hardware unlocked by automatically updating the license in the HW when necessary.
       Any error during this thread execution is reported asynchronously through the AsynchErrorCallback function, if provided.
       When this function returns and the license is valid, the protected IPs are guaranteed to be unlocked.
       \param[in] resume_session_request : If true, the pending session is reused. If no pending session is found, create a new one. If
                                   false and a pending session is found, close it and create a new one.
    */
    void activate( const bool& resume_session_request = false );

    /** \brief Deactivate DRM session
       This function deactivates/locks the hardware back and close the session unless the \p pause_session argument is true. In this case,
       the session is kept opened for later use.
       This function joins the thread keeping the hardware unlocked.
       When the function returns, the hardware are guaranteed to be locked.
       \param[in] pause_session_request : If true, the current session is kept open for later usage. Otherwise, the current session is closed.
    */
    void deactivate( const bool& pause_session_request = false );

    /** \brief Get information from the DRM system using Json::Value object
       This function accesses the internal parameter of the DRM system.
       \param[in,out] json_value : Json::Value object containing all the parameters requested. When the function
            returns this object is overwritten with the values corresponding to the parameters so that the
            user application can parse it to access to them.
            For instance, Json::Value json_value;
                          json_value["CUSTOM_FIELD"]=json::nullValue;
                          json_value["NUM_ACTIVATORS"]=json::nullValue;
    */
    void get( Json::Value& json_value );

    /** \brief Get information from the DRM system using JSON formatted string
       This function accesses the internal parameter of the DRM system.
       \param[in,out] json_string : JSON formatted string listing the parameter names requested as key
            and Json::nullValue as value. For instance,
            std::string json_string = "{\"NUM_ACTIVATORS\": null, \"SESSION_ID\": null}";
    */
    void get( std::string& json_string );

    /** \brief Get information from the DRM system using a parameter key ID.
       This function accesses the internal parameter with the enum identifier defined in #ParameterKey
       \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #ParameterKey.
       \return the value of the corresponding parameter.
    */
    template<typename T> T get( const ParameterKey key_id );

    /** \brief Set information in the DRM system using Json::Value object
       This function overwrites an internal parameter of the DRM system.
       \param[in] json_value : Json::Value object listing the parameter with its new value.
            For instance, Json::Value json_value;
                          json_value["CUSTOM_FIELD"]=0x12345678;
    */
    void set( const Json::Value& json_value );

    /** \brief Set information in the DRM system using JSON formatted string
       This function overwrites an internal parameter of the DRM system.
       \param[in] json_string : JSON formatted string listing the parameter names and corresponding values.
            For instance, std::string json_string = "{\"CUSTOM_FIELD\": 0x12345678}";
    */
    void set( const std::string& json_string );

    /** \brief Set information in the DRM system using a parameter key ID.
       This function overwrite internal parameter based on an identifier defined in #ParameterKey
       \param[in] key_id : Unique identifier of the parameter to access; available IDs are listed in #ParameterKey.
       \param[in] value : Value to overwrite with.
    */
    template<typename T> void set( const ParameterKey key_id, const T& value );

};

}
}

#endif // _H_ACCELIZE_DRM_MANAGER
