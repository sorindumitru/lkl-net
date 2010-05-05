#ifndef LKL_NET_HYPERVISOR_H_
#define LKL_NET_HYPERVISOR_H_

#include <config.h>
#include <console.h>
#include <netinet/in.h>

#define MAX_CONNECTIONS 1024

typedef struct hypervisor {
	unsigned int port;
	struct in_addr address;

	struct list_head links;
	struct list_head switches;
	struct list_head routers;

	struct list_head killme;//hubs or bridges should be killed on exit
				// because they run forever
	struct list_head waitforme;//for other devices we just wait
} hypervisor_t;

void init_hypervisor(hypervisor_t *hypervisor, conf_info_t *info);
void start_request_thread();
void* request_thread(void *params);

#endif /* LKL_NET_HYPERVISOR_H_ */
