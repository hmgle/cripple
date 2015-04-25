#include "stun.h"

#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
	int cli_fd;
	char *stun_server_ip;
	uint8_t buf[STUN_MAX_MESSAGE_SIZE];
	uint8_t *pos;

	if (argc < 2) {
		fprintf(stderr, "usage: %s stun-server-ip\n", argv[0]);
		exit(1);
	}
	stun_server_ip = argv[1];

	struct sockaddr_in to;
	int tolen = sizeof(to);
	memset(&to, 0, tolen);
	to.sin_family = AF_INET;
	to.sin_port = htons(3478);
	inet_pton(AF_INET, stun_server_ip, &to.sin_addr);

	cli_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (cli_fd == -1)
		return -1;
	pos = buf;
	pos[0] = 0x0;
	pos[1] = 0x1;
	pos[2] = 0x0; pos[3] = 0x0;
	pos = buf + 4;
	RAND_bytes(pos, 16);
	uint8_t transaction_id[16];
	memcpy(transaction_id, pos, 16);
	sendto(cli_fd, buf, 20, 0, (struct sockaddr *)&to, tolen);
	ssize_t len;
	struct sockaddr source_addr;
	socklen_t fromlen = sizeof(source_addr);
	len = recvfrom(cli_fd, buf, STUN_MAX_MESSAGE_SIZE, 0,
			&source_addr, &fromlen);
	int i;
	for (i = 0; i < len; i++) {
		fprintf(stderr, "%d: %#4x\n", i, buf[i]);
	}
	return 0;
}
