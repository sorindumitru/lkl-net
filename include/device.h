#ifndef LKL_NET_DEVICE_H_
#define LKL_NET_DEVICE_H_

#include <string.h>
#include <list.h>

enum device_type {
	DEV_BRIDGE,
	DEV_HUB,
	DEV_ROUTER,
	DEV_SWITCH,
	DEV_HYPERVISOR,
	DEV_UNKNOWN
};

typedef struct device {
	enum device_type type;
	char *hostname;
	char *config;
	unsigned int port;

	struct list_head list;
} device_t;

typedef struct device_list_element {
	device_t *device;
	struct list_head list;
} device_list_element_t;

#define PACKAGE_PADDING		128

#define REQUEST_DEVICE_NAME	1
#define REQUEST_DUMP_CONFIG	2
#define REQUEST_ERROR_DEVICE_NOT_FOUND	128

typedef struct hyper_info_head {
	unsigned int type;
	unsigned int length;
} hyper_info_header;

typedef struct hyper_info {
	unsigned int type;
	unsigned int length;
	char padding[PACKAGE_PADDING];
} hyper_info_t;

#define IPV4_ADDR_LEN	16

typedef struct socket {
	char address[IPV4_ADDR_LEN];
	unsigned short port;
} socket_t;

extern enum device_type get_device_type(char *type);
extern char* get_type(enum device_type type);
extern socket_t* get_device_socket(device_t *device); 
extern void* device_request_thread(void *params);
extern void start_device_thread();
extern int send_hyper(int sock, hyper_info_t *hyper);
extern hyper_info_t* recv_hyper(int sock);
extern socket_t* get_remote_device_socket(char *device);

#endif /* LKL_NET_DEVICE_H_ */
