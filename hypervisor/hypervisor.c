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


