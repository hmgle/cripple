#include "utils.h"

#define IS_LO_IP(addr) (((addr) & 0xFF) == 127)

/* On error: return 0 */
uint32_t get_first_network_addr(void)
{
	int sd;
	struct ifconf ifc;
	struct ifreq ifr[10];
	int i;
	int ifc_num;
	uint32_t addr;

	sd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
		return 0;
	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;
	if (ioctl(sd, SIOCGIFCONF, &ifc) < 0)
		return 0;
	ifc_num = ifc.ifc_len / sizeof(struct ifreq);
	for (i = 0; i < ifc_num; i++) {
		if (ioctl(sd, SIOCGIFADDR, &ifr[i]) < 0)
			continue;
		addr =
		   ((struct sockaddr_in *)(&ifr[i].ifr_addr))->sin_addr.s_addr;
		if (!IS_LO_IP(addr))
			return addr;
	}
	return 0;
}
