#ifndef LBCD_PROTOCOL_SERVER_H
#define LBCD_PROTOCOL_SERVER_H

#include "protocol.h"

void proto_pack_lb_info(P_LB_RESPONSE *lb);
int proto_recv_udp(int s, 
                   struct sockaddr_in *cli_addr, int * cli_len,
                   char *mesg, int max_mesg);

int proto_send_status(int s, 
                   struct sockaddr_in *cli_addr, int cli_len,
                   P_HEADER *request_header,
                   p_status_t pstat);
#endif
