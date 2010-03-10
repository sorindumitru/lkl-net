#ifndef CONF_TREE_9Z3VI8KG
#define CONF_TREE_9Z3VI8KG

#include <netinet/in.h>
#include <netinet/ether.h>

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

typedef struct conf_tree{
	int type;
	void*	data;
	int nr_children;
	struct conf_tree **children;
} conf_tree_t;

typedef struct interface{
	struct eth_addr	*mac;
	struct in_addr address, gateway;
	unsigned int netmask_len;
} interface_t;

typedef struct topology{
	unsigned char type;
	struct in_addr address;
	unsigned short port;
} topology_t;


extern int config_init(conf_tree_t* conf);
extern int config_free(conf_tree_t* conf);
//extern int config_read(struct conf_tree_t* conf, int file_descriptor);
extern int config_read_file(conf_tree_t* conf, const char* file_name);

#endif /* CONF_TREE_9Z3VI8KG */
