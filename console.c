#include <console.h>

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

int cmd_show(const char *name)
{
	struct bridge_info binfo;

	lkl_br_get_bridge_info(name, &binfo);

	printf("bridge name\t\tbridge id\t\t\tSTP enabled\n%s\t\t",name);
	lkl_br_dump_bridge_id((unsigned char*) &binfo.bridge_id);
	printf("\t\t\t%s\n", (binfo.stp_enabled ? "yes" : "no"));

	return 0;
}
