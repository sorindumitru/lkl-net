#include <stdlib.h>

#include <config.h>
#include <device.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

char* get_type(enum device_type type)
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

enum device_type get_device_type(char *type)
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

socket_t* get_device_socket(device_t *device)
{
	socket_t *socket = malloc(sizeof(*socket));
	conf_info_t *dev_info = malloc(sizeof(*dev_info));
	config_init(dev_info);
	config_read_file(dev_info, device->config);

	//TODO: Get device address
	socket->address = inet_ntoa(dev_info->general.address);
	socket->port = dev_info->general.port;

	config_free(dev_info);

	return socket;
}
