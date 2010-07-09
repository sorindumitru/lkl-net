#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <tokens.h>
#include <codes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <interface.h>
#include <device.h>

extern char* yytext;
extern FILE* yyin;
extern int num_lines;
extern int yyleng;
extern int yylex();

int config_init(conf_info_t* info)
{
	info->interfaces.next = &info->interfaces;
	info->interfaces.prev = &info->interfaces;
	info->topologies.next = &info->topologies;
	info->topologies.prev = &info->topologies; 
	info->devices.next = &info->devices;
	info->devices.prev = &info->devices;
	return ESUCCESS;
}

int config_free(conf_info_t* info)
{
	struct list_head *head, *temp;

	list_for_each_safe(head, temp, &info->interfaces) {
		interface_t *interface = list_entry(head, interface_t, list);
		list_del(head);
		free(interface);
	}

	list_for_each_safe(head, temp, &info->topologies) {
		topology_t *topology = list_entry(head, topology_t, list);
		list_del(head);
		free(topology);
	}

	list_for_each_safe(head, temp, &info->devices) {
		device_t *device = list_entry(head, device_t, list);
		list_del(head);
		free(device);
	}

	free(info);
	return ESUCCESS;
}

static int config_add_interface(conf_info_t* info, interface_t* interface)
{
	INIT_LIST_HEAD(&interface->list);
	list_add(&interface->list, &info->interfaces);
	return ESUCCESS;
}

static int config_add_topology(conf_info_t* info, topology_t* topology)
{
	INIT_LIST_HEAD(&topology->list);
	list_add(&topology->list, &info->topologies);
	return ESUCCESS;
}

static int config_add_device(conf_info_t* info, device_t *device)
{
	INIT_LIST_HEAD(&device->list);
	list_add(&device->list, &info->devices);
	return ESUCCESS;
}

int config_read_file(conf_info_t* info, const char* file_name)
{
	int token = -1;
	interface_t *current_interface;
	topology_t  *current_topology;

	if (file_name){
		info->config_file = strdup(file_name);
	}
	
	yyin = fopen(file_name, "r");
	if (yyin == NULL) {
		printf("Config reader :: could not open config file\n");
		return 1;
	}
	info->read = 1;
	info->general.hostname = "Device";
	token = yylex();
	while ( token ) {
		switch( token ){
		case TOK_T_HOSTNAME:
			token = yylex();
			if (token != TOK_HOSTNAME) {
				printf("Config reader :: expecting hostname at %d, but got %s\n", num_lines, yytext);
			}
			info->general.hostname = malloc((yyleng+1)*sizeof(char));
			strncpy(info->general.hostname, yytext, yyleng);
			break;
		case TOK_T_IPADDRESS:
			token = yylex();
			if (token != TOK_IPADDRESS) {
				printf("Config reader :: expecting IPv4 address at %d, but found %s\n", num_lines, yytext);
			}
			struct hostent *hostinfo =gethostbyname(yytext);
			info->general.address = *(struct in_addr*)hostinfo->h_addr;
			break;
		case TOK_T_PORT:
			token = yylex();
			if (token != TOK_PORT) {
				printf("Config reader :: expecting port at %d, but found %s\n", num_lines, yytext);
			}
			info->general.port = atoi(yytext);
			break;
		case TOK_INTERFACE:
		{
			token = yylex();
			if ( token != TOK_START ) {
				printf("%d:\tMissing {\n",num_lines);
				exit(-1);
			}
			current_interface = malloc(sizeof(interface_t));
			current_interface->link = NULL;
			
			token = yylex();
			while ( token != TOK_END ) {
				if ( token == TOK_T_IPADDRESS ) {
					token = yylex();
					if (token != TOK_IPADDRESS) {
						printf("Config reader :: expecting IPv4 address at %d, but got %s\n", num_lines, yytext);
					}
					struct hostent *hostinfo =gethostbyname(yytext);
					current_interface->address = *(struct in_addr*)hostinfo->h_addr;
				}
				if ( token == TOK_T_PORT ) {
					token = yylex();
					if (token != TOK_PORT) {
						printf("Config reader :: expecting port at %d, but found%s\n", num_lines, yytext);
					}
					current_interface->port = atoi(yytext);
				}
				if ( token == TOK_T_DEV ) {
					token = yylex();
					if (token != TOK_DEV) {
						printf("Config reader :: expecting device name at %d, but found %s\n", num_lines, yytext);
					}
					current_interface->dev = (char*) malloc((yyleng+1)*sizeof(char));
					memset(current_interface->dev, 0, (yyleng+1));
					strncpy(current_interface->dev, yytext, yyleng);
				}
				if ( token == TOK_NETMASK ) {
					current_interface->netmask_len = atoi(yytext+1);
				}
				if (token == TOK_T_DEFAULT) {
					token = yylex();
					if (token != TOK_IPADDRESS) {
						printf("Config reader :: expecting IPv4 address at %d, but got %s\n", num_lines, yytext);
					}
					struct hostent *hostinfo =gethostbyname(yytext);
					current_interface->def_addr = *(struct in_addr*)hostinfo->h_addr;
				}
				if ( token == TOK_T_MAC ) {
					token = yylex();
					if (token != TOK_MAC) {
						printf("Config reader :: expecting mac address at %d, but found %s\n", num_lines, yytext);
					}
					struct ether_addr *mac = (struct ether_addr*) ether_aton(yytext);
					current_interface->mac = malloc(sizeof(*mac));
					memcpy(current_interface->mac, mac, sizeof(*mac));
				}
				if (token == TOK_T_LINK) {
					token = yylex();
					current_interface->link = strdup(yytext);
				}
				if ( token == TOK_T_GATEWAY ) {
					token = yylex();
					if ( token != TOK_IPADDRESS ) {
						printf("Config reader :: expecting IPv4 address at %d, but got %s\n", num_lines, yytext);
					}
					struct hostent *hostinfo = gethostbyname(yytext);
					//inet_pton(AF_INET, yytext, &current_interface->gateway);
					current_interface->gateway = *(struct in_addr*)hostinfo->h_addr;
				}

				token = yylex();
			}

			config_add_interface(info,current_interface);
			break;
		}
		case TOK_TOPOLOGY:
		{
			token = yylex();
			if( token != TOK_START ){
				printf("%d:\tMissing {\n", num_lines);
				exit(-1);
			}

			current_topology = (topology_t*) malloc(sizeof(topology_t));	
			token = yylex();
			while ( token != TOK_END ) {
				if( token == TOK_BRIDGE ){
					token = yylex();
					if( token != TOK_START ){
						printf("%d:\tMissing {\n",num_lines);
						exit(-1);
					}

					token = yylex();
					while ( token != TOK_END ) {
						if ( token == TOK_PORT ) {
							current_topology->port = atoi(yytext);
						}
						if ( token == TOK_IPADDRESS ) {
							inet_pton(AF_INET, yytext, &current_topology->address);
						}
						if ( token == TOK_HOSTNAME ) {
							//
						}
				
						token = yylex();
					}

				}
				
				token = yylex();	
			}
			config_add_topology(info,current_topology);
			break;
		}
		case TOK_SWITCH:
		{
			token = yylex();
			if (token!=TOK_START) {
				printf("Config reader :: expecting { at %d, but got %s\n", num_lines, yytext);
			}
			current_topology = malloc(sizeof(*current_topology));
			current_topology->type = TOP_SWITCH;
			INIT_LIST_HEAD(&current_topology->port_list);
			token = yylex();
			while (token != TOK_END) {
				struct dev_list *sw_if = malloc(sizeof(*sw_if));
				if (token != TOK_INTERFACE) {
					printf("Config reader :: expecting interface at %d, but got %s\n", num_lines, yytext);
				}
				token = yylex();
				if (token != TOK_DEV) {
					printf("Config reader :: expecting device name at %d, but got %s\n", num_lines, yytext);
				}
				sw_if->dev = malloc(strlen(yytext)+1);
				memset(sw_if->dev, 0, strlen(yytext)+1);
				strncpy(sw_if->dev, yytext, strlen(yytext));
				INIT_LIST_HEAD(&sw_if->list);
				list_add(&sw_if->list, &current_topology->port_list);
				token = yylex();
			}
			config_add_topology(info,current_topology);
			break;
		}
		case TOK_T_HYPERVISOR:
			token = yylex();
			if (token != TOK_START) {
				printf("Config reader :: expecting { at %d, but found %s\n", num_lines, yytext);
			}

			device_t *device;
			token = yylex();
			while (token != TOK_END) {
				device = malloc(sizeof(*device));
				memset(device, 0, sizeof(*device));
				device->x = 100;
				device->y = 100;
				if (token != TOK_T_DEVICE) {
					printf("Config reader :: expecting device at %d, but got %s\n", num_lines, yytext);
				}
				token = yylex();
				if (token != TOK_START) {
					printf("Config reader :: expecting { at %d, but found %s\n", num_lines, yytext);
				}
				
				token = yylex();
				while (token != TOK_END) {
					if (token == TOK_T_TYPE) {
						token = yylex();
						device->type = get_device_type(yytext);
					} else if (token == TOK_T_PORT) {
						token = yylex();
						device->port = atoi(yytext);
					} else if (token == TOK_T_HOSTNAME) {
						token = yylex();
						device->hostname = malloc(strlen(yytext)+1);
						memset(device->hostname, 0, strlen(yytext)+1);
						memcpy(device->hostname, yytext, strlen(yytext));
					} else if (token == TOK_T_CONFIG) {
						token = yylex();	
						device->config = malloc(strlen(yytext)+1);
						memset(device->config, 0, strlen(yytext)+1);
						memcpy(device->config, yytext, strlen(yytext));
					} else if (token == TOK_T_POSX) {
						token = yylex();
						device->x = atoi(yytext);
					} else if (token == TOK_T_POSY) {
						token = yylex();
						device->y = atoi(yytext);
					} else {
						printf("Config reader :: unexpected input at %d, %s\n", num_lines, yytext);
					}
					token = yylex();
				}
				config_add_device(info, device);
				token = yylex();
			}
			break;
		default:
			printf("Config reader :: error at %d, value found is %s\n", num_lines, yytext);
			break;
		}

		token = yylex();
	}
	return ESUCCESS;
}

int dump_config_file(int fd, conf_info_t *conf)
{
	char buffer[256], address[32];
	struct list_head *head;
	//write general info
	memset(buffer, 0, 256);
	sprintf(buffer, "hostname %s;\nport %d;\nipaddress %s;\n", conf->general.hostname, conf->general.port, 
			inet_ntop(AF_INET, &conf->general.address, address, 32));
	if (write(fd, buffer, strlen(buffer)) < 0) {
		perror("write:");
	}

	//dump interfaces
	list_for_each(head, &conf->interfaces) {
		interface_t *interface = list_entry(head, interface_t, list);
		dump_interface(fd, interface);
	}
	return 0;
}
