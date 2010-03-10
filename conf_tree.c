#include <stdio.h>
#include <stdlib.h>
#include <conf_tree.h>
#include <tokens.h>
#include <codes.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

extern char* yytext;
extern FILE* yyin;

static int max_children = 8;

int config_init(conf_tree_t* tree)
{
	tree->type = ROOT;
	tree->nr_children = -1;
	return ESUCCESS;
}

int config_free(conf_tree_t* tree)
{
	
	return ESUCCESS;
}

static int conf_tree_add_child(conf_tree_t* tree, conf_tree_t* child){
	if( tree->nr_children == -1 ) {
		tree->children = (conf_tree_t**) malloc(max_children*sizeof(conf_tree_t*));
		tree->nr_children++;
	}
	if( (tree->nr_children +1)>=max_children ) {
		//realloc childrent
	}
	tree->children[tree->nr_children] = child;
	tree->nr_children++;

	return ESUCCESS;
}

int config_read_file(conf_tree_t* tree, const char* file_name)
{
	int token = -1;
	int i;
	conf_tree_t *current_tree;
	interface_t *current_interface;
	topology_t  *current_topology;

	yyin = fopen(file_name, "r");

	token = yylex();
	while ( token ) {
		switch( token ){
		case TOK_INTERFACE:
			current_interface = (interface_t*) malloc(sizeof(interface_t));
			token = yylex();
			for (i = 0; i < 4; i++) {
				token = yylex();
				//TODO: get interface data
				if ( token == TOK_IPADDRESS ) {
					//current_interface->address = 0;
				}
			}
			break;
		case TOK_TOPOLOGY:
		{
			token = yylex();
			if( token != TOK_START ){
				//TODO:error
			}

			current_topology = (topology_t*) malloc(sizeof(topology_t));
			
			token = yylex();
			if( token == TOK_BRIDGE ){
				token = yylex();
				if( token != TOK_START ){
					//TODO: error
				}

				for (i = 0; i < 2; i++) {
					token = yylex();
					if( token == TOK_PORT ) {
						current_topology->port = atoi(yytext);
					}
					if ( token == TOK_IPADDRESS ) {
						inet_pton(AF_INET, yytext, &current_topology->address);
						/*struct hostent *hostinfo =gethostbyname(yytext);
						current_topology->address = *(struct in_addr*) hostinfo->h_addr;*/
					}
				}

				token = yylex();
				if ( token != TOK_END ) {
					//TODO: error
				}
			}
			
			current_tree = (conf_tree_t*) malloc(sizeof(conf_tree_t));
			current_tree->type = TOPOLOGY;
			current_tree->data = (void*) current_topology;
			conf_tree_add_child(tree, current_tree);

			token = yylex();
			if( token!=TOK_END ) {
				//TODO:error
			}
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
