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

#include <sys/types.h>

#define LBCD_PORTNUM 4330       /* Default port */
#define LBCD_MAXMESG 2048       /* Max UDP message to receive */
#define LBCD_MAX_SERVICES 5     /* Max service requests to allow */
#define LBCD_VERSION 3          /* Protocol version client speaks */
#define LBCD_TIMEOUT 5          /* Default service poll timeout */

/*
 * Request/Response codes.
 */
typedef enum P_OPS {
    op_lb_info_req       = 1,   /* Load balance info, request and reply */
} p_ops_t;

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
    u_int host_weight;          /* Computed host lb weight */
    u_int host_incr;            /* Computed host lb increment */
} LBCD_SERVICE;

/*
 * Request packet
 */
typedef struct {
    u_short version;            /* Protocol version */
    u_short id;                 /* Requestor's unique request id */
    u_short op;                 /* Operation requested */
    u_short status;             /* Number of services requested */
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
    u_int boot_time;            /* Boot time */
    u_int current_time;         /* Host time */
    u_int user_mtime;           /* Time user information last changed */
    u_short l1;                 /* 1 minute load (int) (load*100) */
    u_short l5;                 /* 5 minute load */
    u_short l15;                /* 15 minute load */
    u_short tot_users;          /* Total number of users logged in */
    u_short uniq_users;         /* Total number of uniq users */
    u_char on_console;          /* True if somone on console */
    u_char reserved;            /* Future use, padding ... */
    u_char tmp_full;            /* Percent of tmp full */
    u_char tmpdir_full;         /* Percent of P_tmpdir full */
    u_char pad;                 /* Padding */
    u_char services;            /* Nnumber of service requests */
    LBCD_SERVICE weights[LBCD_MAX_SERVICES + 1];
                                /* Host service weight/increment pairs */
} P_LB_RESPONSE, *P_LB_RESPONSE_PTR;

#endif /* !LBCD_PROTOCOL_H */
