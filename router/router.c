#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/lkl_router.h>
#include <console.h>

char *prompt;
conf_info_t* info;

int main(int argc,char**argv)
{
	struct list_head *head;
	info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);
	char *command;

	if (!info->general.hostname && argc > 2){
		info->general.hostname = strdup(argv[2]);
	}

	if (lkl_env_init(16*1024*1024) < 0) {
		printf("LKL init :: could not init environment\n");
	}

	list_for_each(head,&info->interfaces){
		interface_t *interface = list_entry(head, interface_t, list);
		if (lkl_init_interface(interface) < 0) {
			printf("LKL init :: could not bring up interface %s\n", interface->dev);
		}
	}
	
	lkl_mount_proc();
	enable_ip_forward();

	if (info->general.hostname){
		prompt = malloc(strlen(info->general.hostname)+2);
		sprintf(prompt, "%s>", info->general.hostname);
	} else {
		prompt = malloc(strlen("Router>")+1);
		sprintf(prompt, "Router>");
	}
	initialize_autocomplete();

	start_device_thread();

	while(1){
		command = readline(prompt);
		if (!command || (strlen(command) == 0)) {
			continue;
		}
		add_history(command);
		execute_line(command);
		free(command);

	}
	return 0;
}
