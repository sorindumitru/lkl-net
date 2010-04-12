#ifndef LKL_NET_SWITCH_H_
#define LKL_NET_SWITCH_H_

typedef struct params {
	int nr;
} params;

typedef struct COMMAND {
	char *name;
	params *params;
	rl_icpfunc_t *func;
	char *doc;
} COMMAND;

int set_host_name PARAMS((const char*));
int cmd_show PARAMS((const char*));
int show_help();

COMMAND commands[] = {
	{"show", (params *) NULL, cmd_show, "Show information about BRIDGE"},
	{"help", (params *) NULL, show_help, "Show help"},
	{(char *) NULL, (params *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL}
};

int execute_line(char *line);
COMMAND* find_command(const char* command);

#endif /* LKL_NET_SWITCH_H_ */
