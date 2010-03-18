#ifndef PACKET_CA2K9INK
#define PACKET_CA2K9INK

#define MAX_PACKET_SIZE 8192

typedef struct packet {
	unsigned int size;
	char *data;
} packet_t;

packet_t* alloc_packet(unsigned int size, char *data);
void free_packet(packet_t* packet);
int send_packet(int to, packet_t* packet);
int send_packet_native(int to, packet_t* packet);
packet_t* recv_packet(int from);
packet_t* recv_packet_native(int from);

#endif /* PACKET_CA2K9INK */
