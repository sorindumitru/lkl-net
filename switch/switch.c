#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/lkl_bridge.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <console.h>

#define BRIDGE_NAME "brd0"

int bridge_id;

static int lkl_setup_switch(topology_t *topo)
{
	struct list_head* head;

	list_for_each(head, &topo->port_list){
		struct dev_list* dev = list_entry(head, struct dev_list, list);
		int ifindex = get_dev_index(dev->dev);
		if( lkl_br_add_interface(BRIDGE_NAME, ifindex) < 0) {
			printf("LKL init :: could not add interface %s to switch\n",dev->dev);
		}
	}

	return 0;
}

int main(int argc, const char **argv)
{
	int err;
	struct list_head *head;
	conf_info_t* info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);
	char *prompt;
	char *command;

	if (lkl_env_init(16*1024*1024) < 0) {
		printf("LKL init :: could not init environment\n");
	}

	list_for_each(head,&info->interfaces){
		interface_t *interface = list_entry(head, interface_t, list);
		if (lkl_init_interface(interface) < 0) {
			printf("LKL init :: could not bring up interface %s\n", interface->dev);
		}
	}

	//set up bridge interface
	if (lkl_br_add_bridge("brd0")) {
		printf("LKL init :: could not create bridge interface %s\n", BRIDGE_NAME);
	}

	list_for_each(head,&info->topologies){
		topology_t *topology = list_entry(head, topology_t, list);
		if (topology->type == TOP_SWITCH) {
			lkl_setup_switch(topology);
		}
	}

	if ((err=lkl_if_up(get_dev_index(BRIDGE_NAME)) < 0)) {
		printf("LKL init :: could not bring bridge %s up :: %d\n", BRIDGE_NAME,err);
	}

	prompt = malloc(strlen(info->general.hostname)+2);
	sprintf(prompt, "%s>", info->general.hostname);

	while(1){
		command = readline(prompt);
		if (!command) {
			break;
		}
		add_history(command);
		execute_line(command);
		free(command);
	}

        return 0;
}


