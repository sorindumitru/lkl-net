#include <topology.h>

topology_t* alloc_topology(){
	topology_t* topology = malloc(sizeof(topology_t));

	return topology;
}
