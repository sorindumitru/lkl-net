#include <string.h>
#include <stdlib.h>
#include <console.h>
#include <arpa/inet.h>

command show_if_commands[] = {
	{"bridge", 1, DEVICE_SWITCH, do_show_bridge_interface, "Show bridge interface", (command*) NULL},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL}
};

command show_commands[] = {
	{(char *) "mac-table", 1, DEVICE_SWITCH, do_show_mac_table, "Show interface mac-table", (command*) NULL},
	{(char *) "interface", -1, DEVICE_ALL, NULL, "Show interface information", (command*) show_if_commands},
	{(char *) "route", 0, DEVICE_ROUTER, do_show_ip_route, "Show routing table", (command*) NULL},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL}
};

command root[] = {
	{"show", -1, DEVICE_ALL, NULL, "Show device information", show_commands},
	{"stp", 2, DEVICE_SWITCH, do_set_stp, "Set STP ON/OFF", NULL},
	{"exit", 0, DEVICE_ALL, do_exit_cmd, "Exit", (command*) NULL},
	{"add", 3, DEVICE_ROUTER, do_add_route, "Add router", (command*) NULL},
	{"remove", 3, DEVICE_ROUTER, do_remove_route, "Add router", (command*) NULL},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL}
};



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
			return &commands[i];
		}
	}

	return ((command *)NULL);
}

int do_exit_cmd(params *parameters)
{
	exit(0);
}
