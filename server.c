#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ev.h>
#include <libnet.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "stun.h"
#include "utils.h"
#include "bithacks.h"

#define STUN_PORT 3478

static uint32_t FORGED_IP;
static uint16_t FORGED_PORT;

struct stun_address4 {
	uint32_t ip;
	uint16_t port;
};

struct stun_ctx {
	/*
	 * ev_io io 放最前面确保元素 io 地址即该结构起始地址,
	 * read_cb() 获取 struct stun_ctx server 地址语句:
	 * struct stun_ctx *server = (typeof(server))w;
	 * 依赖于此
	 */
	ev_io io;
	struct stun_address4 addr;
	int fd;
	struct stun_ctx *another;
	uint8_t buf[STUN_MAX_MESSAGE_SIZE];
};

static void stun_msg_hdr_parse(const uint8_t *msg, ssize_t len,
				struct stun_msg_hdr *hdr)
{
	int arrsize;

	assert(len >= 20);
	hdr->type = msg[1] + (msg[0] << 8);
	hdr->len = msg[3] + (msg[2] << 8);
	arrsize = ARRAY_SIZE(hdr->transaction_id);
	memcpy(hdr->transaction_id, msg + 4, arrsize);
}

static int get_change_request_attr(const uint8_t *msg, ssize_t len,
				int *is_change_ip, int *is_change_port)
{
	const uint8_t *pos;
	uint32_t attr_len;

	if (len < 28)
		return -1;
	pos = msg + 20;
	while (1) {
		if (pos[0] == 0x00 && pos[1] == 0x03 &&
		    pos[2] == 0x00 && pos[3] == 0x04) {
			if (B_IS_SET(pos[7], 2))
				*is_change_port = 1;
			else
				*is_change_port = 0;
			if (B_IS_SET(pos[7], 3))
				*is_change_ip = 1;
			else
				*is_change_ip = 0;
			return 0;
		}
		attr_len = (pos[2] << 8) + pos[3];
		if (pos + 4 + attr_len - msg >= len)
			break;
		pos += 4 + attr_len;
	}
	return -1;
}

static ssize_t attach_mapped_addr_attr(uint8_t *buf_end,
				const struct sockaddr_in *addr)
{
	/* attribute type: MAPPED-ADDRESS 0x0001 */
	buf_end[0] = 0x0;
	buf_end[1] = 0x01;
	/* attribute length: 0x0008 */
	buf_end[2] = 0x0;
	buf_end[3] = 0x08;
	/* protocol family: IPv4(0x0001) */
	buf_end[4] = 0x0;
	buf_end[5] = 0x01;
	/* Port */
	uint16_t port = ntohs(addr->sin_port);
	buf_end[6] = (port >> 8) & 0xff;
	buf_end[7] = port & 0xff;
	/* IP */
	uint32_t ip = ntohl(addr->sin_addr.s_addr);
	buf_end[8] = (ip >> 24) & 0xff;
	buf_end[9] = (ip >> 16) & 0xff;
	buf_end[10] = (ip >> 8) & 0xff;
	buf_end[11] = ip & 0xff;
	return 12;
}

static ssize_t attach_source_addr_attr(uint8_t *buf_end,
				const struct stun_address4 *addr)
{
	/* attribute type: SOURCE-ADDRESS 0x0004 */
	buf_end[0] = 0x0;
	buf_end[1] = 0x04;
	/* attribute length: 0x0008 */
	buf_end[2] = 0x0;
	buf_end[3] = 0x08;
	/* protocol family: IPv4(0x0001) */
	buf_end[4] = 0x0;
	buf_end[5] = 0x01;
	/* Port */
	uint16_t port = ntohs(addr->port);
	buf_end[6] = port & 0xff;
	buf_end[7] = (port >> 8) & 0xff;
	/* IP */
	uint32_t ip = ntohl(addr->ip);
	buf_end[8] = (ip >> 24) & 0xff;
	buf_end[9] = (ip >> 16) & 0xff;
	buf_end[10] = (ip >> 8) & 0xff;
	buf_end[11] = ip & 0xff;
	return 12;
}

static ssize_t attach_changed_addr_attr(uint8_t *buf_end,
				uint16_t port, uint32_t ip)
{
	/* attribute type: CHANGED-ADDRESS 0x0005 */
	buf_end[0] = 0x0;
	buf_end[1] = 0x05;
	/* attribute length: 0x0008 */
	buf_end[2] = 0x0;
	buf_end[3] = 0x08;
	/* protocol family: IPv4(0x0001) */
	buf_end[4] = 0x0;
	buf_end[5] = 0x01;
	/* Port */
	buf_end[6] = (port >> 8) & 0xff;
	buf_end[7] = port & 0xff;
	/* IP */
	buf_end[8]  = ip & 0xff;
	buf_end[9]  = (ip >> 8) & 0xff;
	buf_end[10] = (ip >> 16) & 0xff;
	buf_end[11] = (ip >> 24) & 0xff;
	return 12;
}

static ssize_t set_binding_resp(uint8_t *resp, const struct stun_ctx *server,
				const struct stun_msg_hdr *from_hdr,
				const struct sockaddr_in *from)
{
	int arrsize;
	uint8_t *pos;
	ssize_t ret;

	resp[0] = 0x01;
	resp[1] = 0x01;
	arrsize = ARRAY_SIZE(from_hdr->transaction_id);
	memcpy(resp + 4, from_hdr->transaction_id, arrsize);
	pos = resp + 4 + arrsize;
	ret = attach_mapped_addr_attr(pos, from);
	pos += ret;
	ret = attach_source_addr_attr(pos, &server->addr);
	pos += ret;
	/* test */
	ret = attach_changed_addr_attr(pos, FORGED_PORT, FORGED_IP);
	pos += ret;
	ssize_t len = pos - resp - 20;
	resp[2] = (len >> 8) & 0xff;
	resp[3] = len & 0xff;
	return pos - resp;
}

static int set_forgedip_binding_resp(const struct stun_ctx *server,
				     const struct stun_msg_hdr *from_hdr,
				     const struct sockaddr_in *from)
{
	uint8_t resp[2048];
	char errbuf[LIBNET_ERRBUF_SIZE];
	libnet_t *l = libnet_init(LIBNET_RAW4, NULL, errbuf);
	int packet_size = 0;
	int arrsize;
	uint8_t *pos;
	ssize_t ret;
	struct stun_address4 forged_addr = {
		.ip = FORGED_IP,
		.port = FORGED_PORT,
	};

	packet_size += LIBNET_UDP_H;
	resp[0] = 0x01;
	resp[1] = 0x01;
	arrsize = ARRAY_SIZE(from_hdr->transaction_id);
	memcpy(resp + 4, from_hdr->transaction_id, arrsize);
	pos = resp + 4 + arrsize;
	ret = attach_mapped_addr_attr(pos, from);
	pos += ret;
	ret = attach_source_addr_attr(pos, &forged_addr);
	pos += ret;
	ret = attach_changed_addr_attr(pos, server->addr.port, server->addr.ip);
	pos += ret;
	ssize_t len = pos - resp - 20;
	resp[2] = (len >> 8) & 0xff;
	resp[3] = len & 0xff;
	packet_size += len;
	ret = libnet_build_udp(FORGED_PORT, htons(from->sin_port), packet_size, 0,
			       resp, len + 20, l, 0);
	if (ret < 0) {
		fprintf(stderr, "libnet_build_udp() fail: %s\n",
				libnet_geterror(l));
		return -1;
	}
	ret = libnet_build_ipv4(packet_size + LIBNET_IPV4_H, 0, 0, 0, 255,
				IPPROTO_UDP, 0, FORGED_IP,
				from->sin_addr.s_addr, NULL, 0, l, 0);
	if (ret < 0) {
		fprintf(stderr, "libnet_build_ipv4() fail: %s\n",
				libnet_geterror(l));
		return -1;
	}
	if (libnet_write(l) < 0) {
		fprintf(stderr, "libnet_write fail: %s\n", libnet_geterror(l));
		return -1;
	}
	libnet_destroy(l);
	return 0;
}

static void read_cb(EV_P_ ev_io *w, int revents)
{
	struct stun_ctx *server = (typeof(server))w;
	ssize_t len;
	struct stun_msg_hdr msg_hdr;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	int is_change_ip, is_change_port;
	int ret;

	len = recvfrom(server->fd, server->buf, STUN_MAX_MESSAGE_SIZE, 0,
		       (struct sockaddr *)&from, (socklen_t *)&fromlen);
	if (len <= 0) {
		perror("recvfrom");
		return;
	}
	stun_msg_hdr_parse(server->buf, len, &msg_hdr);
#if 0
	fprintf(stderr, "hdr->type: %#x\n", msg_hdr.type);
	fprintf(stderr, "hdr->len: %#x\n", msg_hdr.len);
	int i;
	for (i = 0; i < 16; i++) {
		fprintf(stderr, "tr[%d]: %#x\n", i, msg_hdr.transaction_id[i]);
	}
#endif
	if (msg_hdr.type == BINDING_REQUEST && msg_hdr.len == 0) {
		/* send Binding Resp msg */
		len = set_binding_resp(server->buf, server, &msg_hdr, &from);
		sendto(server->fd, server->buf, len, 0,
			(const struct sockaddr *)&from, (socklen_t)fromlen);
	} else if (msg_hdr.type == BINDING_REQUEST &&
		   get_change_request_attr(server->buf, len,
			   	&is_change_ip, &is_change_port) == 0) {
		ret = set_forgedip_binding_resp(server, &msg_hdr, &from);
		if (ret < 0)
			fprintf(stderr, "set_forgedip_binding_resp fail\n");
	}
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	struct stun_ctx stun_server;
	struct stun_ctx stun_server2;
	uint32_t my_ip = get_first_network_addr();

	stun_server.addr.ip = my_ip;
	stun_server2.addr.ip = my_ip;
	stun_server.addr.port = STUN_PORT;
	stun_server2.addr.port = STUN_PORT + 1;
	stun_server.fd = init_server_UDP_fd(STUN_PORT, stun_server.addr.ip);
	stun_server2.fd = init_server_UDP_fd(STUN_PORT+1, stun_server2.addr.ip);
	assert(stun_server.fd > 0 && stun_server2.fd > 0);
	stun_server.another = &stun_server2;
	stun_server2.another = &stun_server;
	if ((stun_server.addr.ip & 0xff) < 0xfe)
		FORGED_IP = stun_server.addr.ip + 0x1000000;
	else
		FORGED_IP = stun_server.addr.ip - 0x1000000;
	FORGED_PORT = stun_server2.addr.port;

	ev_io_init(&stun_server.io, read_cb, stun_server.fd, EV_READ);
	ev_io_start(loop, &stun_server.io);
	ev_io_init(&stun_server2.io, read_cb, stun_server2.fd, EV_READ);
	ev_io_start(loop, &stun_server2.io);
	ev_run(loop, 0);
	return 0;
}
