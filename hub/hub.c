#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <list.h>
#include <string.h>

#define PORT_NO 10000
#define PACKET_SIZE 8192 

typedef struct connection{
	int sock;
	struct list_head conn_list;
}connection;

typedef struct result{
	int size;
	char *message;
} result;

struct list_head my_conn;
LIST_HEAD(my_conn);

struct result *modify_packet(char *packet, int size)
{
	struct result *myres = malloc(sizeof(struct result));
	myres->size = size;
	myres->message = packet; 
	return myres;
}

void forward_packet(int fd, char *data, int size)
{
	struct list_head *lh;
	struct connection *conn;
	list_for_each(lh,&my_conn){
		conn=list_entry(lh,struct connection,conn_list);
		if(conn->sock != fd){
			printf("tb sa trimit lui %d msg=%s\n",conn->sock, data);
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
	char *buf;
	struct list_head *lh;
	struct connection *conn;
	lh=(struct list_head*)malloc(sizeof(struct list_head));
	conn=(struct connection*)malloc(sizeof(struct connection));
	
	buf=(char*)malloc(PACKET_SIZE);

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
			if(n>0){//TO DO: closed conn; not n==0
				printf("hub:primit datele #%s# de la %d\n",buf,ret_ev.data.fd);
				n=recv(ret_ev.data.fd, buf, size, 0);
				if (n == 0) {
					epoll_ctl(epfd, EPOLL_CTL_DEL, ret_ev.data.fd, &ev);
					remove_connection(ret_ev.data.fd);
				}
				res=modify_packet(buf,size);
				//printf("asta ajunge:%d\t%s\n",res->size,res->message);
				forward_packet(ret_ev.data.fd, res->message, res->size);
			} else{
				printf("received a fin from %d\n",ret_ev.data.fd);
				epoll_ctl(epfd, EPOLL_CTL_DEL,ret_ev.data.fd, &ev);
				remove_connection(ret_ev.data.fd); //remove connection from the connections list
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
