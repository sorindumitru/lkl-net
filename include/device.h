#ifndef LKL_NET_DEVICE_H_
#define LKL_NET_DEVICE_H_

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

static enum device_type get_device_type(char *type)
{
	if (!strcmp("router", type)) {
		return DEV_ROUTER;
	}
	if (!strcmp("switch", type)) {
		return DEV_SWITCH;
	}
	if (!strcmp("hub", type)) {
		return DEV_HUB;
	}
	if (!strcmp("bridge", type)) {
		return DEV_BRIDGE;
	}

	return DEV_UNKNOWN;
}


static char* get_type(enum device_type type)
{
	switch(type){
	case DEV_ROUTER:
		return "router";
		break;
	case DEV_SWITCH:
		return "switch";
		break;
	case DEV_HUB:
		return "hub";
		break;
	default:
		return "unknown";
		break;
	}
}

typedef struct socket {
	char *address;
	unsigned short port;
} socket_t;

extern socket_t* get_device_socket(device_t *device); 

#endif /* LKL_NET_DEVICE_H_ */
