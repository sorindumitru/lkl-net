#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <list.h>
#include <string.h>
#include <config.h>

#define PORT_NO 10000
#define PACKET_SIZE 4096

#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]

#define NIPQUAD_FMT "%u.%u.%u.%u"


typedef struct connection{
	int sock;
	struct list_head conn_list;
}connection;

typedef struct result{
	int size;
	unsigned char *message;
} result;

struct list_head my_conn;
LIST_HEAD(my_conn);

typedef struct eth_header {
	unsigned char dest[6];
	unsigned char src[6];
	unsigned short protocol;
} eth_header;

typedef struct ip_header {
	unsigned char ihl:4,
		      version:4;
	unsigned char tos;
	unsigned short total_length;
	unsigned short id;
	unsigned short flags:3,
		       fragment_offset:13;
	unsigned char ttl;
	unsigned char protocol;
	unsigned short hdr_checksum;
	unsigned int saddr;
	unsigned int daddr;
	//unsigned char options[0];
} ip_header;   

typedef struct arp_header {
	unsigned short hdw_type;
	unsigned short prot_type;
	unsigned char hdw_addr_len;
	unsigned char prot_addr_len;
	unsigned short opcode;
	unsigned char sender_hdwaddr[6];
	unsigned int sender_protaddr;
	unsigned char target_hdwaddr[6];
	unsigned int target_protaddr;
} arp_header;	
  
static unsigned short dump_eth_header(eth_header *e)
{
	printf("Ethernet:\n\
		\tDestination:\t%02X:%02X:%02X:%02X:%02X:%02X\n\
		\tSource:\t%02X:%02X:%02X:%02X:%02X:%02X\n\
		\tProtocolt: %u\n",\
	(unsigned int)e->dest[0], (unsigned int)e->dest[1], (unsigned int)e->dest[2], (unsigned int)e->dest[3], (unsigned int)e->dest[4], (unsigned int)e->dest[5],\
	(unsigned int)e->src[0], (unsigned int)e->src[1], (unsigned int)e->src[2], (unsigned int)e->src[3], (unsigned int)e->src[4], (unsigned int)e->src[5],ntohs(e->protocol));

	return ntohs(e->protocol);
}

static void dump_ip_header(ip_header *i)
{

	printf("IP:\n\tihl=%u\tversion=%u\ttos=%u\n\
	\ttotal_length=%u\tid=%u\tflags=%u\n\
	\tfragment_offset=%u\tTTL=%u\tprotocol=%u\n\
	\tDestination:\t"NIPQUAD_FMT"\n\
	\tSource:"NIPQUAD_FMT"\n",\
	i->ihl,i->version,i->tos,\
	ntohs(i->total_length),ntohs(i->id),ntohs(i->flags),\
	ntohs(i->fragment_offset),i->ttl,i->protocol,\
	NIPQUAD(i->daddr),NIPQUAD(i->saddr));

}

static void dump_arp_header(arp_header *arp)
{
	
	printf("ARP:\n\thdw_type=%u\t \
	prot_type=%u\t	\
	hdw_addr_len=%d\t	\
	\n\tprot_addr_len=%d\
	\topcode=%u\t	\
	\n\tsender_hdwaddr=%02X:%02X:%02X:%02X:%02X:%02X	\
	\n\tsender_protaddr=" NIPQUAD_FMT"	\
	\n\ttarget_hdwaddr=%02X:%02X:%02X:%02X:%02X:%02X	\
	\n\ttarget_protaddr="NIPQUAD_FMT,	\
	arp->hdw_type, arp->prot_type, arp->hdw_addr_len, \
	arp->prot_addr_len, arp->opcode,\
	(unsigned int)arp->sender_hdwaddr[0],(unsigned int)arp->sender_hdwaddr[1],(unsigned int)arp->sender_hdwaddr[2],(unsigned int)arp->sender_hdwaddr[3],(unsigned int)arp->sender_hdwaddr[4],(unsigned int)arp->sender_hdwaddr[5],\
	NIPQUAD(arp->sender_protaddr),\
	arp->target_hdwaddr[0],arp->target_hdwaddr[1],arp->target_hdwaddr[2],arp->target_hdwaddr[3],arp->target_hdwaddr[4],arp->target_hdwaddr[5],\
	NIPQUAD(arp->target_protaddr));
}

struct result *modify_packet(unsigned char *packet, int size)
{
	struct result *myres = malloc(sizeof(struct result));
	myres->size = size;
	myres->message = packet; 
	return myres;
}

conf_info_t *info = NULL;

void forward_packet(int fd, unsigned char *data, int size)
{
	struct list_head *lh;
	struct connection *conn;
	list_for_each(lh,&my_conn){
		conn=list_entry(lh,struct connection,conn_list);
		if(conn->sock != fd){
			//printf("tb sa trimit lui %d msg=%s\n",conn->sock, data);
			if(send(conn->sock, &size, sizeof(size), 0) < 0 ){
				perror("error:send:\n");
				exit(-1);
			}
			if(send(conn->sock, data, size, 0) < -1){
				perror("error:send:");
				exit(-1);
			}
		}
	}
	return;	
}

void remove_connection(int fd)
{	
	struct list_head *lh,*aux;
	struct connection *conn;
	list_for_each_safe(lh,aux,&my_conn){
		conn=list_entry(lh,struct connection,conn_list);
		if(conn->sock == fd){
			list_del(lh);
			return;
		}
	}
}

/*void print_data_hexa(int size, unsigned char *buff)
{
	int i;
	printf("NEW DATA::START\n");
	for(i=0;i<size;i++){
		printf("%02x",buff[i]);
	}
	printf("\nNEW DATA::END\n");
}*/
void dump_header(unsigned char *buf)
{
	unsigned short prot_type;
	prot_type = dump_eth_header((struct eth_header*)buf);
	if (prot_type == 2048)//IP header
		dump_ip_header((struct ip_header*)(buf+14));
	else if (prot_type ==2054 )//ARP header
		dump_arp_header((struct arp_header*)(buf+14));	
	printf("\n===============================================\n");
}

void wait_for_messages( int port_no )
{
	int sockfd,newsockfd;
	socklen_t len;
	struct sockaddr_in ip4addr;
	struct sockaddr_in ll_addr;
	int yes = 1;//reuse port

	int size;

	struct connection *nc;
	struct epoll_event ev;
	int epfd;

	struct result *res;
	int n;
	unsigned char *buf;
	struct list_head *lh;
	struct connection *conn;
	lh=(struct list_head*)malloc(sizeof(struct list_head));
	conn=(struct connection*)malloc(sizeof(struct connection));
	
	buf=(unsigned char*)malloc(PACKET_SIZE);

	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 6);
	if (sockfd < 0){
		perror("Error creating socket");
		exit(-1);
	}
	//bind socket to port number port_no on all interfaces
	ip4addr.sin_family = AF_INET;
	ip4addr.sin_port = htons(port_no);
	ip4addr.sin_addr.s_addr = INADDR_ANY; /* all interfaces */
	if(bind(sockfd, (struct sockaddr*)&ip4addr, sizeof ip4addr) != 0){
		perror("Error bind");
		exit(-1);
	}
	
	//reuse port
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1){ 
		perror("setsockopt");
		exit(-1);
	}
	
	//wait for connections
	if(listen(sockfd,128) != 0){
		perror("ERROR Listen");
		exit(-1);
	}
	
	// create epoll descriptor 
	epfd = epoll_create(2);
	ev.data.fd = sockfd;        
	ev.events = EPOLLIN;// | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	while(1){        // server loop 
		//printf("here i go\n");
    		struct epoll_event ret_ev;
    		epoll_wait(epfd, &ret_ev, 1, -1);// wait for readiness notification 
    		if (ret_ev.data.fd == sockfd && ((ret_ev.events & EPOLLIN) != 0) ) {
			printf("new connection\n");
			memset(buf,0,PACKET_SIZE);
			newsockfd=accept(sockfd,(struct sockaddr*)&ll_addr,&len);
			//save the new connection
			printf("adaug in lista %d\n",newsockfd);
			nc=(struct connection *)malloc(sizeof(struct connection));
			nc->sock=newsockfd;
			INIT_LIST_HEAD(&nc->conn_list);
			list_add(&nc->conn_list,&my_conn);
			//events
			ev.data.fd = newsockfd;        
			ev.events = EPOLLIN;// | EPOLLET;
			epoll_ctl(epfd, EPOLL_CTL_ADD, newsockfd, &ev);
		}else {
			memset(buf,0,PACKET_SIZE);
			n=recv(ret_ev.data.fd, &size, sizeof(size), 0);
			//printf("!!!!!!!!!!!!!!!!!N=%d\n",n);
			if(n>0){
				memset(buf,0,PACKET_SIZE);
				//printf("hub:primit datele #%s# de la %d\n",buf,ret_ev.data.fd);
				n=recv(ret_ev.data.fd, buf, size, 0);
				//print_data_hexa(size,buf);
				res=modify_packet(buf,size);
				dump_header(buf);
				forward_packet(ret_ev.data.fd, res->message, res->size);
				/*if (n<=14){
					//length packet
					printf("LENGTH PACKET\n");
					forward_packet(ret_ev.data.fd, res->message, res->size);
				}else if(n>14){
					dump_eth_header((struct eth_header*)buf+sizeof(int));
					res=modify_packet(buf,size);
					//printf("asta ajunge:%d\t%s\n",res->size,res->message);
					forward_packet(ret_ev.data.fd, res->message, res->size);
				}*/
			} else{
				printf("received a fin from %d\n",ret_ev.data.fd);
				memset(buf,0,PACKET_SIZE);
				n=recv(ret_ev.data.fd, buf, size, 0);
				remove_connection(ret_ev.data.fd); //remove connection from the connections list
				shutdown(ret_ev.data.fd,2);
				epoll_ctl(epfd, EPOLL_CTL_DEL,ret_ev.data.fd, &ev);
			}
			
    		}
	}
}


int main(int argc, char** argv)
{
	printf("LKL NET :: hub started on %d\n", atoi(argv[1]));
	//conf_info_t *info = malloc(sizeof(*info));
	//config_init(info);

	wait_for_messages(atoi(argv[1]));
	return 0;
}
