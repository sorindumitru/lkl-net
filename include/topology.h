#ifndef TOPOLGY_DHTGB6RP
#define TOPOLGY_DHTGB6R

typedef struct topology{
	unsigned char type;
	struct in_addr address;
	unsigned short port;
	char* dev;

	struct list_head list;
} topology_t;

/**
 * Topologies:
 */
#define TOP_BRIDGE	1
#define TOP_LINK	2

topology_t* alloc_topology();

#endif /* TOPOLGY_DHTGB6RP */
