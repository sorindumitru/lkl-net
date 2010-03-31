#ifndef INTERFACE_6IT2DSD
#define INTERFACE_6IT2DSD

#include <list.h>
#include <netinet/in.h>
#include <netinet/ether.h>

typedef struct interface{
	char* dev;
	struct eth_addr	*mac;
	struct in_addr address, gateway;
	unsigned int netmask_len;
	unsigned short port;

	struct list_head list;
} interface_t;

interface_t* alloc_interface();
int lkl_init_interface(const interface_t* interface);

#endif /* INTERFACE_6IT2DSD */
