#ifndef LBCD_PROTOCOL_H
#define LBC_PROTOCOL_H

#define PROTO_PORTNUM 4330 
#define PROTO_MAXMESG 2048    /* max udp message to receive */
#define PROTO_VERSION 2

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
    status_proto_version       =3,  /* protocol version error */
    status_proto_error         =4,  /* generic protocol error */
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
  u_int boot_time;
  u_int current_time;
  u_int user_mtime;  /* time user information last changed */
  u_short l1; /* (int) (load*100) */
  u_short l5;
  u_short l15;
  u_short tot_users;  /* total number of users logged in */
  u_short uniq_users; /* total number of uniq users */
  u_char  on_console; /* true if somone on console */
  u_char  reserved;   /* future use, padding... */
} P_LB_RESPONSE, *P_LB_RESPONSE_PTR;


typedef struct {
  u_int boot_time;
  u_int current_time;
  double l1;
  double l5;
  double l15;
  u_int user_mtime;
  u_short tot_users;  /* total number of users logged in */
  u_short uniq_users; /* total number of uniq users */
  u_char  on_console; /* true if somone on console */
  u_char  reserved;   /* future use, padding... */
} LB_RESPONSE, *LB_RESPONSE_PTR;

#endif