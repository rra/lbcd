/*
 * Definition of the lbcd wire protocol.
 *
 * Written by Larry Schwimmer
 * Copyright 1996, 1997, 1998, 2012
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * See LICENSE for licensing terms.
 */

#ifndef LBCD_PROTOCOL_H
#define LBCD_PROTOCOL_H 1

#include <config.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_STDINT_H
# include <stdint.h>
#endif
#if HAVE_SYS_BITYPES_H
# include <sys/bitypes.h>
#endif
#include <sys/types.h>

#define LBCD_PORTNUM 4330       /* Default port */
#define LBCD_MAXMESG 2048       /* Max UDP message to receive */
#define LBCD_MAX_SERVICES 5     /* Max service requests to allow */
#define LBCD_VERSION 3          /* Protocol version client speaks */
#define LBCD_TIMEOUT 5          /* Default service poll timeout */

/*
 * Request/Response codes.
 */
enum lbcd_op {
    LBCD_OP_LBINFO = 1          /* Load balance info, request and reply */
};

typedef enum P_STATUS {
    status_request       = 0,   /* A request packet */
    status_ok            = 1,   /* Load balance info, request and reply */
    status_error         = 2,   /* Generic error */
    status_lbcd_version  = 3,   /* Protocol version error */
    status_lbcd_error    = 4,   /* Generic protocol error */
    status_unknown_op    = 5,   /* Unknown operation requested */
} p_status_t;

/*
 * Service request/response types.
 */
/* Service name (NUL-terminated). */
typedef char LBCD_SERVICE_REQ[32];

typedef struct LBCD_SERVICE {
    uint32_t host_weight;       /* Computed host lb weight */
    uint32_t host_incr;         /* Computed host lb increment */
} LBCD_SERVICE;

/*
 * Request packet
 */
typedef struct {
    uint16_t version;           /* Protocol version */
    uint16_t id;                /* Requestor's unique request id */
    uint16_t op;                /* Operation requested */
    uint16_t status;            /* Number of services requested */
} P_HEADER,*P_HEADER_PTR;

/*
 * Extended request packet.
 */
typedef struct {
    P_HEADER h;
    LBCD_SERVICE_REQ names[LBCD_MAX_SERVICES];
} P_HEADER_FULL, *P_HEADER_FULLPTR;

/*
 * Reply packet.
 */
typedef struct {
    P_HEADER h;
    uint32_t boot_time;         /* Boot time */
    uint32_t current_time;      /* Host time */
    uint32_t user_mtime;        /* Time user information last changed */
    uint16_t l1;                /* 1 minute load (int) (load*100) */
    uint16_t l5;                /* 5 minute load */
    uint16_t l15;               /* 15 minute load */
    uint16_t tot_users;         /* Total number of users logged in */
    uint16_t uniq_users;        /* Total number of uniq users */
    uint8_t on_console;         /* True if somone on console */
    uint8_t reserved;           /* Future use, padding ... */
    uint8_t tmp_full;           /* Percent of tmp full */
    uint8_t tmpdir_full;        /* Percent of P_tmpdir full */
    uint8_t pad;                /* Padding */
    uint8_t services;           /* Nnumber of service requests */
    LBCD_SERVICE weights[LBCD_MAX_SERVICES + 1];
                                /* Host service weight/increment pairs */
} P_LB_RESPONSE, *P_LB_RESPONSE_PTR;

#endif /* !LBCD_PROTOCOL_H */
