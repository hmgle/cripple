#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

int init_server_UDP_fd(int port, uint32_t ipaddr);

/* On error: return 0 */
uint32_t get_first_network_addr(void);

#endif
