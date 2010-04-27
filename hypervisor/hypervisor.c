#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <config.h>
#include <hypervisor.h>

conf_info_t *info;
pthread_t request;

int main(int argc, char **argv)
{
	char *prompt = ">";
	char *command;
	void *thread_ret;
	info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);

	printf("LKL NET :: Hypervisor is starting\n");

	start_request_thread();

	initialize_autocomplete();

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
	
	pthread_join(&request, &thread_ret);
}

void start_request_thread()
{
	int err;
	err = pthread_create(&request, NULL, request_thread, NULL);
	if (err < 0) {
		perror("LKL NET :: could not start request thread\n");
		exit(-1);
	}
}

void request_thread(void *params)
{
	printf("LKL NET :: started request thread\n");
}
