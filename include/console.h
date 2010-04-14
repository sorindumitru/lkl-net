#ifndef LKL_NET_CONSOLE_H_
#define LKL_NET_CONSOLE_H_

#include <readline/readline.h>
#include <readline/history.h>

#include <switch_cmd.h>

#ifndef LKL_NET_CONSOLE_PARAMS
#define LKL_NET_CONSOLE_PARAMS 1

#define MAX_PARAM_NO	8
typedef struct params {
	void *p[MAX_PARAM_NO];
} params;
#endif

#define DEVICE_ALL	1
#define DEVICE_ROUTER	2
#define DEVICE_SWITCH	4

typedef struct command {
	char *name;
	int parameters_no;
	unsigned device_type;
	int (*function)(params*);
	char *documentation;
	struct command *children;
} command;

int execute_line(char *line);
command* find_command(const command* commands, const char* command);

int do_exit_cmd(struct params* parameters);

#endif /* LKL_NET_CONSOLE_H_ */
