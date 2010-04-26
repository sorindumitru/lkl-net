#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <hypervisor.h>

conf_info_t *info;

int main(int argc, char **argv)
{
	char *prompt = ">";
	char *command;
	info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);

	printf("LKL NET :: Hypervisor is starting\n");

	start_request_thread();

	printf("LKL NET :: Hypervisor initialised\n");
	while(1){
		command = readline(prompt);
		if (!command || (strlen(command) == 0)) {
			continue;
		}
		add_history(command);
		execute_line(command);
		free(command);
	}

}

void start_request_thread()
{

}
