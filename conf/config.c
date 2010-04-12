#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <tokens.h>
#include <codes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <interface.h>

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
	return ESUCCESS;
}

int config_free(conf_info_t* tree)
{
	//TODO	
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

int config_read_file(conf_info_t* info, const char* file_name)
{
	int token = -1;
	interface_t *current_interface;
	topology_t  *current_topology;

	yyin = fopen(file_name, "r");
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
		case TOK_INTERFACE:
		{
			token = yylex();
			if ( token != TOK_START ) {
				printf("%d:\tMissing {\n",num_lines);
				exit(-1);
			}
			current_interface = malloc(sizeof(interface_t));
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
					current_interface->mac = (struct eth_addr*) ether_aton(yytext);
				}
				if ( token == TOK_T_GATEWAY ) {
					token = yylex();
					if ( token != TOK_IPADDRESS ) {
						printf("Config reader :: expecting IPv4 address at %d, but got %s\n", num_lines, yytext);
					}
					struct hostent *hostinfo =gethostbyname(yytext);
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
		default:
			printf("Error in config file\n");
			break;
		}

		token = yylex();
	}
	return ESUCCESS;
}
