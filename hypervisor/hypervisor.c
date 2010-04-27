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
pthread_t request;
unsigned int port;
char *prompt = ">";

int main(int argc, char **argv)
{
	char *command;
	info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);

	printf("LKL NET :: Hypervisor is starting\n");

	start_request_thread();
	initialize_autocomplete();
	port = info->general.port;

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

	hypervisor->killme.next = &hypervisor->killme;
	hypervisor->killme.prev = &hypervisor->killme;

	hypervisor->waitforme.next = &hypervisor->waitforme;
	hypervisor->waitforme.prev = &hypervisor->waitforme;
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
		.sin_port = port,
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
	}

	shutdown(req_socket, SHUT_RDWR);

	return NULL;
}
