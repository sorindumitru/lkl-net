/**
 * Bridge from host network to LKL network
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <packet.h>
#include <net/if.h>
#include <linux/if_tun.h>

#include <config.h>

#define BUFFSIZE 8192
char buffer[BUFFSIZE];
char interface[IFNAMSIZ];

conf_info_t *info;

int init_host_interface(char* dev)
{
	int device, err;
	struct ifreq ifr;

	if ( (device=open("/dev/net/tun",O_RDWR)) < 0) {
		perror("open /dev/net/tun:");
		return device;
	}
	
	memset(&ifr,0,sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if ( *dev ) {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if ( (err=ioctl(device, TUNSETIFF, (void*) &ifr)) < 0 ) {
		perror("ioctl:");
		close(device);
		return err;
	}

	strcpy(dev,ifr.ifr_name);
	
	return device;
}

int main(int argc, char **argv)
{
	int err;
	topology_t* topology;
	struct list_head *head;

	int port;
	struct in_addr address;

	int host, lkl, epoll_listen;
	struct sockaddr_in saddr;
	struct epoll_event ev_lkl, ev_host;

	printf("::Bridge v1.0\n");

	printf("::Reading config file\n");
	info = malloc(sizeof(*info));
	config_init(info);
	config_read_file(info, argv[1]);
	list_for_each(head, &info->topologies){
		topology = list_entry(head, topology_t, list);
	}

	port = topology->port;
	address = topology->address;
	printf("::Init host interface\n");
	host = init_host_interface(interface);
	if (host < 0) {
		perror("init_host_interface:");
		return -1;
	}
	
	printf("::Initialized TUN/TAP interface %s\n",interface);
	
	lkl = socket(AF_INET, SOCK_STREAM, 0);
	if (lkl < 0 ) {
		perror("socket:");
		return -1;
	}
	saddr.sin_port = htons(port);
	saddr.sin_addr = address;
	saddr.sin_family = PF_INET;
	if ((err=connect(lkl, (struct sockaddr*) &saddr, sizeof(saddr))) < 0) {
		perror("connect:");
		return -1;
	}

	epoll_listen = epoll_create(2);
	if ( epoll_listen < 0 ) {
		perror("epoll:");
		exit(-1);
	}
	ev_lkl.data.fd = lkl;
	ev_lkl.events = EPOLLIN;
	ev_host.data.fd = host;
	ev_host.events = EPOLLIN;
	epoll_ctl(epoll_listen, EPOLL_CTL_ADD, lkl, &ev_lkl);
	epoll_ctl(epoll_listen, EPOLL_CTL_ADD, host, &ev_host);

	while (1) {
		struct epoll_event ret_ev;

		epoll_wait(epoll_listen, &ret_ev, 1, -1);

		if (ret_ev.data.fd == lkl && ((ret_ev.events & EPOLLIN) != 0)) {
			packet_t *packet = recv_packet(lkl);
			if (!packet) {
				continue;
			}
			err =send_packet_native(host, packet);
			if (err < 0) {
				continue;
			}
			free_packet(packet);
		}

		if (ret_ev.data.fd == host && ((ret_ev.events & EPOLLIN) != 0)) {
			packet_t *packet = recv_packet_native(host);		
			if (!packet) {
				continue;
			}
			err = send_packet(lkl, packet);
			if (err < 0) {
				continue;
			}
			free_packet(packet);	
		}
	}
}
