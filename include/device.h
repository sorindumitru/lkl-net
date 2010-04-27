#ifndef LKL_NET_DEVICE_H_
#define LKL_NET_DEVICE_H_

#include <string.h>

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

extern enum device_type get_device_type(char *type);

extern char* get_type(enum device_type type);

typedef struct socket {
	char *address;
	unsigned short port;
} socket_t;

extern socket_t* get_device_socket(device_t *device); 

#endif /* LKL_NET_DEVICE_H_ */
