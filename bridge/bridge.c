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
#include <netinet/in.h>
#include <netpacket/packet.h>

#include <net/if.h>
#include <linux/if_tun.h>

#include <conf_tree.h>

#define BUFFSIZE 8192
char buffer[BUFFSIZE];

char interface[IFNAMSIZ];

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

int dev_index(char* dev){
	struct ifreq ifr;

	int index =1;
	int sock = socket(PF_PACKET, SOCK_RAW, 0);
	ifr.ifr_ifindex = index;
	while ( ioctl(sock, SIOCGIFNAME, &ifr) == 0 ) {
		if ( strcmp(ifr.ifr_name, dev) == 0 ) {
			close(sock);
			return index;
		}
		ifr.ifr_ifindex = ++index;
	}
	close(sock);
	return -1;	 
}

int main(int argc, const char *argv[])
{
	int host, lkl, maxfd = -1, result;
	int port;
	int err;
	unsigned int addr_len;
	struct in_addr address;
	struct sockaddr_ll saddr;
	fd_set working, readset;

	//read config file
	conf_info_t* tree = malloc(sizeof(conf_info_t));
	config_init(tree);
	config_read_file(tree,argv[1]);//should parse arguments
	
	topology_t* topo;
	struct list_head* head;
	list_for_each(head, &tree->topologies){
		topo=list_entry(head, topology_t, list);
	}
	port = topo->port;
	address = topo->address;

	printf("::Starting\n");
	host = init_host_interface(interface);	
	if ( host < 0 ) {
		perror("initializing host interface:");
		return -1;
	}
	if( host > maxfd )
		maxfd = host;

	printf("::Initialized TUN/TAP interface %s\n",interface);

	lkl = socket(PF_INET, SOCK_RAW, htons(ETH_P_ALL));
	/*saddr.sin_port = htons(port);
	saddr.sin_addr = address;
	saddr.sin_family = PF_INET;
	if ( (err=connect(lkl, (struct sockaddr*) &saddr, sizeof(saddr))) < 0 ) {
		perror("connect:");
		return -1;
	}*/
	saddr.sll_family = AF_INET;
	saddr.sll_halen = 6;
	saddr.sll_ifindex = dev_index("eth0");
	if( lkl > maxfd )
		maxfd = lkl;
	
	printf("::Connected\n");

	FD_ZERO(&working);
	FD_SET(lkl,&working);
	FD_SET(host,&working);

	int nr_bytes;
	while(1){
		memcpy(&readset, &working, sizeof(working));
		result = select(maxfd+1, &readset, 0, 0, NULL); 

		if( FD_ISSET(lkl, &readset) ) {
			nr_bytes = recvfrom(lkl, buffer, BUFFSIZE, 0, (struct sockaddr*) &saddr, &addr_len);
			write(host, buffer, nr_bytes);
			printf("recv\n");
		}

		if( FD_ISSET(host, &readset) ) {
			nr_bytes = read(host, buffer, BUFFSIZE);
			printf("got data from host %d\n", nr_bytes);
			nr_bytes = sendto(lkl, buffer, nr_bytes, 0, (struct sockaddr*) &saddr, sizeof(saddr));
			//nr_bytes = send(lkl,buffer,nr_bytes,0);
			if( nr_bytes < 0 ) {
				perror("sendto:");
				exit(-1);
			}
			printf("sent %d\n",nr_bytes);
		}
	}

	return 0;
}
