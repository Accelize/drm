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

#ifndef _H_ACCELIZE_METERING_SESSION_MANAGER
#define _H_ACCELIZE_METERING_SESSION_MANAGER

#include <string>
#include <functional>

#include "accelize/drmc/common.h"

namespace Accelize {
namespace DRMLib {

/** \brief manages a session of metering on a DRM controller.

The session of metering life-cycle is:
- 1. starting a session : from a locked DRM, install the first license from
  Accelize WebService which unlocks the protected IPs.
- 2. keep protected IPs unlocked by regularly supplying licenses and uploading metering :
  this operation will be done in a dedicated thread managed by this class. It will involve
  communication with the DRM controller and the Accelize WebService.
- 3. May be paused : the DRM is no longer supplied with new licenses.
  Between pause(3) and resume(4) the end of license time maybe reached and therefore the protected IPs
  may be locked.
- 4. May be resumed after paused : the manager makes sure that the protected IPs is unlocked back again.
- 5. stop a session : uploads last metering data to the Accelize WebService and locks the protected IPs.
  After stop the session is correctly ended on the Accelize Webservice.

*/
class DRMLIB_EXPORT MeteringSessionManager {
private:
    class Impl; //!< Internal representation
    Impl* pImpl; //!< Internal representation

public:
    /** \brief Read callback function by offset

        Offset is relative to first register of DRM controller
        \warning This function must be thread-safe in case of concurrency on the register bus
    */
    typedef std::function<int/*errcode*/ (uint32_t /*register offset*/, uint32_t* /*returned data*/)> ReadReg32ByOffsetHandler;
    /** \brief Write callback function by offset

        Offset is relative to first register of DRM controller
        \warning This function must be thread-safe in case of concurrency on the register bus
    */
    typedef std::function<int/*errcode*/ (uint32_t /*register offset*/, uint32_t /*data to write*/)> WriteReg32ByOffsetHandler;
    /** \brief Read callback function by register name

        \warning This function must be thread-safe in case of concurrency on the register bus
    */
    typedef std::function<int/*errcode*/ (const std::string& /*register name*/, uint32_t* /*returned data*/)> ReadReg32ByNameHandler;
    /** \brief Write callback function by register name

        \warning This function must be thread-safe in case of concurrency on the register bus
    */
    typedef std::function<int/*errcode*/ (const std::string& /*register name*/, uint32_t /*data to write*/)> WriteReg32ByNameHandler;
    /** \brief Asynchronous Error callback function.

        This function is called in case of asynchronous error during operation
    */
    typedef std::function<void (const std::string&/*error message*/)> ErrorCallBackHandler;


    MeteringSessionManager() = delete; //!< No default constructor

    /** \brief Constructor with "ByOffset" functions

       Constructs a MeteringSessionManager with ByOffset callback functions. \see ReadReg32ByOffsetHandler WriteReg32ByOffsetHandler

       \param[in] conf_file_path : Path to json configuration file with configuration of the manager (Accelize WebService URL, Board type, Udid, ...)
       \param[in] cred_file_path : Path to json file containing your accelize.com Account credentials (client_id, client_private)
       \param[in] f_drm_read32 : Callback function for reading DRM 32bit registers by offset
       \param[in] f_drm_write32 : Callback function for writing DRM 32bit registers by offset
       \param[in] f_error_cb : Callback function in case of asynchronous error
    */
    MeteringSessionManager(const std::string& conf_file_path, const std::string& cred_file_path, ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb);

    /** \brief Constructor with "ByOffset" functions, uninitialized webservice connection

       Constructs a MeteringSessionManager with ByOffset callback functions. \see ReadReg32ByOffsetHandler WriteReg32ByOffsetHandler

       \param[in] f_drm_read32 : Callback function for reading DRM 32bit registers by offset
       \param[in] f_drm_write32 : Callback function for writing DRM 32bit registers by offset
       \param[in] f_error_cb : Callback function in case of asynchronous error

       \warning Only used for Debug purposes
    */
    MeteringSessionManager(ReadReg32ByOffsetHandler f_drm_read32, WriteReg32ByOffsetHandler f_drm_write32, ErrorCallBackHandler f_error_cb);

    /** \brief Constructor with "ByName" functions

       Constructs a MeteringSessionManager with ByName callback functions. \see ReadReg32ByNameHandler WriteReg32ByNameHandler

       \param[in] conf_file_path : Path to json configuration file with configuration of the manager (Accelize WebService URL, Board type, Udid, ...)
       \param[in] cred_file_path : Path to json file containing your accelize.com Account credentials (client_id, client_private)
       \param[in] f_drm_read32 : Callback function for reading DRM 32bit registers by name
       \param[in] f_drm_write32 : Callback function for writing DRM 32bit registers by name
       \param[in] f_error_cb : Callback function in case of asynchronous error

    */
    MeteringSessionManager(const std::string& conf_file_path, const std::string& cred_file_path, ReadReg32ByNameHandler f_drm_read32, WriteReg32ByNameHandler f_drm_write32, ErrorCallBackHandler f_error_cb);

    /** \brief Constructor with "ByName" functions, uninitialized webservice connection

       Constructs a MeteringSessionManager with ByName callback functions. \see ReadReg32ByNameHandler WriteReg32ByNameHandler

       \param[in] f_drm_read32 : Callback function for reading DRM 32bit registers by name
       \param[in] f_drm_write32 : Callback function for writing DRM 32bit registers by name
       \param[in] f_error_cb : Callback function in case of asynchronous error

       \warning Only used for Debug purposes
    */
    MeteringSessionManager(ReadReg32ByNameHandler f_drm_read32, WriteReg32ByNameHandler f_drm_write32, ErrorCallBackHandler f_error_cb);

    MeteringSessionManager(const MeteringSessionManager&) = delete; //!< Non-copyable
    MeteringSessionManager(MeteringSessionManager&&); //!< Support move
    ~MeteringSessionManager(); //!< Destructor

    /** \brief Set Udid

        This function can be used in case the Udid is not already set in the configuration file.
        In case the Udid has already been set by the configuration file, the new provided Udid will be used.
        The Udid will be transmitted to the Accelize WebService as additional information about the metering session.
        Please contact us if you don't know what Udid to use.
        \param[in] udid : udid as a std::string, format "00000000-0000-0000-0000-000000000000"
    */
    void setUdid(const std::string& udid);

    /** \brief Set BoardType

        This function can be used in case the BoardType is not already set in the configuration file.
        In case the BoardType has already been set by the configuration file, the new provided BoardType will be used.
        The BoardType will be transmitted to the Accelize WebService as additional information about the metering session.
        Please contact us if you don't know what BoardType to use.
        \param[in] boardType : boardType as a std::string
    */
    void setBoardType(const std::string& boardType);

    /** \brief Start a session (1)

       This function will always start a new session of metering. The behavior is undefined in case a session was already started.
       This function will start a std::thread to keep session active with new licenses when necessary (2).
       Any error during this thread execution will be reported asynchronously via the provided ErrorCallBackHandler.
       Once this function returns successfully the protected IPs are guaranteed to be unlocked.
    */
    void start_session();

    /** \brief Stop a session (5)

       This function will stop a session of metering.
       This function will join the thread keeping the session active (2) that may have been started by auto_start_session(), start_session() or resume_session().
       Once this function returns successfully the protected IPs are guaranteed to be locked and the session has been correctly ended on Accelize WebService.
    */
    void stop_session();

    /** \brief Pause a session (3)

       This function will pause a session of metering.
       This function will join the thread keeping the session active (2) that may have been started by auto_start_session(), start_session() or resume_session().
       Once this function returns successfully the protected IPs may or may not be locked depending on the time left for licenses currently installed on the DRM controller.
       Though it is guaranteed that protected IPs will be locked if those licenses reach the end of their validity time
    */
    void pause_session();

    /** \brief Resume a session (4)

       This function will resume a session of metering.
       This function will start a std::thread to keep session active with new licenses when necessary (2).
       Any error during this thread execution will be reported asynchronously via the provided ErrorCallBackHandler.
       Once this function returns successfully the protected IPs are guaranteed to be unlocked.
    */
    void resume_session();

    /** \brief Automatically Start or Resume a session (4)
       This function will automatically start or resume a session of metering.
       Depending on detected state of the DRM controller this function will call start_session() or resume_session().
       As a consequence : This function will start a std::thread to keep session active with new licenses when necessary (2).
       Once this function returns successfully the protected IPs are guaranteed to be unlocked.
    */
    void auto_start_session();

    /** \brief Dump DRM Hardware report

       \param[in,out] os : std::ostream on which hardware report will be dumped as text
       \note Only for Debug purposes
    */
    void dump_drm_hw_report(std::ostream& os);
};

}
}

#endif // _H_ACCELIZE_METERING_SESSION_MANAGER
