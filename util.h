#ifndef LBCD_UTIL_H
#define LBCD_UTIL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void util_debug_on();
void util_debug_off();

void util_log_info();
void util_log_error();
void util_log_open();
void util_log_close();
void util_start_daemon();

unsigned int util_get_ipaddress(char *host);
char *util_get_host_by_address(struct in_addr in);
int util_get_pid_from_file(char *file);
int util_write_pid_in_file(char *file);

#endif
