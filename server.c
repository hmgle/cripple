#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ev.h>
#include <assert.h>
#include <stdio.h>

#include "stun.h"

#define STUN_PORT 3478

static int init_server_UDP_fd(int port, char *bindaddr)
{
	int fd;
	struct sockaddr_in addr;
	int ret;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return -1;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bindaddr && inet_aton(bindaddr, &addr.sin_addr) == 0) {
		close(fd);
		return -1;
	}
	ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
		return -1;
	return fd;
}

struct stun_ctx {
	ev_io io;
	int fd;
	struct stun_ctx *another;
	uint8_t buf[STUN_MAX_MESSAGE_SIZE];
};

static void stun_msg_hdr_parse(const uint8_t *msg, ssize_t len,
				struct stun_msg_hdr *hdr)
{
	/* TODO */
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
}

int main(int argc, char **argv)
{
	struct ev_loop *loop = EV_DEFAULT;
	struct stun_ctx stun_server;
	struct stun_ctx stun_server2;

	stun_server.fd = init_server_UDP_fd(STUN_PORT, "0.0.0.0");
	stun_server2.fd = init_server_UDP_fd(STUN_PORT + 1, "0.0.0.0");
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
