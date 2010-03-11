#include <stdio.h>
#include <stdlib.h>
#include <conf_tree.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, const char *argv[])
{
	conf_info_t* info = (conf_info_t*) malloc(sizeof(conf_info_t));
	config_init(info);
	config_read_file(info,argv[1]);

	struct list_head *head;
	list_for_each( head, &info->topologies){
		topology_t* bridge = list_entry(head, topology_t, list);
		printf("Port = %d\n", bridge->port);
		printf("Address = %s\n", inet_ntoa(bridge->address));
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
