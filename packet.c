#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <packet.h>

packet_t* alloc_packet(unsigned int size, char* data)
{
	packet_t* packet = malloc(sizeof(*packet));
	
	packet->size = size;
	packet->data = data;

	return packet;
}

void free_packet(packet_t* packet)
{
	free(packet->data);
	free(packet);
}

int send_packet(int to, packet_t *packet)
{
	int err;

	if (packet->size > MAX_PACKET_SIZE) {
		packet->size == MAX_PACKET_SIZE;
		printf("Packet too big!\n");
	}
	
	err = send(to, &packet->size, sizeof(packet->size), 0);
	if (err < 0) {
		perror("send_packet:send size:");
		return err;
	}

	err = send(to, packet->data, packet->size, 0);
	if (err < 0) {
		perror("send_packet:send_data:");
		return err;
	}

	return err;//number of written bytes not error
}

int send_packet_native(int to, packet_t* packet)
{
	int err;

	err = write(to, packet->data, packet->size);
	if (err < 0) {
		perror("send_packet_native:send_data:");
		return err;
	}

	return err;
}

packet_t* recv_packet(int from)
{
	int size;
	char *data;
	int err;

	err = read(from, &size, sizeof(size));
	if (err < 0) {
		return NULL;
	}

	data = malloc(size);
	err = read(from, data, size);
	if (err < 0) {
		return NULL;
	}

	if (size > MAX_PACKET_SIZE) {
		printf("Packet to big!\n");
		size = MAX_PACKET_SIZE;
	}

	return alloc_packet(size, data);
}

packet_t* recv_packet_native(int from)
{
	int size;
	char *data = malloc(MAX_PACKET_SIZE);

	size = read(from, data, MAX_PACKET_SIZE);
	if (size < 0) {
		return NULL;
	}

	return alloc_packet(size, data);
}
