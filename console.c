#include <string.h>
#include <stdlib.h>
#include <console.h>
#include <config.h>
#include <arpa/inet.h>
#include <fcntl.h>

command show_if_commands[] = {
#if defined(ISROUTER)||defined(ISSWITCH)
#endif
#ifdef ISROUTER
#endif
#ifdef ISSWITCH
	{"bridge", 1, DEVICE_SWITCH, do_show_bridge_interface, "Show bridge interface", (command*) NULL, "<bridge_name>"},
#endif
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command show_commands[] = {
#if defined(ISROUTER)||defined(ISSWITCH)
	{"mac-table", 1, DEVICE_SWITCH, do_show_mac_table, "Show interface mac-table", (command*) NULL, "<switch-name>"},
	{"interface", -1, DEVICE_ALL, NULL, "Show interface information", show_if_commands, NULL},
	{"route", 0, DEVICE_ROUTER, do_show_ip_route, "Show routing table", (command*) NULL, NULL},
#endif
#ifdef ISSWITCH
	{"stp", 1, DEVICE_SWITCH, do_show_stp, "Show stp information", (command*) NULL, "<bridge_name>"},
#endif
#ifdef ISHYPERVISOR
	{"devices", 0, DEVICE_HYPERVISOR, do_show_devices, "Show devices", (command *) NULL, NULL},
	{"device", 1, DEVICE_HYPERVISOR, do_show_device, "Show device information", (command *) NULL, NULL},
#endif
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command create_commands[] = {
#ifdef ISHYPERVISOR
	{"link", 2, DEVICE_HYPERVISOR, do_create_link, "Create new link", (command*) NULL, "<hub_name> <port>"},
	{"router", 1, DEVICE_HYPERVISOR, do_create_router, "Create new router", (command*) NULL, "<config file>"},
	{"switch", 1, DEVICE_HYPERVISOR, do_create_switch, "Create new switch", (command*) NULL, "<config file>"},
#endif
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command interface_commands[] = {
#ifdef ISROUTER
	{"add", 4, DEVICE_ROUTER, do_add_interface, "Add a new interface", (command*) NULL, "<name> <MAC address> <gateway address> <port no>"},
	{"up", 1, DEVICE_ROUTER, do_set_interface_up, "Bring interface up", (command*) NULL, "<interface name>" },
	{"down", 1, DEVICE_ROUTER, do_set_interface_down, "Set interface down", (command*) NULL, "<interface name>"},
	{"address", 3, DEVICE_ROUTER, do_change_if_address, "Change IP address", (command*) NULL, "<interface name> <IPv4 address> <netmask length>"},
	{"list", 0, DEVICE_ROUTER, do_list_router_interfaces, "List router interfaces", (command*) NULL, NULL},
	{(char *) NULL, -1, DEVICE_ROUTER, NULL,"Interface information",(command*) NULL, NULL}
#endif
};

command root[] = {
#ifdef ISSWITCH
	{"stp", 2, DEVICE_SWITCH, do_set_stp, "Set STP ON/OFF", NULL, "<on/off> <switch_name>"},
	{"dump", 1, DEVICE_ROUTER, do_dump_config_file, "Dump config file", (command*) NULL, "<config file>"},
#endif
#ifdef ISROUTER	
	{"remove", 3, DEVICE_ROUTER, do_remove_route, "Remove route", (command*) NULL, "<network_address/netmask> <interface_name>"},
	{"add", 3, DEVICE_ROUTER, do_add_route, "Add route", (command*) NULL, "<network_address/netmask> <interface_name>"},
	{"interface", -1, DEVICE_ROUTER, NULL, "Interface commands",interface_commands , NULL},
	{"dump", 1, DEVICE_ROUTER, do_dump_config_file, "Dump config file", (command*) NULL, "<config file>"},
#endif
#ifdef ISHYPERVISOR
	{"create", -1, DEVICE_HYPERVISOR, NULL, "Create new link/device", create_commands, NULL},
	{"telnet", 2, DEVICE_HYPERVISOR, do_telnet, "Telent to a device", (command*) NULL, "<ip_address> <port_no> | dev <device_name>"},
	{"boot", 0, DEVICE_HYPERVISOR, do_boot_up, "Boot up devices", (command*) NULL, ""},
	{"dump", 1, DEVICE_ALL, do_dump_hyper_config, "Dump configuration", (command *) NULL, "<file>"},
#endif
	{"show", -1, DEVICE_ALL, NULL, "Show device information", show_commands, NULL},
	{"exit", 0, DEVICE_ALL, do_exit_cmd, "Exit", (command*) NULL, NULL},
	{"test", 1, DEVICE_ALL, do_test, "Test", (command*) NULL, "<device_name>"},
	{(char *) NULL, -1, DEVICE_ALL, NULL,"Show device information",(command*) NULL, NULL}
};

command *com = root;
extern conf_info_t *info;

int execute_line(char *line)
{
	char *token = strtok(line," \t");
	command* com = find_command(root, token);
	params *parameters;

	if (!com) {
		printf("command not found\n");
		return -1;
	}

	while (com->parameters_no == -1) {
		token = strtok(NULL," \t");
		if (!token) {
			printf("Command incomplete\n");
			return -1;
		}
		com = find_command(com->children, token);
		if (!com) {
			printf("Command not found\n");
			return -1;
		}
	}
	if (com->parameters_no > 0) {
		int i=0;
		parameters = malloc(sizeof(struct params));
		for(i=0;i<com->parameters_no;i++) {
			token = strtok(NULL," \t");
			if (!token) {
				printf("Command incomplete\n");
				return -1;
			}
			parameters->p[i] = token;
		}
	}

	return ((*(com->function)) (parameters));
}

command* find_command(const command *commands, const char *name)
{
	int i;

	for (i = 0; commands[i].name; i++) {
		if (strcmp (name, commands[i].name) == 0) {
			return (command*) &commands[i];
		}
	}

	return ((command *)NULL);
}

int do_exit_cmd(params *parameters)
{
	exit(0);
}

int do_dump_config_file(params *parameters)
{
	int fd = open(parameters->p[0], O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd < 0) {
		perror("Could not open file");
		return -1;
	}
	dump_config_file(fd, info);	
	return 0;
}

int do_test(params* parameters)
{
	socket_t *socket = get_remote_device_socket(parameters->p[0]);
	if (socket == NULL) {
		printf("Device not found\n");
		return -1;
	}
	printf("Address:%s\nPort:%d\n", socket->address, socket->port);
	return 0;
}
