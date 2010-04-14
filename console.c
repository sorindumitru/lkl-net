#include <string.h>
#include <stdlib.h>
#include <console.h>

/* command commands[] = {
	{"stp", (params *) NULL, do_set_stp, "Set stp ON or OFF"},
	{"show", (params *) NULL, cmd_show, "Show information about BRIDGE"},
	{"macs", (params *) NULL, do_show_arp_table, "Show arp table"},
	{"help", (params *) NULL, show_help, "Show help"},
	{(char *) NULL, (params *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL}
};*/

command show_if_commands[] = {
	{"bridge", 1, DEVICE_SWITCH, do_show_bridge_interface, "Show bridge interface", (command*) NULL},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL}
};

command show_commands[] = {
	{(char *) "mac-table", 1, DEVICE_SWITCH, do_show_mac_table, "Show interface mac-table", (command*) NULL},
	{(char *) "interface", -1, DEVICE_ALL, NULL, "Show interface information", (command*) show_if_commands},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL}
};

command root[] = {
	{"show", -1, DEVICE_ALL, NULL, "Show device information", show_commands},
	{"stp", 2, DEVICE_SWITCH, do_set_stp, "Set STP ON/OFF", NULL},
	{"exit", 0, DEVICE_ALL, do_exit_cmd, "Exit", (command*) NULL},
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
	/*int i = 0;
	command *command;
	char *word;

	while (line[i] && whitespace(line[i])) {
		i++;
	}
	word = line + i;

	while (line[i] && !whitespace(line[i])) {
		i++;
	}

	if (line[i]) {
		line[i++] = '\0';
	}

	command = find_command(word);

	if (!command) {
		printf("command not found\n");
		return -1;
	}
	
	while (whitespace(line[i])) {
	    i++;
	}
	word = line + i;

	if (!word) {
		printf("show accepts one parameter\n");
		return -1;
	}

	return ((*(command->function)) (word));*/
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
