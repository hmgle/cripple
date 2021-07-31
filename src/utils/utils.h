#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "utils_export.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

int UTILS_EXPORT init_server_UDP_fd(int port, uint32_t ipaddr);

/* On error: return 0 */
uint32_t UTILS_EXPORT get_first_network_addr(void);

#endif
