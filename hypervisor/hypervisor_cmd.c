#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>

#include <config.h>
#include <device.h>
#include <hypervisor.h>
#include <hypervisor_cmd.h>

#include <arpa/inet.h>

extern conf_info_t *info;
extern hypervisor_t *hypervisor;
pthread_t request;

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
	list_for_each(head, &hypervisor->links){
		device_t *device = list_entry(head, device_t, list);
		printf("LKL NET :: started %s\n", device->hostname);
		struct params params;
		params.p[1] = malloc(8*sizeof(char));
		params.p[2] = malloc(sizeof(char));
		sprintf((char*)params.p[1],"%d",device->port);
		do_create_link(&params);
		//free(params.p[0]);
	}

	sleep(1);

	list_for_each(head, &hypervisor->switches){
		device_t *device = list_entry(head, device_t, list);
		printf("LKL NET :: started %s\n", device->hostname);
		struct params params;
		params.p[1] = malloc(8*sizeof(char));
		params.p[2] = malloc(sizeof(char));
		sprintf((char*)params.p[1],"%s",device->config);
		do_create_switch(&params);
		//free(params.p[0]);
	}

	list_for_each(head, &hypervisor->routers){
		device_t *device = list_entry(head, device_t, list);
		printf("LKL NET :: started %s\n", device->hostname);
		struct params params;
		params.p[1] = malloc(256*sizeof(char));
		params.p[2] = malloc(sizeof(char));
		sprintf((char*)params.p[1],"%s",device->config);
		do_create_router(&params);
		//free(params.p[0]);
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

void init_hypervisor(hypervisor_t *hypervisor, conf_info_t *info)
{
	hypervisor->port = info->general.port;
	hypervisor->address = info->general.address;

	hypervisor->links.next = &hypervisor->links;
	hypervisor->links.prev = &hypervisor->links;
	hypervisor->switches.next = &hypervisor->switches;
	hypervisor->switches.prev = &hypervisor->switches;
	hypervisor->routers.next = &hypervisor->routers;
	hypervisor->routers.prev = &hypervisor->routers;
	hypervisor->killme.next = &hypervisor->killme;
	hypervisor->killme.prev = &hypervisor->killme;
	hypervisor->waitforme.next = &hypervisor->waitforme;
	hypervisor->waitforme.prev = &hypervisor->waitforme;
}

void do_get_device_name(int sock, hyper_info_t *hinfo)
{
	struct list_head *head;
	socket_t *socket;
	int found = 0;
	hyper_info_t *response = malloc(sizeof(*response));
	response->type = REQUEST_DEVICE_NAME;
	response->length = 2*sizeof(unsigned int);
	
	list_for_each(head, &info->devices){
		device_t *dev = list_entry(head, device_t, list);
		if (!strcmp(hinfo->padding,dev->hostname)){
			found = 1;
			socket = get_device_socket(dev);
			break;
		}
	}

	if (found) {
		memcpy(response->padding, socket, sizeof(*socket));
		response->length += sizeof(*socket);
	} else {
		response->type = REQUEST_ERROR_DEVICE_NOT_FOUND;
		memset(response->padding, 0, PACKAGE_PADDING);
		response->length += 6;
	}

	send_hyper(sock, response);
}

void start_request_thread()
{
	int err;
	err = pthread_create(&request, NULL, request_thread, NULL);
	if (err < 0) {
		perror("LKL NET :: could not start request thread\n");
		exit(-1);
	}
}

void* request_thread(void *params)
{
	struct epoll_event event;
	int req_socket, err, one = 1, epoll_fd;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(57200),
	};
	addr.sin_addr.s_addr = INADDR_ANY;

	printf("LKL NET :: started request thread\n");
	req_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (req_socket < 0) {
		perror("LKL NET :: request thread :: Could not open request socket\n");
		exit(-1);
	}
	err = bind(req_socket, (struct sockaddr*) &addr, sizeof(addr));
	if (err < 0) {
		perror("LKL NET :: request thread :: Could not bind socket\n");
		exit(-1);
	}
	err = setsockopt(req_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	if (err < 0) {
		perror("LKL NET :: requestthread :: Could not set reuse address on socket\n");
		exit(-1);
	}
	err = listen(req_socket, MAX_CONNECTIONS);
	if (err < 0) {
		perror("LKL NET :: request thread :: Could not listen on socket\n");
		exit(-1);
	}

	epoll_fd = epoll_create(16);
	if (epoll_fd < 0) {
		perror("LKL NET :: request thread :: Could not intialise epoll\n");
		exit(-1);
	}
	event.data.fd = req_socket;
	event.events = EPOLLIN;
	err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, req_socket, &event);
	if (err < 0) {
		perror("LKL NET :: request thread :: Could not add socket to epoll\n");
		exit(-1);
	}

	while (1) {
		struct epoll_event ret_event;
		err = epoll_wait(epoll_fd, &ret_event, 1, -1);
		if (err < 0) {
			perror("LKL NET :: request thread :: Error on wait\n");
		}
		//TODO: process requests
		if (ret_event.data.fd == req_socket) {
			struct epoll_event new_event;
			int dev_sock = accept(req_socket, NULL, NULL);
			if (dev_sock < 0) {
				perror("could not accept connection");
				continue;
			}
			new_event.data.fd = dev_sock;
			new_event.events = EPOLLIN;
			err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dev_sock, &new_event);
		} else {
			int sock = ret_event.data.fd;
			hyper_info_t *hinfo = recv_hyper(sock);
			switch(hinfo->type){
			case REQUEST_DEVICE_NAME:
				printf("Received request for device name %s\n", hinfo->padding);
				do_get_device_name(sock,hinfo);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
				break;
			default:
				printf("Unknown request");
				break;
			}
			free(hinfo);
		}
	}

	shutdown(req_socket, SHUT_RDWR);

	return NULL;
}
