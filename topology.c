#include <stdlib.h>
#include <netinet/in.h>
#include <topology.h>

topology_t* alloc_topology(){
	topology_t* topology = malloc(sizeof(*topology));

	return topology;
}
