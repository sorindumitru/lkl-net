#ifndef LKL_NET_CONSOLE_H_
#define LKL_NET_CONSOLE_H_

#include <readline/readline.h>
#include <readline/history.h>

#include <device.h>
#include <switch_cmd.h>
#include <router_cmd.h>
#include <hypervisor_cmd.h>

#define MAX_COMMAND_NO 20

#define DEVICE_ALL	1
#define DEVICE_ROUTER	2
#define DEVICE_SWITCH	4
#define DEVICE_HYPERVISOR 8

typedef struct command {
	char *name;
	int parameters_no;
	unsigned device_type;
	int (*function)(params*);
	char *documentation;
	struct command *children;
	char *parameters;
} command;

typedef struct all_commands {
	char **cmds;
	char **doc;
	int cmds_no;
} all_commands;

all_commands *cmd;
command *com;

int execute_line(char *line);
command* find_command(const command* commands, const char* command);
char *complete_other_words(const char *text,int state);
void list_commands(command* c, int i);
void initialize_autocomplete(void);

int do_dump_config_file(struct params *parameters);
int do_exit_cmd(struct params* parameters);
int do_test(struct params *parameters);

#endif /* LKL_NET_CONSOLE_H_ */
