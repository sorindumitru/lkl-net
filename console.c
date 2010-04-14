#include <console.h>

COMMAND commands[] = {
	{"stp", (params *) NULL, do_set_stp, "Set stp ON or OFF"},
	{"show", (params *) NULL, cmd_show, "Show information about BRIDGE"},
	{"macs", (params *) NULL, do_show_arp_table, "Show arp table"},
	{"help", (params *) NULL, show_help, "Show help"},
	{(char *) NULL, (params *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL}
};

int execute_line(char *line)
{
	int i = 0;
	COMMAND *command;
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

	return ((*(command->func)) (word));
}

COMMAND* find_command(const char *name)
{
	int i;

	for (i = 0; commands[i].name; i++) {
		if (strcmp (name, commands[i].name) == 0) {
			return (&commands[i]);
		}
	}

	return ((COMMAND *)NULL);
}
