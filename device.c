#include <config.h>
#include <device.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
