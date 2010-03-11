#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <net/if.h>


#include <asm/env.h>
#include <asm/eth.h>

#include <conf_tree.h>

int main(int argc, const char *argv[])
{
	int ifindex = -1, err = -1;
	conf_info_t* conf = (conf_info_t*) malloc(sizeof(conf_info_t));
	interface_t* eth;
	config_init(conf);
	config_read_file(conf,argv[1]);

	int socket, client;
	socklen_t clilen;
	struct sockaddr_in sin, sclient;
	
	struct list_head* head;
	list_for_each(head, &conf->interfaces){
		eth = list_entry(head, interface_t, list);
	}

	//set up interface
	if ( lkl_env_init(16*1024*1024) < 0 ) {
		printf("failed to init lkl\n");
		return -1;
	}
	ifindex = lkl_add_eth(eth->dev, (char*) eth->mac, 32);
	if ( ifindex == 0 ) {
		return -1;
	}
	err = lkl_if_set_ipv4(ifindex, eth->address.s_addr, eth->netmask_len);
	if ( err < 0 ) {
		printf("failed to set ip\n");
		return -1;
	}
	err = lkl_if_up(ifindex);
	if ( err < 0 ) {
		printf("failed to bring up interface\n");
		return -1;
	}
	err = lkl_set_gateway(eth->gateway.s_addr);
	if ( err < 0) {
		printf("failed to set gateway %s : %s\n", inet_ntoa(eth->gateway), strerror(err));
		return -1;
	}

	socket = lkl_sys_socket(PF_INET, SOCK_STREAM, 0);
	if ( socket < 0 ) {
		printf("could not create socket\n");
		return -1;
	}
	memset((char *) &sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = eth->address.s_addr;
	sin.sin_port = htons(eth->port);

	err = lkl_sys_bind(socket, (struct sockaddr*) &sin, sizeof(sin));
	if ( err < 0 ) {
		printf("could not bind\n");
		return -1;
	}

	lkl_sys_listen(socket, 16);

	clilen = sizeof(struct sockaddr_in);
	client = lkl_sys_accept( socket, (struct sockaddr*) &sclient, &clilen);
	if ( client < 0 ) {
		printf("could not accept\n");
	}

	char buffer[1024];
	memset(buffer,0,1024);
	err = lkl_sys_recv(client,buffer,1024,0);
	if ( err < 0 ) {
		printf("error on recv\n");
		return -1;
	}
	err = lkl_sys_send(client, buffer, 1024,0);
	if ( err < 0 ) {
		printf("error on recv\n");
		return -1;
	}

	return 0;
}
