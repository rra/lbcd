#ifndef LBCD_SERVER_H
#define LBCD_SERVER_H

#include "protocol.h"

/* protocol.c */

#ifdef __cplusplus
extern "C" {
#endif

void lbcd_pack_info(P_LB_RESPONSE *lb);
int lbcd_recv_udp(int s, 
                   struct sockaddr_in *cli_addr, int * cli_len,
                   char *mesg, int max_mesg);

int lbcd_send_status(int s, 
                   struct sockaddr_in *cli_addr, int cli_len,
                   P_HEADER *request_header,
                   p_status_t pstat);

int lbcd_print_load(void);

#ifdef __cplusplus
}
#endif

#endif
