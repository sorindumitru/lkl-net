#include <stdio.h>
#include <stdlib.h>
#include <conf_tree.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, const char *argv[])
{
	conf_tree_t* tree = (conf_tree_t*) malloc(sizeof(conf_tree_t));
	config_init(tree);
	config_read_file(tree,argv[1]);

	conf_tree_t* topology = *tree->children;
	struct topology* bridge = (struct topology*)topology->data;
	printf("Port = %d\n", bridge->port);
	printf("Address = %s\n", inet_ntoa(bridge->address));
	return 0;
}
