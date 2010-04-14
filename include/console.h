#ifndef LKL_NET_CONSOLE_H_
#define LKL_NET_CONSOLE_H_

#include <readline/readline.h>
#include <readline/history.h>

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
int do_set_stp PARAMS((const char*));
int do_show_arp_table PARAMS((const char*));

int execute_line(char *line);
COMMAND* find_command(const char* command);

#endif /* LKL_NET_CONSOLE_H_ */
