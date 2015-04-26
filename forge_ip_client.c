#include <getopt.h>
#include <libnet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"

#define SERVER_PORT 7899

static void usage(char **argv)
{
	fprintf(stderr, "Usage: %s [Options]\n"
			"\n"
			"Options:\n"
			" -p --port <forged port>     Set forged Port\n"
			" -i --ip <forget ipaddress>  Set forged IP\n"
			" -s --server <server IP>     Set server IP\n"
			" -o --sport <server port>    Set server port\n"
			"\n", argv[0]);
}

static int set_forged_addr_msg(uint8_t *buf,
			const char *forged_ip, uint16_t forged_port)
{
	/* TODO */
	return 0;
}

int main(int argc, char **argv)
{
	int cli_fd;
	int opt, index;
	uint8_t buf[128] = {0};
	struct sockaddr_in to;
	uint16_t forged_port = 1234;
	struct in_addr my_addr;
	my_addr.s_addr = get_first_network_addr();
	char *forged_ip = inet_ntoa(my_addr);
	uint16_t server_port = SERVER_PORT;
	char *server_ip = "138.128.215.119";

	static struct option long_opts[] = {
		{"help",   no_argument,       0, 'h'},
		{"port",   required_argument, 0, 'p'},
		{"ip",     required_argument, 0, 'i'},
		{"server", required_argument, 0, 's'},
		{"sport",  required_argument, 0, 'o'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "hp:i:",
				  long_opts, &index)) != -1) {
		switch (opt) {
		case 'p':
			forged_port = atoi(optarg);
			break;
		case 'i':
			forged_ip = optarg;
			break;
		case 's':
			server_ip = optarg;
			break;
		case 'o':
			server_port = atoi(optarg);
			break;
		case 0:
		case 'h':
		default:
			usage(argv);
			exit(0);
		}
	}
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = server_port;
	inet_pton(AF_INET, server_ip, &to.sin_addr);
	cli_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (cli_fd < 0) {
		perror("socket");
		exit(1);
	}
	int len = set_forged_addr_msg(buf, forged_ip, forged_port);
	sendto(cli_fd, buf, len, 0, (const struct sockaddr *)&to, sizeof(to));
	/* TODO: recv */
	return 0;
}
