#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <config.h>
#include <device.h>
#include <hypervisor.h>
#include <hypervisor_cmd.h>

#include <arpa/inet.h>

extern conf_info_t *info;

int do_create_link(struct params *params)
{
	pid_t pid;
	int err;

	pid = fork();

	if (pid > 0) {
		//add link to link list	
	} else if (pid == 0) {
		char *args[] = {
			"xterm",
			"-e",
			"bin/hub", 
			params->p[0],
			NULL
		};
		err = execvp("xterm", args);
		if (err < 0) {
			perror("could not run program");
		}
	} else {
		perror("LKL NET :: could not create link");
		exit(-1);
	}

	return 0;
}

int do_show_devices(struct params *params)
{
	struct list_head *head;
	printf("Devices:\n");
	list_for_each(head,&info->devices) {
		device_t *device = list_entry(head, device_t, list);
		printf("\t%s : %s\n", device->hostname, get_type(device->type));
	}
	return 0;
}

int do_create_router(struct params *params)
{
	return 0;
}

int do_show_device(struct params *params)
{
	device_t *device;
	struct list_head *head;
	socket_t *socket;
	
	list_for_each(head, &info->devices){
		device = list_entry(head, device_t, list);
		if (!strcmp(device->hostname,params->p[0])) {
			break;
		}
	}

	if (device == NULL) {
		printf("Device not found\n");
		return -1;
	}

	socket = get_device_socket(device);
	printf("Device:\n\tName:\t\t%s\n\tAddress:\t%s\n\tPort:\t\t%d\n", params->p[0], socket->address, socket->port);

	return 0;
}

int do_telnet(struct params *params)
{
	pid_t pid;
	int err;
	struct in_addr addr;
	pid = fork();
	char *address = params->p[0];
	char *port = params->p[1];// or device name
	
	//check if parameter is address or device name and convert device name to address
	if (!inet_pton(AF_INET, address, &addr)) {
		//TODO: convert device name to address 
	}

	if (pid > 0) {
		//add link to link list	
	} else if (pid == 0) {
		char *args[] = {
			"xterm",
			"-e",
			"telnet", 
			address,
			port,
			NULL
		};
		err = execvp("xterm", args);
		if (err < 0) {
			perror("could not run program");
		}
	} else {
		perror("LKL NET :: could not create link");
		exit(-1);
	}
	return 0;
}
