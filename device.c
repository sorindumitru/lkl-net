#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <config.h>
#include <device.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string.h>

#include <interface.h>

extern conf_info_t *info;

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

int hypervisor_socket(){
	struct hostent *host;
	struct sockaddr_in hypervisor;
	int err, sock;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("could no open socket");
		return sock;
	}
	host = gethostbyname("127.0.0.1");
	hypervisor.sin_family = AF_INET;
	hypervisor.sin_port = htons(57200);
	hypervisor.sin_addr = *((struct in_addr*)host->h_addr);

	err = connect(sock, (struct sockaddr*) &hypervisor, sizeof(struct sockaddr));
	if (err < 0) {
		perror("could not connect");
		return err;
	}

	return sock;
}

socket_t* get_device_socket(device_t *device)
{
	char *address;
	socket_t *socket = malloc(sizeof(*socket));
        memset(socket, 0, sizeof(*socket));
	
	if (device->type == DEV_HUB) {
		strcpy(socket->address,"127.0.0.1");
		socket->port = device->port;
	} else {
		conf_info_t *dev_info = malloc(sizeof(*dev_info));
		config_init(dev_info);
		config_read_file(dev_info, device->config);

		address = inet_ntoa(dev_info->general.address);
		memcpy(socket->address, address, strlen(address));
		socket->port = dev_info->general.port;

		config_free(dev_info);
		//free(address);
	}

	return socket;
}

socket_t* get_remote_device_socket(char *device)
{
	socket_t *dev_socket;
	int sock = hypervisor_socket();
	hyper_info_t hinfo, *response;

	hinfo.type = REQUEST_DEVICE_NAME;
	hinfo.length = sizeof(hyper_info_header) + strlen(device) + 1;
	memset(hinfo.padding, 0, PACKAGE_PADDING);
	memcpy(hinfo.padding, device, strlen(device));

	send_hyper(sock, &hinfo);
	response = recv_hyper(sock);

	if (response->type == REQUEST_ERROR_DEVICE_NOT_FOUND) {
		close(sock);
		return NULL;
	}
	
	dev_socket = (socket_t*) response->padding;
	
	close(sock);
	return dev_socket;
}

int send_hyper(int sock, hyper_info_t *hinfo)
{
	int err;
	err = send(sock, hinfo, sizeof(hyper_info_header), 0);
	if (err < 0) {
		perror("could not send header");
		return -1;
	}
	err = send(sock, hinfo->padding, hinfo->length-2*sizeof(unsigned int), 0);
	if (err < 0) {
		perror("could not send data");
		return -1;
	}

	return 0;
}

hyper_info_t* recv_hyper(int sock)
{
	int err;
	hyper_info_t *hinfo = malloc(sizeof(*hinfo));

	err = recv(sock, hinfo, sizeof(hyper_info_header), 0);
	if (err < 0) {
		perror("could not receive header");
		return NULL;
	}
	err = recv(sock, hinfo->padding, hinfo->length - 2*sizeof(unsigned int), 0);
	if (err < 0) {
		perror("could not receive data");
		return NULL;
	}

	return hinfo;
}

#ifdef ISLKL
void start_device_thread()
{
	int err;
	pthread_t request;
	err = pthread_create(&request, NULL, device_request_thread, NULL);
	if (err < 0) {
		perror("LKL NET :: could not start request thread\n");
		exit(-1);
	}
}

#define MAX_CONNECTIONS	1024

void* device_request_thread(void *params)
{
	struct epoll_event event;
	int req_socket, err, one = 1, epoll_fd;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = info->general.port,
	};
        printf("REQUEST THREAD PORT %d\n",info->general.port);
	struct sockaddr_in raddr;
	struct hostent *he = gethostbyname("127.0.0.1");
	addr.sin_addr  = *(struct in_addr*)he->h_addr;

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
		perror("LKL NET :: request thread :: Could not set reuse address on socket\n");
		exit(-1);
	}
	err = listen(req_socket, MAX_CONNECTIONS);
	if (err < 0) {
		perror("LKL NET :: request thread :: Could not listen on socket\n");
		exit(-1);
	}

	//dump config file
	getsockname(req_socket, (struct sockaddr*) &raddr, &addrlen);
	if (!info->read){
		struct params params;
		info->general.address = raddr.sin_addr;
		info->general.port = raddr.sin_port;
		params.p[0] = info->config_file;
		do_dump_config_file(&params);
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
                        int err, sock = ret_event.data.fd;
                        request_t *request = malloc(sizeof(*request));
                        err = recv(sock, request, sizeof(*request), 0);
                        if (err < 0) {
                                printf("could not receive");
                        }
                        switch (request->type) {
                        case REQ_ADD_IF:
                                {
                                        interface_t *interface = malloc(sizeof(*interface));
                                        int size;
                                        err = recv(sock, interface, sizeof(*interface),  0);
                                        if (err < 0) {
                                                perror("could not recv if");
                                        }
                                        err = recv(sock, &size, sizeof(size),  0);
                                        if (err < 0) {
                                                perror("could not recv size1");
                                        }
                                        interface->dev = malloc(size);
                                        err = recv(sock, interface->dev, size,  0);
                                        if (err < 0) {
                                                perror("could not recv dev");
                                        }
                                        err = recv(sock, &size, sizeof(size),  0);
                                        if (err < 0) {
                                                perror("could not recv size2");
                                        }
                                        if (size > 0) {
                                                interface->link = malloc(size);
                                                err = recv(sock, interface->link, size,  0);
                                                if (err < 0) {
                                                        perror("could not recv link");
                                                }
                                        }
                                        interface->mac = malloc(sizeof(*(interface->mac)));
                                        err = recv(sock, interface->mac, sizeof(*(interface->mac)),  0);
                                        if (err < 0) {
                                                perror("could not recv mac");
                                        }
                                        lkl_init_interface_short(interface);
                                        break;
                                }
                        case REQ_DEL_IF:
                                break;
                        case REQ_ADD_LINK:
                                break;
                        default:
                                printf("UUPS\n");
                                break;
                        }
                }
	}

	shutdown(req_socket, SHUT_RDWR);

	return NULL;

}
#endif

int do_dump_config_file(params *parameters)
{
	int fd = open(parameters->p[0], O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd < 0) {
		perror("Could not open file");
		return -1;
	}
	dump_config_file(fd, info);	
	return 0;
}
