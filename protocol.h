#ifndef LBCD_PROTOCOL_H
#define LBCD_PROTOCOL_H

#include <sys/types.h>

/*
 * Constants
 */

#define LBCD_PORTNUM 4330	/* default port */
#define LBCD_MAXMESG 2048	/* max udp message to receive */
#define LBCD_MAX_SERVICES 5	/* max service requests to allow */
#define LBCD_VERSION 3		/* protocol version client speaks */

/*
 * Request/Response codes
 */

typedef enum P_OPS {
    op_lb_info_req             =1,  /* load balance info, request and reply */
} p_ops_t;

typedef enum P_STATUS {
    status_request             =0,  /* a request packet */
    status_ok                  =1,  /* load balance info, request and reply */
    status_error               =2,  /* generic error */
    status_lbcd_version        =3,  /* protocol version error */
    status_lbcd_error          =4,  /* generic protocol error */
    status_unknown_op          =5,  /* unknown operation requested */
} p_status_t;

/*
 * Service request/response types
 */
typedef char LBCD_SERVICE_REQ[16];

typedef struct LBCD_SERVICE {
  u_int host_weight;		/* computed host lb weight */
  u_int host_incr;		/* computed host lb increment */
  LBCD_SERVICE_REQ name;	/* service name (NUL terminated) */
} LBCD_SERVICE;

/*
 * Request packet
 */
typedef struct {
  u_short   version;  /* protocol version */
  u_short   id;       /* requestor's uniq request id */
  u_short   op;       /* operation requested */
  u_short   status;   /* set on reply */
} P_HEADER,*P_HEADER_PTR;

/*
 * Reply packet
 */
typedef struct {
  P_HEADER h;
  u_int boot_time;		/* boot time */
  u_int current_time;		/* host time */
  u_int user_mtime;		/* time user information last changed */
  u_short l1;			/* 1 minute load (int) (load*100) */
  u_short l5;			/* 5 minute load */
  u_short l15;			/* 15 minute load */
  u_short tot_users;		/* total number of users logged in */
  u_short uniq_users;		/* total number of uniq users */
  u_char on_console;		/* true if somone on console */
  u_char reserved;		/* future use, padding ... */
  u_char tmp_full;		/* percent of tmp full */
  u_char tmpdir_full;		/* percent of P_tmpdir full */
  u_char pad;			/* padding */
  u_char services;		/* number of service requests */
  u_int host_weight;		/* computed host lb weight */
  u_int host_incr;		/* computed host lb increment */
} P_LB_RESPONSE, *P_LB_RESPONSE_PTR;

#endif
