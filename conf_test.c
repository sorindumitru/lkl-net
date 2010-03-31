#include <stdio.h>
#include <stdlib.h>
#include <config.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, const char *argv[])
{
	conf_info_t* info = (conf_info_t*) malloc(sizeof(conf_info_t));
	config_init(info);
	config_read_file(info,argv[1]);

	struct list_head *head;
	list_for_each(head, &info->topologies){
		topology_t* top = list_entry(head, topology_t, list);
		if (top->type == TOP_BRIDGE) {
			printf("Port = %d\n", top->port);
			printf("Address = %s\n", inet_ntoa(top->address));
		}
		if (top->type == TOP_SWITCH) {
			struct list_head *sw_head;
			list_for_each(sw_head, &top->port_list){
				struct dev_list *dle = list_entry(sw_head, struct dev_list, list);
				printf("Device %s\n", dle->dev);
			}
		}
	}

	list_for_each(head, &info->interfaces){
		interface_t* eth = list_entry(head, interface_t, list);
		printf("Address = %s\n", inet_ntoa(eth->address));
		printf("Netmask = %u\n", eth->netmask_len);
		//printf("Mac = %s\n", ether_ntoa(eth->mac));
		printf("Gateway = %s\n", inet_ntoa(eth->gateway));
		printf("Native interface = %s\n", eth->dev);
	}
	return 0;
}
