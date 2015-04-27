#include <ev.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include "utils.h"

#define SERVER_PORT 7899

struct forge_ip_server_ctx {
	ev_io io;
	int fd;
	uint8_t buf[128];
};

static void forge_ip_read_cb(EV_P_ ev_io *w, int revents)
{
	struct forge_ip_server_ctx *s = (typeof(s))w;
	ssize_t len;
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	struct sockaddr_in real_source;

	len = recvfrom(s->fd, s->buf, 127, 0,
		       (struct sockaddr *)&from, (socklen_t *)&fromlen);
	if (len <= 0)
		return;
	s->buf[len] = '\0';
	memset(&real_source, 0, sizeof(real_source));
	real_source.sin_family = AF_INET;
	real_source.sin_port = (s->buf[0] << 8) + s->buf[1];
	if (inet_pton(AF_INET, (char *)&s->buf[2],
				&real_source.sin_addr) <= 0) {
		fprintf(stderr, "inet_pton() fail: %s\n", strerror(errno));
		fprintf(stderr, "from: %s\n", inet_ntoa(from.sin_addr));
		fprintf(stderr, "buf: %s\n", s->buf);
		return;
	}
	fprintf(stderr, "========================================\n"
			"real source port: %d ip: %s\n",
			real_source.sin_port, &s->buf[2]);
	len = sprintf((char *)s->buf, "forged source port: %d ip: %s\n",
			ntohs(from.sin_port), inet_ntoa(from.sin_addr));
	fprintf(stderr, "%s========================================\n\n",
		s->buf);
	sendto(s->fd, s->buf, len, 0,
		(const struct sockaddr *)&real_source, sizeof(real_source));
}

int main(int argc, char **argv)
{
	struct forge_ip_server_ctx s;
	struct ev_loop *loop = EV_DEFAULT;

	s.fd = init_server_UDP_fd(SERVER_PORT, 0);
	assert(s.fd > 0);

	ev_io_init(&s.io, forge_ip_read_cb, s.fd, EV_READ);
	ev_io_start(loop, &s.io);
	ev_run(loop, 0);
	return 0;
}
