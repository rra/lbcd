#ifndef LBCD_PROTOCOL_H
#define LBC_PROTOCOL_H

#define LBCD_PORTNUM 4330 
#define LBCD_MAXMESG 2048    /* max udp message to receive */
#define LBCD_VERSION 2

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef enum P_OPS {
    op_lb_info_req             =1,  /* load balance info, request and reply */
} p_ops_t;

typedef enum P_STATUS {
    status_request             =0,  /* a request packet */
    status_ok                  =1,  /* load balance info, request and reply */
    status_error               =2,  /* generic error */
    status_lbcd_version       =3,  /* protocol version error */
    status_lbcd_error         =4,  /* generic protocol error */
    status_unknown_op          =5,  /* unknown operation requested */
} p_status_t;

typedef struct {
  u_short   version;  /* protocol version */
  u_short   id;       /* requestor's uniq request id */
  u_short   op;       /* operation requested */
  u_short   status;   /* set on reply */
} P_HEADER,*P_HEADER_PTR;

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
  u_short services;		/* number of service requests */
  u_int host_weight;		/* computed host lb weight */
  u_int host_incr;		/* computed host lb increment */
} P_LB_RESPONSE, *P_LB_RESPONSE_PTR;


#endif
