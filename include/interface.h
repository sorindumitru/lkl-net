#ifndef INTERFACE_6IT2DSD
#define INTERFACE_6IT2DSD

#include <list.h>
#include <netinet/in.h>
#include <netinet/ether.h>

typedef struct interface{
	char* dev;
	struct ether_addr	*mac;
	struct in_addr address, def_addr, gateway;
	unsigned int netmask_len;
	unsigned short port;
	char *link;
	struct list_head list;
} interface_t;

interface_t* alloc_interface();
int lkl_init_interface(const interface_t* interface);
int lkl_list_interfaces(int max_ifno);
void lkl_change_ifname(int ifindex, char *newname);
void dump_interface(int fd, interface_t *interface);

#endif /* INTERFACE_6IT2DSD */
