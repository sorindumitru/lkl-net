#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/lkl_router.h>

int main(int argc,char**argv)
{
	int err;
	struct list_head *head;
	conf_info_t* info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);

	if (lkl_env_init(16*1024*1024) < 0) {
		printf("LKL init :: could not init environment\n");
	}

	list_for_each(head,&info->interfaces){
		interface_t *interface = list_entry(head, interface_t, list);
		if (lkl_init_interface(interface) < 0) {
			printf("LKL init :: could not bring up interface %s\n", interface->dev);
		}
	}
	
	lkl_mount_proc();

	printf("ip_forward=%c\n",read_ip_forward_value());
	enable_ip_forward();
	printf("next ip_forward=%c\n",read_ip_forward_value());
	printf("next ip_forward=%c\n",read_ip_forward_value());
	while(1){
	}
	return 0;
}
