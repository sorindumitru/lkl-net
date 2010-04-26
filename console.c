#include <string.h>
#include <stdlib.h>
#include <console.h>
#include <arpa/inet.h>

command show_if_commands[] = {
#if defined(ISROUTER)||defined(ISSWITCH)
#endif
#ifdef ISROUTER
#endif
#ifdef ISSWTICH
	{"bridge", 1, DEVICE_SWITCH, NULL, "Show bridge interface", (command*) NULL, "<bridge_name>"},
#endif
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command show_commands[] = {
#if defined(ISROUTER)||defined(ISSWITCH)
	{"mac-table", 1, DEVICE_SWITCH, NULL, "Show interface mac-table", (command*) NULL,"<switch-name>"},
	{"interface", -1, DEVICE_ALL, NULL, "Show interface information", (command*) show_if_commands,NULL},
	{"route", 0, DEVICE_ROUTER, NULL, "Show routing table", (command*) NULL,NULL},
#endif
#ifdef ISHYPERVISOR
	{"devices", 0, DEVICE_HYPERVISOR, do_show_devices, "Show devices", (command *) NULL, NULL},
	{"device", 1, DEVICE_HYPERVISOR, do_show_device, "Show device information", (command *) NULL, NULL},
#endif
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command create_commands[] = {
#ifdef ISHYPERVISOR
	{"link", 1, DEVICE_HYPERVISOR, NULL, "Create new link", (command*) NULL, "<hub_name>"},	
	{"router", 1, DEVICE_HYPERVISOR, NULL, "Create new router", (command*) NULL, "<router_name>"},
#endif
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command root[] = {
#ifdef ISSWITCH
	{"stp", 2, DEVICE_SWITCH, NULL, "Set STP ON/OFF", NULL, "<on/off> <switch_name>"},
#endif
#ifdef ISROUTER	
	{"remove", 3, DEVICE_ROUTER, NULL, "Remove route", (command*) NULL,"<ip_address> <gateway> <netmask>"},
	{"add", 3, DEVICE_ROUTER, NULL, "Add route", (command*) NULL, "<ip_address> <gateway> <netmask>"},
#endif
#ifdef ISHYPERVISOR
	{"create", -1, DEVICE_HYPERVISOR, NULL, "Create new link/device", create_commands,NULL},
#endif
	{"show", -1, DEVICE_ALL, NULL, "Show device information", show_commands, NULL},
	{"exit", 0, DEVICE_ALL, NULL, "Exit", (command*) NULL, NULL},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};


extern command *com=root;

int execute_line(char *line)
{
	char *token = strtok(line," \t");
	command* com = find_command(root, token);
	params *parameters;

	if (!com) {
		printf("command not found\n");
		return -1;
	}

	while (com->parameters_no == -1) {
		token = strtok(NULL," \t");
		if (!token) {
			printf("Command incomplete\n");
			return -1;
		}
		com = find_command(com->children, token);
		if (!com) {
			printf("Command not found\n");
			return -1;
		}
	}
	if (com->parameters_no > 0) {
		int i=0;
		parameters = malloc(sizeof(struct params));
		for(i=0;i<com->parameters_no;i++) {
			token = strtok(NULL," \t");
			if (!token) {
				printf("Command incomplete\n");
				return -1;
			}
			parameters->p[i] = token;
		}
	}

	return ((*(com->function)) (parameters));
}

command* find_command(const command *commands, const char *name)
{
	int i;

	for (i = 0; commands[i].name; i++) {
		if (strcmp (name, commands[i].name) == 0) {
			return (command*) &commands[i];
		}
	}

	return ((command *)NULL);
}

int do_exit_cmd(params *parameters)
{
	exit(0);
}
