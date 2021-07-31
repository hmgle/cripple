#ifndef _STUN_H
#define _STUN_H

#include <stdint.h>
#include "server_export.h"

#define STUN_MAX_MESSAGE_SIZE	2048

#define BINDING_REQUEST			0x0001
#define BINDING_RESPONSE		0x0001
#define BINDING_ERROR_RESPONSE		0x0111
#define SHARED_SECRET_REQUEST		0x0002
#define SHARED_SECRET_RESPONSE		0x0102
#define SHARED_SECRET_ERROR_RESPONSE	0x0112

struct SERVER_EXPORT stun_msg_hdr {
	uint16_t type;
	uint16_t len;
	uint8_t transaction_id[16];
} __attribute__ ((packed));

#define MAPPED_ADDRES		0x0001
#define RESPONSE_ADDR		0x0002
#define CHANGE_REQUES		0x0003
#define SOURCE_ADDRES		0x0004
#define CHANGED_ADDRE		0x0005
#define USERNAME		0x0006
#define PASSWORD		0x0007
#define MESSAGE_INTEG		0x0008
#define ERROR_CODE		0x0009
#define UNKNOWN_ATTRI		0x000a
#define REFLECTED_FRO		0x000b

struct SERVER_EXPORT stun_msg_attr {
	uint16_t type;
	uint16_t len;
	uint8_t value[0];
} __attribute__ ((packed));

#endif
