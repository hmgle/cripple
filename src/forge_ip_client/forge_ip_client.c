#include <getopt.h>
#include <libnet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <utils.h>

#define SERVER_PORT 7899
#define CLIENT_PORT 7999

static int send_forged_addr_msg(const struct sockaddr_in *forged,
				const struct sockaddr_in *real,
				const struct sockaddr_in *dest)
{
	int packet_size = 0;
	uint8_t buf[128];
	libnet_ptag_t ret;
	char errbuf[LIBNET_ERRBUF_SIZE];
	libnet_t *l = libnet_init(LIBNET_RAW4, NULL, errbuf);
	if (l == NULL) {
		fprintf(stderr, "You need 'sudo' to run this!\n");
		exit(1);
	}

	packet_size += LIBNET_UDP_H;
	/* store real addr port */
	buf[0] = (real->sin_port >> 8) & 0xff;
	buf[1] = real->sin_port & 0xff;
	int buf_size = sprintf((char *)buf+2, "%s", inet_ntoa(real->sin_addr));
	buf_size += 2;
	packet_size += buf_size;
	ret = libnet_build_udp(forged->sin_port, dest->sin_port, packet_size, 0,
				buf, buf_size, l, 0);
	if (ret < 0) {
		fprintf(stderr, "libnet_build_udp() fail: %s\n",
				libnet_geterror(l));
		return -1;
	}
	ret = libnet_build_ipv4(packet_size + LIBNET_IPV4_H, 0, 0, 0, 255,
				IPPROTO_UDP, 0, forged->sin_addr.s_addr,
				dest->sin_addr.s_addr, NULL, 0, l, 0);
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

static void usage(char **argv)
{
	fprintf(stderr, "Usage: %s [Options]\n"
			"\n"
			"Options:\n"
			" -p --fp <forged port>       Set forged Port\n"
			" -i --fa <forged ipaddress>  Set forged IP\n"
			" -s --sa <server IP>         Set server IP\n"
			" -o --sp <server port>       Set server port\n"
			" -c --cp <client port>       Set client port\n"
			"\n", argv[0]);
}

int main(int argc, char **argv)
{
	int cli_fd;
	int opt, index;
	uint8_t buf[256];
	struct sockaddr_in to;
	struct sockaddr_in real;
	struct sockaddr_in forged;
	uint16_t forged_port = 1234;
	uint16_t client_port = CLIENT_PORT;
	struct in_addr my_addr;
	my_addr.s_addr = get_first_network_addr();
	char *forged_ip = inet_ntoa(my_addr);
	uint16_t server_port = SERVER_PORT;
	char *server_ip = "138.128.215.119";
	int ret;

	static struct option long_opts[] = {
		{"help", no_argument,       0, 'h'},
		{"fp",   required_argument, 0, 'p'},
		{"fa",   required_argument, 0, 'i'},
		{"sp",   required_argument, 0, 'o'},
		{"sa",   required_argument, 0, 's'},
		{"cp",   required_argument, 0, 'c'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "hp:i:s:o:c:",
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
		case 'c':
			client_port = atoi(optarg);
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

	memset(&real, 0, sizeof(real));
	real.sin_family = AF_INET;
	real.sin_port = client_port;
	real.sin_addr = my_addr;

	memset(&forged, 0, sizeof(forged));
	forged.sin_family = AF_INET;
	forged.sin_port = forged_port;
	inet_pton(AF_INET, forged_ip, &forged.sin_addr);

	cli_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (cli_fd < 0) {
		perror("socket");
		exit(1);
	}
	struct timeval timeout = {
		.tv_sec = 3,
		.tv_usec = 0,
	};
	setsockopt(cli_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	ret = bind(cli_fd, (struct sockaddr*)&real, sizeof(real));
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	int try_count = 3;
	do {
		ret = send_forged_addr_msg(&forged, &real, &to);
		if (ret < 0) {
			fprintf(stderr, "send_forged_addr_msg fail\n");
			exit(1);
		}
		ret = recvfrom(cli_fd, buf, 127, 0,
			       (struct sockaddr *)&from, (socklen_t *)&fromlen);
		if (ret <= 0)
			continue;
		buf[ret] = '\0';
		printf("%s\n", buf);
		return 0;
	} while (--try_count > 0);
	fprintf(stderr, "Block!\n");
	return -1;
}
