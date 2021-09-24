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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pnr/provencore.h"

#define TZ_IOCTL_ALLOC          1
#define TZ_IOCTL_FREE           2
#define TZ_IOCTL_SEND           3
#define TZ_IOCTL_CONFIG         4
#define TZ_IOCTL_STATUS         5
#define TZ_IOCTL_CONFIG_SID     6
#define TZ_IOCTL_SEND_EXT       7
#define TZ_IOCTL(cmd, flags)    ((cmd) | ((flags) << 16))
#define TZ_IOCTL_VERSION        8224

////////////////////////////////////////////////////////////////////////////////
//
// Below ioctl are functional only starting REEV3
// Regarding whether they're used with V1 or V2, that shall return either ENOSYS
// or ENOTTY.
//
////////////////////////////////////////////////////////////////////////////////
#define TZ_IOCTL_SEND_RESP          8
#define TZ_IOCTL_GET_RESP           9
#define TZ_IOCTL_WAIT_RESP          10
#define TZ_IOCTL_SEND_REQ           11
#define TZ_IOCTL_GET_REQ            12
#define TZ_IOCTL_WAIT_REQ           13
#define TZ_IOCTL_CANCEL_REQ         14
#define TZ_IOCTL_SEND_SIGNAL        15
#define TZ_IOCTL_GET_SIGNAL         16
#define TZ_IOCTL_WAIT_SIGNAL        17
#define TZ_IOCTL_WAIT_EVENT         18
#define TZ_IOCTL_GET_PENDING_EVENTS 19

/* "virtual" SID meant to discover the underlying service using the name stored in
 * shared buffer of the session.
 */
#define TZ_CONFIG_ARG_GETSYSPROC_SID  UINT32_MAX

#define DEFAULT_PROVENCORE_DEVICE   "/dev/trustzone"

#ifndef PAGE_SIZE
#define PAGE_SIZE   0x1000
#endif

/**
 * @brief Records the information about a session opened through the
 * provencore driver.
 */
struct pnc_session {
    int fd;             /**< File descriptor open to the provencore device */
    void *ptr;          /**< Pointer to the memory mapped shared buffer */
    size_t size;        /**< Size in bytes of the allocated shared buffer */
    _Bool configured;   /**< Indicate whether the session is configured */
    uint32_t version;   /**< REE version */
};


static int _pnc_open(pnc_session_t *session, size_t size)
{
    if (session == NULL || (size % PAGE_SIZE) != 0) {
        return -EINVAL;
    }

    char const *devname = NULL;
    int ret;

    devname = getenv("PROVENCORE_DEVICE");
    if (devname == NULL) {
        devname = DEFAULT_PROVENCORE_DEVICE;
    }

    session->fd = open(devname, O_RDWR);
    if (session->fd == -1) {
        fprintf(stderr, "failed to open device file %s: %s\n",
                devname, strerror(errno));
        return -ENODEV;
    }

    ret = ioctl(session->fd, TZ_IOCTL_VERSION, &session->version);
    if (ret < 0) {
        if (errno == ENOSYS) {
            /* It is REEV1 indicating TZ_IOCTL_VERSION is not supported... */
            session->version = 1;
        } else if (errno == ENOTTY) {
            /* It is REEV2 indicating TZ_IOCTL_VERSION is not supported... */
            session->version = 2;
        } else {
            ret = -errno;
            goto release_fd;
        }
    }
    if ((session->version == 1) || (session->version == 2)) {
        /* Legacy version number for REEv1 or REEv2: move it as a bitfield:
         *   - 1 ==> 0x100, e.g 1.00
         *   - 2 ==> 0x200, e.g 2.00
         */
        session->version = (session->version << 8);
    }

    ret = ioctl(session->fd, TZ_IOCTL_ALLOC, size);
    if (ret < 0) {
        ret = -errno;
        goto release_fd;
    }

    session->ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, session->fd, 0);
    if (session->ptr == NULL) {
        ret = -ENOMEM;
        goto release_fd;
    }

    session->size = size;
    session->configured = 0;
    return 0;

release_fd:
    close(session->fd);
    session->fd = -1;
    session->configured = 0;
    return ret;
}

int pnc_session_new(size_t size, pnc_session_t **session)
{
    pnc_session_t *pnc_new;

    /* If we configure the session with a string rather than
     * a 64 bits id, we need more room in shared memory
     * to be able to store that string.
     */
    size = (size > PAGE_SIZE) ? size : PAGE_SIZE;

    int ret;

    if (session == NULL) {
        return -EINVAL;
    }

    pnc_new = (pnc_session_t *)malloc(sizeof(pnc_session_t));
    if (pnc_new == NULL) {
        return -ENOMEM;
    }

    ret = _pnc_open(pnc_new, size);
    if (ret < 0) {
        free(pnc_new);
        return ret;
    }

    *session = pnc_new;
    return 0;
}

void pnc_session_destroy(pnc_session_t *session)
{
    if (session == NULL || session->fd == -1) {
        return;
    }

    close(session->fd);
    munmap(session->ptr, session->size);
    memset(session, 0, sizeof(pnc_session_t));
    free(session);
}

int pnc_session_config_by_name(pnc_session_t *session, const char *name)
{
    if (session == NULL) {
        return -EINVAL;
    }

    int ret;
    uint32_t version = 0;

    pnc_session_get_version(session, &version);
    if (version < 0x302) {
        /* Avoid forwarding config request, as not supported by driver */
        fprintf(stderr, "config failure: configuring session by name requires REE>=3.2\n");
        return -ENOTSUP;
    }

    if (strlen(name) >= session->size) {
        return -EOVERFLOW;
    }
    strcpy((char *)session->ptr, name);

    ret = ioctl(session->fd, TZ_IOCTL_CONFIG_SID, TZ_CONFIG_ARG_GETSYSPROC_SID);
    if (ret < 0) {
        return -errno;
    }

    session->configured = (ret == 0) ? 1:0;
    return ret;
}

int pnc_session_config(pnc_session_t *session, uint64_t id, _Bool is_sid)
{
    if (session == NULL) {
        return -EINVAL;
    }

    int ret;
    uint32_t version = 0;

    /* Get REE version */
    pnc_session_get_version(session, &version);
    if ((is_sid == 0) && (version >= 0x300)) {
        /* Avoid forwarding config request if is_sid is not true since this 
         * feature is not supported anymore.
         */
        fprintf(stderr, "config failure: REE version requires a valid SID\n");
        return -EINVAL;
    }

    ret = ioctl(session->fd, is_sid ? TZ_IOCTL_CONFIG_SID : TZ_IOCTL_CONFIG, id);
    if (ret < 0) {
        return -errno;
    }

    session->configured = (ret == 0) ? 1:0;
    return ret;
}

int pnc_session_getinfo(pnc_session_t *session, void **ptr, size_t *size)
{
    if (session == NULL || session->fd == -1) {
        return -EINVAL;
    }
    if (ptr != NULL) {
        *ptr = session->ptr;
    }
    if (size != NULL)
        *size = session->size;
    return 0;
}

int pnc_session_get_version(pnc_session_t *session, uint32_t *version)
{
    if (session == NULL || version == NULL) {
        return -EBADF;
    }
    *version = session->version;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// The below 2 functions (pnc_session_request and pnc_session_request_ext)
// become obsolete with REEV3, replaced with
// pnc_session_send_request_and_wait_response
//
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Parameter vector for the \ref TZ_IOCTL_SEND_EXT request.
 */
struct pnc_send_params {
        uint32_t type;      /**< Input request type */
        uint32_t flags;     /**< Input request flags */
        uint32_t timeout;   /**< Optional input request timeout */
        uint32_t status;    /**< Output status code. */
};

typedef struct pnc_send_params pnc_send_params_t;


int pnc_session_request(pnc_session_t *session, uint32_t type,
                        uint32_t flags)
{
    if (session == NULL || !session->configured || (flags & ~UINT32_C(0xffff)) != 0) {
        return -EINVAL;
    }

    return ioctl(session->fd, TZ_IOCTL(TZ_IOCTL_SEND, flags), type);
}

int pnc_session_request_ext(pnc_session_t *session, uint32_t type,
                            uint32_t flags, uint32_t timeout,
                            uint32_t *status)
{
    if (session == NULL || !session->configured || (flags & ~UINT32_C(0xffff)) != 0) {
        return -EINVAL;
    }

    pnc_send_params_t params = {
        .type = type,
        .flags = flags,
        .timeout = timeout,
    };
    int ret = ioctl(session->fd, TZ_IOCTL_SEND_EXT, &params);
    if (ret != 0) {
        return ret;
    }
    if (status != NULL) {
        *status = params.status;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Below is API available starting REEV3. Usage of this API with REEV2 (or V1...)
// version of Linux Kernel Provencore driver will return -ENOTTY or -ENOSYS
//
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Parameter vector for ioctl needing it..
 */
typedef struct pnc_ioctl_params {
    uint32_t sent;      /**< Sent value */
    uint32_t returned;  /**< Returned value */
    uint32_t timeout;   /**< Optional timeout to wait for event(s), NO_TIMEOUT otherwise */
} pnc_ioctl_params_t;

int pnc_session_send_response(pnc_session_t *session, uint32_t response)
{
    int ret;

    if (session == NULL) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = ioctl(session->fd, TZ_IOCTL_SEND_RESP, response);
    if (ret < 0) {
        ret = -errno;
    }
    return ret;
}

int pnc_session_get_response(pnc_session_t *session, uint32_t *response)
{
    int ret;

    if ((session == NULL) || (response == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = ioctl(session->fd, TZ_IOCTL_GET_RESP, response);
    if (ret < 0) {
        ret = -errno;
    }
    return ret;
}

int pnc_session_wait_response(pnc_session_t *session, uint32_t *response,
        uint32_t timeout)
{
    int ret;

    if ((session == NULL) || (response == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    pnc_ioctl_params_t params = {
        .timeout = timeout,
    };

    ret = ioctl(session->fd, TZ_IOCTL_WAIT_RESP, &params);
    if (ret == 0) {
        *response = params.returned;
    } else {
        ret = -errno;
    }

    return ret;
}

int pnc_session_send_request(pnc_session_t *session, uint32_t request)
{
    int ret;

    if (session == NULL) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = ioctl(session->fd, TZ_IOCTL_SEND_REQ, request);
    if (ret < 0) {
        ret = -errno;
    }
    return ret;
}

int pnc_session_get_request(pnc_session_t *session, uint32_t *request)
{
    int ret;

    if ((session == NULL) || (request == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = ioctl(session->fd, TZ_IOCTL_GET_REQ, request);
    if (ret < 0) {
        ret = -errno;
    }
    return ret;
}

int pnc_session_wait_request(pnc_session_t *session, uint32_t *request,
        uint32_t timeout)
{
    int ret;

    if ((session == NULL) || (request == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    pnc_ioctl_params_t params = {
        .timeout = timeout,
    };

    ret = ioctl(session->fd, TZ_IOCTL_WAIT_REQ, &params);
    if (ret == 0) {
        *request = params.returned;
    } else {
        ret = -errno;
    }

    return ret;
}

int pnc_session_cancel_request(pnc_session_t *session, uint32_t *response,
     uint32_t timeout)
{
    int ret;

    if (session == NULL) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    pnc_ioctl_params_t params = {
        .timeout = timeout,
    };

    ret = ioctl(session->fd, TZ_IOCTL_CANCEL_REQ, &params);
    if (ret == (int)REQUEST_CANCEL_RESPONSE) {
        *response = params.returned;
    } else if (ret != (int)REQUEST_CANCEL_OK) {
        ret = -errno;
    }

    return ret;
}

int pnc_session_send_request_and_wait_response(pnc_session_t *session,
        uint32_t request, uint32_t timeout, uint32_t *response)
{
    int ret;

    if ((session == NULL) || (response == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = pnc_session_send_request(session, request);
    if (ret != 0) {
        return ret;
    }

    return pnc_session_wait_response(session, response, timeout);
}

int pnc_session_send_signal(pnc_session_t *session, uint32_t signals)
{
    int ret;

    if (session == NULL) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = ioctl(session->fd, TZ_IOCTL_SEND_SIGNAL, signals);
    if (ret < 0) {
        ret = -errno;
    }
    return ret;
}

int pnc_session_get_signal(pnc_session_t *session, uint32_t *signals)
{
    int ret;

    if ((session == NULL) || (signals == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    ret = ioctl(session->fd, TZ_IOCTL_GET_SIGNAL, signals);
    if (ret < 0) {
        ret = -errno;
    }
    return ret;
}

int pnc_session_wait_signal(pnc_session_t *session, uint32_t *signals,
        uint32_t timeout)
{
    int ret;

    if ((session == NULL) || (signals == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    pnc_ioctl_params_t params = {
        .timeout = timeout,
    };

    ret = ioctl(session->fd, TZ_IOCTL_WAIT_SIGNAL, &params);
    if (ret == 0) {
        *signals = params.returned;
    } else {
        ret = -errno;
    }

    return ret;
}

int pnc_session_wait_event(pnc_session_t *session, uint32_t *events,
        uint32_t mask, uint32_t timeout)
{
    int ret;

    if ((session == NULL) || (events == NULL)) {
        return -EBADF;
    }
    if (!session->configured) {
        return -EINVAL;
    }

    pnc_ioctl_params_t params = {
        .sent = mask,
        .timeout = timeout,
    };

    ret = ioctl(session->fd, TZ_IOCTL_WAIT_EVENT, &params);
    if (ret == 0) {
        *events = params.returned;
    } else {
        ret = -errno;
    }

    return ret;
}

int pnc_session_get_pending_events(pnc_session_t *session, uint32_t *events)
{
    int ret;

    if (session == NULL) {
        return -EINVAL;
    }
    if (!session->configured) {
        return -ENODEV;
    }

    pnc_ioctl_params_t params;

    ret = ioctl(session->fd, TZ_IOCTL_GET_PENDING_EVENTS, &params);
    if (ret == 0) {
        if (events)
            *events = params.returned;
    } else {
        ret = -errno;
        if (ret == -EAGAIN || ret == -ENOENT) {
            /*
             * These error codes should not happen as the session was configured
             * successfully
             */
            ret = -ENODEV;
        }
    }

    return ret;
}

int pnc_session_get_fd(pnc_session_t *session)
{
    if (session == NULL)
        return -1;

    return session->fd;
}
