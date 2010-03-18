#ifndef CONF_TREE_9Z3VI8KG
#define CONF_TREE_9Z3VI8KG

#include <list.h>
#include <interface.h>
#include <topology.h>

#define ROOT 1024
#define	INTERFACE 1025
#define	ADDRESS 1026
#define	GATEWAY 1027
#define	MAC 1028
#define	NETMASK 1029
#define	DEV 1030
#define	TOPOLOGY 1031
#define	BRIDGE 1032
#define PORT 1033

typedef struct gen_info{
	char* hostname;
	unsigned short port;
} gen_info_t;

typedef struct conf_info{
	gen_info_t info;
	struct list_head interfaces;
	struct list_head topologies;
} conf_info_t;

extern int config_init(conf_info_t* conf);
extern int config_free(conf_info_t* conf);
//extern int config_read(struct conf_tree_t* conf, int file_descriptor);
extern int config_read_file(conf_info_t* conf, const char* file_name);

#endif /* CONF_TREE_9Z3VI8KG */
