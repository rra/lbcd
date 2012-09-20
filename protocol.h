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

/* Protocol operation codes.  Currently, there's only one. */
enum lbcd_op {
    LBCD_OP_LBINFO = 1          /* Load balance info, request and reply */
};

/* Status codes returned in the header.  Some of these aren't used. */
enum lbcd_status {
    LBCD_STATUS_REQUEST    = 0, /* A request packet */
    LBCD_STATUS_OK         = 1, /* Load balance info, request and reply */
    LBCD_STATUS_ERROR      = 2, /* Generic error */
    LBCD_STATUS_VERSION    = 3, /* Protocol version error */
    LBCD_STATUS_PROTOCOL   = 4, /* Generic protocol error */
    LBCD_STATUS_UNKNOWN_OP = 5, /* Unknown operation requested */
};

/* Service name as sent on the wire (nul-terminated). */
typedef char lbcd_name_type[32];

/* Service weight information as included in the reply. */
struct lbcd_service {
    uint32_t host_weight;       /* Computed host lb weight */
    uint32_t host_incr;         /* Computed host lb increment */
};

/*
 * Packet header.  This is the entirety of the v2 request, and the header of
 * the v3 request.  It's also the header of the response from the server.
 */
struct lbcd_header {
    uint16_t version;           /* Protocol version */
    uint16_t id;                /* Requestor's unique request id */
    uint16_t op;                /* Operation requested */
    uint16_t status;            /* Number of services requested */
};

/*
 * Extended request packet.
 */
typedef struct {
    struct lbcd_header h;
    lbcd_name_type names[LBCD_MAX_SERVICES];
} P_HEADER_FULL, *P_HEADER_FULLPTR;

/*
 * Reply packet.
 */
typedef struct {
    struct lbcd_header h;
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
    struct lbcd_service weights[LBCD_MAX_SERVICES + 1];
                                /* Host service weight/increment pairs */
} P_LB_RESPONSE, *P_LB_RESPONSE_PTR;

#endif /* !LBCD_PROTOCOL_H */
