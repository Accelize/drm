// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018, ProvenRun S.A.S
 */
/**
 * @file
 * @brief
 * @author Henri Chataing
 * @date September 12th, 2018 (creation)
 * @copyright (c) 2018-2021, Prove & Run S.A.S and/or its affiliates.
 *   All rights reserved.
 */

#ifndef _PROVENCORE_H_INCLUDED_
#define _PROVENCORE_H_INCLUDED_

#ifdef __cplusplus
extern "C"
{

typedef bool _Bool;
#endif

#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>

struct pnc_session;
typedef struct pnc_session pnc_session_t;

#define NO_TIMEOUT (0)

/**
 * @brief Open a new session for communicating with a provencore application.
 * @param size          Size in bytes of the requested shared buffer
 * @param session       Updated with the pointer to the allocated session handle
 * @return
 */
int pnc_session_new(size_t size, pnc_session_t **session);

/**
 * @brief Close the selected session.
 * @brief session       Pointer to the session handle
 */
void pnc_session_destroy(pnc_session_t *session);

/* Some macros to extract REE version informations
 * Given that \ref pnc_session_get_version allow retreival of version number, a
 * 32 bit integer with following layout:
 *  - [0;7]: minor number
 *  - [8;31]: major number
 */
#define REE_MAJOR(x)    ((((uint32_t)x) & UINT32_C(0xFFFFFF00)) >> 8)
#define REE_MINOR(x)    (((uint32_t)x) & UINT32_C(0xFF))

/**
 * @brief Get version for the REE solution used between S and NS world.
 *
 * Some functionalities, some API may or may not be available for given versions.
 * This function shall help developpers to know what is possible and what is not.
 *
 * Call to this function is only possible after a successfull call to
 * \ref pnc_session_new because session handle is needed to communicate with
 * Linux kernel driver
 *
 * @brief session       Pointer to the session handle
 * @param version       Buffer filled with value of running REE version
 * @return              - -EBADF if \p session or \p version is NULL
 *                      - -ENOSYS or -ENOTTY is used with non updated REE V1 and
 *                        V2. Available by default starting V3.
 *                      - 0 and \p version filled with REE version
 */
int pnc_session_get_version(pnc_session_t *session, uint32_t *version);

/**
 * @brief Configure the selected session with the identifier of the provencore
 *  application.
 * @param session       Pointer to the session handle
 * @param name          Name of the ProvenCore service or process to connect to
 * @return              0 in case of success, different from 0 in case of failure.
 *                      A negative value indicates error occured on the NS side,
 *                      a positive value indicates error occured on the S side.
 *
 * Among negative errors, one can find:
 * - -EOVERFLOW if service name is too long (unlikely)
 * - -ENOTSUP if REE version is not supported
 * - -EINVAL if session is not valid
 *
 * Notes: - Available only since REEV3.02.
 *        - process name must follow the DTS node name convention:
 *            at most 31 bytes long
 *            characters in 0-9, A-Z, a-z, ',', '.', '_', '+', '-'
 *            must start with a lower or uppercase letter
 */
int pnc_session_config_by_name(pnc_session_t *session, const char *name);

/**
 * @brief Configure the selected session with the identifier of the provencore
 *  application.
 * @param session       Pointer to the session handle
 * @param id            Identifier of the provencore application
 * @param is_sid        Indicate whether \p id is a PID or a service identifier
 *                      NOTE: starting REEV3, this value will always have to be 
 *                            true to pass configuration.
 * @return              0 in case of success, different from 0 in case of failure.
 *                      A negative value indicates error occured on the NS side,
 *                      a positive value indicates error occured on the S side.
 */
int pnc_session_config(pnc_session_t *session, uint64_t id, _Bool is_sid);

/**
 * @brief Retrieve the information for the selected session.
 * @param session       Pointer to the session handle
 * @param ptr           Updated with the virtual address of the shared buffer
 * @param size          Updated with the size in butes of the shared buffer
 * @return
 */
int pnc_session_getinfo(pnc_session_t *session, void **ptr, size_t *size);

////////////////////////////////////////////////////////////////////////////////
//
// Below 2 functions () are functions becoming obsolete with REEV3 and replaced
// by \ref pnc_session_send_request_and_wait_response
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Device selection flags.
 */
#define REE_DEV_MMC         0
#define REE_DEV_ENET        1

/**
 * @brief Send a request through the selected session.
 * The function sends a notification and wait for the provencore application
 * response.
 * @param session       Pointer to the session handle
 * @param type          Request type
 * @return
 *      - 0 on success
 *      - -1 in case of failure; \ref errno can take the following error codes:
 *          - ENODEV        the session is not in the correct state:
 *              make sure \ref pnc_session_config was properly called
 *              and the session does not have pending requests
 *          - EINTR         the request was interrupted by a signal
 *          - ERESTARTSYS   should normally not be received, indicates
 *              that the driver failed to reserve resources, the request
 *              can be retried immediately
 *          - EAGAIN        the driver failed to allocate resources,
 *              the request should be retried after a while
 */
int pnc_session_request(pnc_session_t *session, unsigned int type,
                        unsigned int flags);

/**
 * @brief Send a request with extended parameters through the selected session.
 * The function sends a notification and wait for the provencore application
 * response.
 * @param session       Pointer to the session handle
 * @param type          Request type
 * @return
 *      - 0 on success
 *      - -1 in case of failure; \ref errno can take the following error codes:
 *          - ENODEV        the session is not in the correct state:
 *              make sure \ref pnc_session_config was properly called
 *              and the session does not have pending requests
 *          - ETIMEDOUT     \ref timeout milliseconds elapsed before
 *              the request was completed by the server application
 *          - EINTR         the request was interrupted by a signal
 *          - ERESTARTSYS   should normally not be received, indicates
 *              that the driver failed to reserve resources, the request
 *              can be retried immediately
 *          - EAGAIN        the driver failed to allocate resources,
 *              the request should be retried after a while
 */
int pnc_session_request_ext(pnc_session_t *session, unsigned int type,
                            unsigned int flags, uint32_t timeout,
                            unsigned int *status);

////////////////////////////////////////////////////////////////////////////////
//
// Below is API available starting REEV3. Usage of this API with REEV2 (or V1...)
// version of Linux Kernel Provencore driver will return -ENOTTY or -ENOSYS
//
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Send response to a previous request.
 *
 * @param session       Pointer to the session handle
 * @param response      Response to send
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -EPROTO if server not ready to send response
 *                      - -EBADF if \p session is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00).
 *                      - 0 on success
 */
int pnc_session_send_response(pnc_session_t *session, uint32_t response);

/**
 * @brief Fetch available response for a given session
 *
 * Protocol forbids client to send new request until response received (unless
 * request is a `special request` such as request for request cancellation or
 * session termination.
 * As a result, the latest pending `normal response` is always available with a
 * call to this function. Subsequent call to this function, without sending new
 * request in between, will return -EAGAIN.
 *
 * @param session       Pointer to the session handle
 * @param response      Buffer updated with available response
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -EBADF if \p session or \p response is NULL
 *                      - -ENODEV if session not configured
 *                      - -EAGAIN if no response available
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00).
 *                      - 0 on success
 */
int pnc_session_get_response(pnc_session_t *session, uint32_t *response);

/**
 * @brief Wait for response reception
 *
 * @param session       Pointer to the session handle
 * @param response      Buffer updated with available response
 * @param timeout       Timeout in milliseconds to wait for application's
 *                      response.
 *                      Using \p timeout = \ref NO_TIMEOUT (0) sets
 *                      an infinite timeout.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -ETIMEDOUT if no response in time
 *                      - -EPIPE if session terminated while waiting
 *                      - -EBADF if \p session or \p response is NULL
 *                      - -EAGAIN if response not yet available
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00).
 *                      - 0 on success
 */
int pnc_session_wait_response(pnc_session_t *session, uint32_t *response,
        uint32_t timeout);

/**
 * @brief Send a request through the selected session.
 *
 * Don't wait for response.
 * API user can then do anything else and then:
 * - later call @pnc_session_get_response to check if a response is available
 * - later call @pnc_session_wait_response to force wait for a response
 *
 * @param session       Pointer to the session handle
 * @param request       Request to send
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -EPROTO if client not ready to send request
 *                      - -EBADF if \p session is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00).
 *                      - 0 on success
 */
int pnc_session_send_request(pnc_session_t *session, uint32_t request);

/**
 * @brief Fetch available request for a given session
 *
 * Protocol forbids client to send new request until A_RESPONSE received (unless
 * request is a `special request` such as A_CANCEL or A_TERM).
 * As a result, the latest pending `normal request` is always available with a
 * call to this function. Subsequent calls to this function, without having sent
 * A_RESPONSE in between, will return -EAGAIN.
 *
 * @param session       Pointer to the session handle
 * @param request       Buffer filled with available request
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -EBADF if \p session or \p request is NULL
 *                      - -ENODEV if session not configured
 *                      - -EAGAIN if no request available
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00).
 *                      - 0 on success
 */
int pnc_session_get_request(pnc_session_t *session, uint32_t *request);

/**
 * @brief Wait for request reception
 *
 * @param session       Pointer to the session handle
 * @param request       Buffer filled with available request
 * @param timeout       Timeout in milliseconds to wait for application's
 *                      request.
 *                      Using \p timeout = \ref NO_TIMEOUT (0) sets
 *                      an infinite timeout.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -EPROTO if server not ready to wait for new request
 *                      - -ETIMEDOUT if no request in time
 *                      - -EPIPE if session terminated while waiting
 *                      - -EBADF if \p session or \p request is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - 0 on success
 */
int pnc_session_wait_request(pnc_session_t *session, uint32_t *request,
        uint32_t timeout);

/** Possible return code for below \ref pnc_session_cancel_request */
#define REQUEST_CANCEL_OK       UINT32_C(0xABE00001)
#define REQUEST_CANCEL_RESPONSE UINT32_C(0xABE00002)

/**
 * @brief Request previous request cancellation and wait for acknowledge
 *
 * Regarding timings, acknowledge can also be a response to previous request.
 *
 * Out of usual negative error, this function may return:
 *  - REQUEST_CANCEL_OK: request cancelled and not handled by server application
 *  - REQUEST_CANCEL_RESPONSE: response to previous request fetched in
 * \p response buffer.
 *
 * @param session       Pointer to the session handle
 * @param response      Buffer filled with response
 * @param timeout       Timeout in milliseconds to wait for application's
 *                      acknowledge..
 *                      Using \p timeout = \ref NO_TIMEOUT (0) sets
 *                      an infinite timeout.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -EPROTO if client not ready to cancel request
 *                      - -ETIMEDOUT if no acknowledge in time
 *                      - -EPIPE if session terminated while waiting
 *                      - -EBADF if \p session or \p response is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - REQUEST_CANCEL_xxx status on success (see above)
 */
int pnc_session_cancel_request(pnc_session_t *session, uint32_t *response,
     uint32_t timeout);

/**
 * @brief Send a request through the selected session and wait for response.
 *
 * Composite function for the handling of consecutive calls to:
 *  - pnc_session_send_request
 *  - pnc_session_wait_response
 *
 * @param session       Pointer to the session handle
 * @param request       Request to send
 * @param timeout       Timeout in milliseconds to wait for application's
 *                      response.
 *                      Using \p timeout = \ref NO_TIMEOUT (0) sets
 *                      an infinite timeout.
 * @param response      Updated with the request's response in case of success.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -EBADF if \p session or \p response is NULL
 *                      - -ENODEV if session not configured
 *                      - -EPROTO if client not ready to send request
 *                      - -ETIMEDOUT if response not received in time
 *                      - -EPIPE if session terminated while waiting
 *                      - -EAGAIN if response not yet available (should never
 *                        occur)
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - 0 on success
 */
int pnc_session_send_request_and_wait_response(pnc_session_t *session,
        uint32_t request, uint32_t timeout, uint32_t *response);

/**
 * @brief Set signal pending and notify S
 *
 * A signal is a bit pending in a 32-bit signal register. This function allow to
 * set bits pending in session's signal register and to notify S if signal not
 * already pending.
 *
 * @param session       Pointer to the session handle
 * @param signals       Bitfield of all bits that must be set pending in
 *                      session's signal register.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -EBADF if \p session is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - 0 on success
 */
int pnc_session_send_signal(pnc_session_t *session, uint32_t signals);

/**
 * @brief Fetch any pending signal for a given session
 *
 * When receiving E_SIGNAL(s), NS driver only acknowledges E_SIGNAL notification
 * but signals are kept pending in S to NS session's signal register.
 * It is call to this function responsible for any pending signal acknowledge:
 * no new E_SIGNAL notification will be sent for an already pending signal as
 * long as this function is not called.
 * Subsequent calls to this function can always lead to new bits in \p signals.
 *
 * @param session       Pointer to the session handle
 * @param signals       Buffer updated with session's signal register content
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -EBADF if \p session or \p signals is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - 0 on success
 */
int pnc_session_get_signal(pnc_session_t *session, uint32_t *signals);

/**
 * @brief Wait for new S signal
 *
 * Returns as soon as any S to NS signal is pending.
 * This function removes any pending signal from session's signal register thus
 * allowing S to notify any new signal if any.
 *
 * @param session       Pointer to the session handle
 * @param signals       Buffer updated with session's signal register content
 * @param timeout       Timeout in milliseconds to wait for application's
 *                      signal.
 *                      Using \p timeout = \ref NO_TIMEOUT (0) sets
 *                      an infinite timeout.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -ETIMEDOUT if signal not received in time
 *                      - -EPIPE if session terminated while waiting
 *                      - -EBADF if \p session or \p signals is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - 0 on success
 */
int pnc_session_wait_signal(pnc_session_t *session, uint32_t *signals,
        uint32_t timeout);

/**
 * Bits that can be used to build mask when calling \ref pnc_session_wait_event
 */
#define EVENT_PENDING_SIGNAL    (UINT32_C(1) << 0)
#define EVENT_PENDING_REQUEST   (UINT32_C(1) << 1)
#define EVENT_PENDING_RESPONSE  (UINT32_C(1) << 2)
#define EVENT_PENDING_ALL       (EVENT_PENDING_SIGNAL  | \
                                 EVENT_PENDING_REQUEST | \
                                 EVENT_PENDING_RESPONSE)

/**
 * @brief Wait for any S event
 *
 * A S event can be a client request, a server response and/or a signal.
 *
 * Available bits in mask to wait event(s) are defined above:
 *  - EVENT_PENDING_SIGNAL: function returns if S signal received
 *  - EVENT_PENDING_REQUEST: function returns if S request received
 *  - EVENT_PENDING_RESPONSE: function returns if S response received
 *  - EVENT_PENDING_ALL: function returns if any of S event received
 *
 * When returning successfully, \p events bits are set to indicate which kind of
 * event was received and the corresponding \ref pnc_session_get_response,
 * \ref pnc_session_get_request and \ref pnc_session_get_signal functions can be
 * used to get each event details.
 *
 * @param session       Pointer to the session handle
 * @param events        Buffer filled with available event(s) bitfield.
 * @param mask          Bitmask to filter required events to wait for
 * @param timeout       Timeout in milliseconds to wait for application's
 *                      signal.
 *                      Using \p timeout = \ref NO_TIMEOUT (0) sets
 *                      an infinite timeout.
 * @return              - -ENOENT if SHM is not ready
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -ETIMEDOUT if signal not received in time
 *                      - -EPIPE if session terminated while waiting
 *                      - -EBADF if \p session or \p events is NULL
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x300 (e.g 3.00). 
 *                      - 0 on success
 */
int pnc_session_wait_event(pnc_session_t *session, uint32_t *events,
        uint32_t mask, uint32_t timeout);

/**
 * @brief Get a bitmask representing the available pending events
 *
 * A S event can be a client request, a server response and/or a signal.
 *
 * Bits returned in \p events are defined above:
 *  - EVENT_PENDING_SIGNAL if S signal received
 *  - EVENT_PENDING_REQUEST if S request received
 *  - EVENT_PENDING_RESPONSE if S response received
 *
 * When returning successfully, \p events bits are set to indicate which kind of
 * event was received and the corresponding \ref pnc_session_get_response,
 * \ref pnc_session_get_request and \ref pnc_session_get_signal functions can be
 * used to get each event details.
 *
 * @param session       Pointer to the session handle
 * @param events        Buffer filled with available event(s) bitfield.
 *                      - -EINVAL if invalid session handle
 *                      - -ENODEV if session not configured
 *                      - -ENOSYS or -ENOTTY if used with REE version strictly 
 *                        lower than 0x301 (e.g 3.01).
 *                      - 0 on success
 */
int pnc_session_get_pending_events(pnc_session_t *session, uint32_t *events);

/**
 * @brief Get the File Descriptor associated with an opened session
 *
 * @param session       Pointer to the session handle
 *
 * @return              The file descriptor associated to \p session, or -1 if
 *                      \p session is NULL
 */
int pnc_session_get_fd(pnc_session_t *session);

#ifdef __cplusplus
}
#endif

#endif /* _PROVENCORE_H_INCLUDED_ */
