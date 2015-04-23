#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ev.h>
#include <assert.h>
#include <stdio.h>

#include "stun.h"
#include "utils.h"

#define STUN_PORT 3478

struct stun_address4 {
	uint32_t ip;
	uint16_t port;
};

struct stun_ctx {
	ev_io io;
	struct stun_address4 addr;
	int fd;
	struct stun_ctx *another;
	uint8_t buf[STUN_MAX_MESSAGE_SIZE];
};

static void stun_msg_hdr_parse(const uint8_t *msg, ssize_t len,
				struct stun_msg_hdr *hdr)
{
	/* TODO */
}

static int set_binding_resp(uint8_t *buf, const struct stun_ctx *server)
{
	/* TODO */
	return 0;
}

static void read_cb(EV_P_ ev_io *w, int revents)
{
	struct stun_ctx *server = (typeof(server))w;
	ssize_t len;
	struct sockaddr_in from;
	struct stun_msg_hdr msg_hdr;
	int fromlen = sizeof(from);

	len = recvfrom(server->fd, server->buf, STUN_MAX_MESSAGE_SIZE, 0,
		       (struct sockaddr *)&from, (socklen_t*)&fromlen);
	if (len <= 0) {
		perror("recvfrom");
		return;
	}
	stun_msg_hdr_parse(server->buf, len, &msg_hdr);
	if (msg_hdr.type == BINDING_REQUEST && msg_hdr.len == 0) {
		/* send Binding Resp msg */
	}
#if 0 /* debug */
	server->buf[len] = 0;
	fprintf(stderr, "%s", server->buf);
#endif
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
	stun_server2.fd = init_server_UDP_fd(STUN_PORT + 1, stun_server2.addr.ip);
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
