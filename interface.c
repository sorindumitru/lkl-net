#include <stdlib.h>
#include <stdio.h>
#include <interface.h>

#include <asm/env.h>
#include <asm/eth.h>

interface_t* alloc_interface()
{
	interface_t* interface = malloc(sizeof(interface_t));
	interface->address.s_addr = 0;
	return interface;
}

int lkl_init_interface(const interface_t* interface)
{
	int ifindex;
	struct tun_device* td = malloc(sizeof(*td));

	/* TODO: initializa from interface_t */	
	td->type = TUN_HUB;
	td->port = interface->port;
	td->address = interface->gateway.s_addr;
	
	if ((ifindex=lkl_add_eth_tun(interface->dev, (char*) interface->mac, 32, td)) < 0) {
		printf("LKL init :: could not bring up interface %s\n",interface->dev);
		return -1;
	}

	if (interface->address.s_addr == 0) {
		printf("LKL init :: warning! interface address not defined\n");
	} else {
		if (lkl_if_set_ipv4(ifindex, interface->address.s_addr, interface->netmask_len) < 0) {
			printf("LKL init :: could not set IPv4 address\n");
			return -1;
		}
	}

	if (lkl_if_up(ifindex) < 0) {
		printf("LKL init :: could not bring up interface\n");
		return -1;
	}

	return ifindex;
}
