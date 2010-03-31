#ifndef TOPOLGY_DHTGB6RP
#define TOPOLGY_DHTGB6R

#include <list.h>

struct dev_list{
	char *dev;
	struct list_head list;
};

struct in_addr;

typedef struct topology{
	unsigned char type;
	union{
		struct {
			struct in_addr address;
			unsigned short port;
			char *dev;
		};
		struct list_head port_list;
	};

	struct list_head list;
} topology_t;

/**
 * Topologies:
 */
#define TOP_BRIDGE	1
#define TOP_LINK	2
#define TOP_SWITCH	3

topology_t* alloc_topology();

#endif /* TOPOLGY_DHTGB6RP */
