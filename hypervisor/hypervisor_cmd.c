#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <config.h>
#include <device.h>
#include <hypervisor.h>
#include <hypervisor_cmd.h>

#include <arpa/inet.h>

extern conf_info_t *info;
extern hypervisor_t *hypervisor;

int do_create_link(struct params *params)
{
	pid_t pid;
	int err;

	pid = fork();

	if (pid > 0) {
		if (!params->p[2]) {
			device_t *dev = malloc(sizeof(*dev));
			dev->type = DEV_HUB;
			dev->port = atoi(params->p[1]);
			dev->hostname = strdup(params->p[0]);
			INIT_LIST_HEAD(&dev->list);
			list_add(&dev->list,&hypervisor->links);
		}
	} else if (pid == 0) {
		char *args[] = {
			"xterm",
			"-e",
			"bin/hub", 
			params->p[1],
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
	pid_t pid;
	int err;

	pid = fork();

	if (pid > 0) {
		if (!params->p[2]) {
			device_t *dev = malloc(sizeof(*dev));
			dev->type = DEV_ROUTER;
			dev->config = strdup(params->p[1]);
			dev->hostname = strdup(params->p[0]);
			INIT_LIST_HEAD(&dev->list);
			list_add(&dev->list,&hypervisor->routers);
		}
	} else if (pid == 0) {
		char *args[] = {
			"xterm",
			"-e",
			"bin/router", 
			params->p[1],
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

int do_create_switch(struct params *params)
{
	pid_t pid;
	int err;

	pid = fork();

	if (pid > 0) {
		if (!params->p[2]) {
			device_t *dev = malloc(sizeof(*dev));
			dev->type = DEV_SWITCH;
			dev->config = strdup(params->p[1]);
			dev->hostname = strdup(params->p[0]);
			INIT_LIST_HEAD(&dev->list);
			list_add(&dev->list,&hypervisor->switches);
		}
	} else if (pid == 0) {
		char *args[] = {
			"xterm",
			"-e",
			"bin/switch", 
			params->p[1],
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
	printf("Device:\n\tName:\t\t%s\n\tAddress:\t%s\n\tPort:\t\t%d\n", (char*)params->p[0], socket->address, socket->port);

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

int do_boot_up(struct params *par) {
	struct list_head *head;

	//starting hubs
	list_for_each(head, &info->devices){
		device_t *device = list_entry(head, device_t, list);
		if (device->type == DEV_HUB) {
			printf("LKL NET :: started %s\n", device->hostname);
			struct params params;
			params.p[1] = malloc(8*sizeof(char));
			params.p[2] = malloc(sizeof(char));
			sprintf((char*)params.p[1],"%d",device->port);
			do_create_link(&params);
			free(params.p[0]);
		}
	}

	sleep(1);

	list_for_each(head, &info->devices){
		device_t *device = list_entry(head, device_t, list);
		if (device->type != DEV_HUB) {
			printf("LKL NET :: started %s\n", device->hostname);
			struct params params;
			params.p[1] = malloc(256*sizeof(char));
			params.p[2] = malloc(sizeof(char));
			sprintf((char*)params.p[1],"%s",device->config);
			switch(device->type){
			case DEV_ROUTER:
				do_create_router(&params);
				break;
			case DEV_SWITCH:
				do_create_switch(&params);
				break;
			default:
				printf("LKL NET :: unknown device %s\n", device->hostname);
			}
			free(params.p[0]);
		}
	}

	return 0;
}

static void dump_device(int fd, device_t* device)
{
	char buffer[128];
	memset(buffer,0,128);
	sprintf(buffer,"\tdevice {\n");
	write(fd, buffer, strlen(buffer));
	memset(buffer,0,128);
	sprintf(buffer,"\t\ttype %s;\n",get_type(device->type));
	write(fd, buffer, strlen(buffer));
	memset(buffer,0,128);
	sprintf(buffer,"\t\thostname %s;\n",device->hostname);
	write(fd, buffer, strlen(buffer));
	if (device->type != DEV_HUB) {
		memset(buffer,0,128);
		sprintf(buffer,"\t\tconfig %s;\n",device->config);
		write(fd, buffer, strlen(buffer));
	} else {
		memset(buffer,0,128);
		sprintf(buffer,"\t\tport %d;\n", device->port);
		write(fd, buffer, strlen(buffer));
	}
	memset(buffer,0,128);
	sprintf(buffer,"\t}\n");
	write(fd, buffer, strlen(buffer));

}


int do_dump_hyper_config(params *params)
{
	struct list_head *head;
	int fd = open(params->p[0], O_CREAT | O_WRONLY | O_TRUNC, 0666);	
	if (fd < 0) {
		perror("could not open file");
		return -1;
	}

	write(fd, "hypervisor {\n", strlen("hypervisor {\n"));

	list_for_each(head, &hypervisor->links) {
		device_t *device = list_entry(head, device_t, list);
		dump_device(fd, device);
	}

	list_for_each(head, &hypervisor->routers) {
		device_t *device = list_entry(head, device_t, list);
		dump_device(fd, device);
	}

	list_for_each(head, &hypervisor->switches) {
		device_t *device = list_entry(head, device_t, list);
		dump_device(fd, device);
	}

	write(fd, "}\n", strlen("}\n"));

	return 0;
}
