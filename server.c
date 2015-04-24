#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ev.h>
#include <assert.h>
#include <stdio.h>

#include "stun.h"
#include "utils.h"
#include "bithacks.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define STUN_PORT 3478

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
	int i;
	int arrsize;

	hdr->type = msg[1] + (msg[0] << 8);
	hdr->len = msg[3] + (msg[2] << 8);
	arrsize = ARRAY_SIZE(hdr->transaction_id);
	for (i = 0; i < arrsize; i++)
		hdr->transaction_id[i] = msg[4 + i];
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
	buf_end[6] = port & 0xff;
	buf_end[7] = (port >> 8) & 0xff;
	/* IP */
	uint32_t ip = ntohl(addr->sin_addr.s_addr);
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
	/* TODO */
	int i;
	int arrsize;
	uint8_t *pos;

	resp[0] = 0x01;
	resp[1] = 0x01;
	arrsize = ARRAY_SIZE(from_hdr->transaction_id);
	for (i = 0; i < arrsize; i++)
		resp[4 + i] = from_hdr->transaction_id[i];
	pos = resp + 4 + i;
	ssize_t ret = attach_mapped_addr_attr(pos, from);
	pos += ret;
	return pos - resp;
}

static void read_cb(EV_P_ ev_io *w, int revents)
{
	struct stun_ctx *server = (typeof(server))w;
	ssize_t len;
	struct sockaddr_in from;
	struct stun_msg_hdr msg_hdr;
	int fromlen = sizeof(from);

	len = recvfrom(server->fd, server->buf, STUN_MAX_MESSAGE_SIZE, 0,
		       (struct sockaddr *)&from, (socklen_t *)&fromlen);
	if (len <= 0) {
		perror("recvfrom");
		return;
	}
	stun_msg_hdr_parse(server->buf, len, &msg_hdr);
	// fprintf(stderr, "hdr->type: %#x\n", msg_hdr.type);
	// fprintf(stderr, "hdr->len: %#x\n", msg_hdr.len);
	// int i;
	// for (i = 0; i < 16; i++) {
	// 	fprintf(stderr, "tr[%d]: %#x\n", i, msg_hdr.transaction_id[i]);
	// }
	if (msg_hdr.type == BINDING_REQUEST && msg_hdr.len == 0) {
		/* send Binding Resp msg */
		len = set_binding_resp(server->buf, server, &msg_hdr, &from);
		sendto(server->fd, server->buf, len, 0,
			(const struct sockaddr *)&from, (socklen_t)fromlen);
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

	ev_io_init(&stun_server.io, read_cb, stun_server.fd, EV_READ);
	ev_io_start(loop, &stun_server.io);
	ev_io_init(&stun_server2.io, read_cb, stun_server2.fd, EV_READ);
	ev_io_start(loop, &stun_server2.io);
	ev_run(loop, 0);

	return 0;
}
