#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>

#include <config.h>
#include <device.h>
#include <hypervisor.h>
#include <hypervisor_cmd.h>

conf_info_t *info;
hypervisor_t *hypervisor;
pthread_t request;
unsigned int port;
char *prompt = ">";

int main(int argc, char **argv)
{
	struct list_head *head, *temp;
	char *command;
	info = malloc(sizeof(*info));
	hypervisor = malloc(sizeof(*hypervisor));
	config_init(info);
	config_read_file(info, argv[1]);
	init_hypervisor(hypervisor,info);

	printf("LKL NET :: Hypervisor is starting\n");

	start_request_thread();
	initialize_autocomplete();
	port = info->general.port;

	list_for_each_safe(head, temp, &info->devices){
		device_t *dev = list_entry(head, device_t, list);
		list_del(head);
		INIT_LIST_HEAD(&dev->list);
		switch(dev->type){
		case DEV_HUB:
			list_add(&dev->list, &hypervisor->links);
			break;
		case DEV_SWITCH:
			list_add(&dev->list, &hypervisor->switches);
			break;
		case DEV_ROUTER:
			list_add(&dev->list, &hypervisor->routers);
			break;
		default:
			break;
		}
	}

	printf("LKL NET :: Hypervisor initialised\n");
	while(1){
		command = readline(prompt);
		if (!command || (strlen(command) == 0)) {
			continue;
		}
		add_history(command);
		execute_line(command);
		free(command);
	}
	
	pthread_kill(request, SIGKILL);

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
