#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <argp.h>
#include <netdb.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

#include <asm/env.h>
#include <asm/eth.h>
//
#include <asm/poll.h>


#define PORT_NO 60000
#define MAX_PFDS 32
/**
 * Program arguments
 */
const char *argp_program_version = "lkl-ping::0.1";
const char *argp_program_bug_address = "sorin.dumitru@cti.pub.ro";
static char doc[] = "Ping-over-lkl";
static char args_doc[] = "";
static struct argp_option options[] = {
	{"interface", 'i', "string", 0, "native interface to use:"},
	{"mac", 'm', "mac address", 0,
	 "MAC address for the lkl interface:"},
	{"address", 'a', "IPv4 address", 0,
	 "IPv4 address for the lkl interface:"},
	{"netmask-length", 'n', "int", 0,
	 "IPv4 netmask length for the lkl interface:"},
	{"gateway", 'g', "IPv4 address", 0, "IPv4 gateway for lkl:"},
	{"default", 'd', "IPv4 address", 0, "IPv4 default root"},
	{"lkl", 'l', 0, 0, "Use LKL:"},
	{"port", 'p', "int", 0, "Port"},
	{"role", 'r', "string", 0, "server/client"},
	{"to", 't', "Destination IPv4 address", 0, "IPv4 destination"},
	{0},

};

typedef struct ping_args {
	struct ether_addr *mac;
	const char *iface, *request, *role;
	struct in_addr address, def, gateway, host;
	unsigned int netmask_len, port, lkl;
} ping_args_t;

static ping_args_t cla = {
	.port = 50000,
};



static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	ping_args_t *cla = state->input;
	switch (key) {
	case 'm':
		{
			cla->mac = ether_aton(arg);
			if (!cla->mac) {
				printf("bad MAC address: %s\n", arg);
				return -1;
			}
			break;
		}
	case 'a':
		{
			struct hostent *hostinfo = gethostbyname(arg);
			if (!hostinfo) {
				printf("unknown host %s\n", arg);
				return -1;
			}
			cla->address =
			    *(struct in_addr *) hostinfo->h_addr;
			break;
		}
	case 'd':
		{
			struct hostent *hostinfo = gethostbyname(arg);
			if (!hostinfo) {
				printf("unknown host %s\n", arg);
				return -1;
			}
			cla->def =
			    *(struct in_addr *) hostinfo->h_addr;
			break;
		}
	case 'g':
		{
			struct hostent *hostinfo = gethostbyname(arg);
			if (!hostinfo) {
				printf("unknown host %s\n", arg);
				return -1;
			}
			cla->gateway =
			    *(struct in_addr *) hostinfo->h_addr;
			break;
		}
	case 'n':
		{
			cla->netmask_len = atoi(arg);
			if (cla->netmask_len <= 0
			    || cla->netmask_len >= 31) {
				printf("bad netmask length %d\n",
				       cla->netmask_len);
				return -1;
			}
			break;
		}
	case 'p':
		{
			cla->port = atoi(arg);
			if (cla->port <= 0) {
				printf("bad port number %d\n", cla->port);
				return -1;
			}
			break;
		}
	case 't':
		{
			struct hostent *hostinfo = gethostbyname(arg);
			if (!hostinfo) {
				printf("unknown host %s\n", arg);
				return -1;
			}
			cla->host = *(struct in_addr *) hostinfo->h_addr;
			break;
		}
	case 'i':
		{
			if (if_nametoindex(arg) < 0) {
				printf("invalid interface: %s\n", arg);
				return -1;
			}
			cla->iface = arg;
			break;
		}
	case 'l':
		{
			cla->lkl = 1;
			break;
		}
	case 'r':
		{
			cla->role = arg;
			break;
		}
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

/* end program arguments */

int do_connect(int fd, const struct sockaddr *saddr, socklen_t slen)
{
	if (cla.lkl)
		return lkl_sys_connect(fd, (struct sockaddr *) saddr,
				       slen);
	else
		return connect(fd, saddr, slen);
}

int do_socket(int family, int type, int protocol)
{
	if (cla.lkl)
		return lkl_sys_socket(family, type, protocol);
	else
		return socket(family, type, protocol);
}

ssize_t do_read(int fd, void *buf, size_t len)
{
	if (cla.lkl)
		return lkl_sys_read(fd, buf, len);
	else
		return read(fd, buf, len);
}

ssize_t do_write(int fd, const void *buf, size_t len)
{
	if (cla.lkl)
		return lkl_sys_write(fd, buf, len);
	else
		return write(fd, buf, len);
}

static int do_full_write(int fd, const char *buffer, int length)
{
	int n, todo = length;

	while (todo && (n = do_write(fd, buffer, todo)) >= 0) {
		todo -= n;
		buffer += n;
	}

	if (!todo)
		return length;
	else
		return n;
}

#define get_error(err) (cla.lkl?-err:errno)

/** 
 * @brief Simple ping implementation on LKL.
 * First implementation will just send a message and
 * see if it receives a reply. We'll see later what we 
 * we can do about ICMP.
 * 
 * @param argc Number of arguments
 * @param argv The arguments 
 * 
 * @return 
 */
int lkl_ping(int argc, char **argv)
{
	struct tun_device* td = malloc(sizeof(struct tun_device));
	int err, sock;
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
	};
	char req[1024], buffer[4096];

	if (argp_parse(&argp, argc, argv, 0, 0, &cla) < 0)
		return -1;

		td->port = cla.port;
		td->type = TUN_HUB;
		td->address = cla.gateway.s_addr;
		td->port = cla.port;
		td->type = TUN_HUB;
		td->address = cla.gateway.s_addr;

	if (cla.lkl) {
		int ifindex;

		if (!cla.iface || !cla.mac || !cla.netmask_len ||
		    !cla.address.s_addr || !cla.gateway.s_addr) {
			printf
			    ("lkl mode and no interface, mac, address, netmask length or gateway specified!\n");
			return -1;
		}

		if (lkl_env_init(16 * 1024 * 1024) < 0)
			return -1;
			printf("MAC : %s\n", ether_ntoa(cla.mac));	
		if ((ifindex =
		     lkl_add_eth_tun(cla.iface, (char *) cla.mac, 32, td)) == 0)
			return -1;

		if ((err =
		     lkl_if_set_ipv4(ifindex, cla.address.s_addr,
				     cla.netmask_len)) < 0) {
			printf("failed to set IP: %s/%d: %s\n",
			       inet_ntoa(cla.address),
			       cla.netmask_len, strerror(-err));
			return -1;
		}

		if ((err = lkl_if_up(ifindex)) < 0) {
			printf
			    ("failed to bring interface up: %s\n",
			     strerror(-err));
			return -1;
		}

		if ((err = lkl_set_gateway(cla.def.s_addr))) {
			printf("failed to set gateway %s: %s\n",
			       inet_ntoa(cla.address), strerror(-err));
			return -1;
		}
	}

	if ((sock = do_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("can't create socket: %s\n",
		       strerror(get_error(sock)));
		return -1;
	}

	saddr.sin_port = htons(PORT_NO);
	saddr.sin_addr = cla.host;

	if (strcmp(cla.role, "server") == 0) {
		int sd_current;
		socklen_t addrlen;
		struct sockaddr_in pin;
		char msg[4026];

		saddr.sin_addr.s_addr = htonl(INADDR_ANY);

		/* bind the socket to the port number */
		if (lkl_sys_bind
		    (sock, (struct sockaddr *) &saddr,
		     sizeof(saddr)) == -1) {
			perror("bind");
			exit(1);
		}

		printf("bind\n");

		/* show that we are willing to listen */
		if (lkl_sys_listen(sock, 10) == -1) {
			perror("listen");
			exit(1);
		}

		printf("listen\n");

		/* wait for a client to talk to us */
		addrlen = sizeof(pin);
		if ((sd_current =
		     lkl_sys_accept(sock, (struct sockaddr *) &pin,
				    &addrlen)) == -1) {
			perror("accept");
			exit(1);
		}

		/* if you want to see the ip address and port of the client, uncomment the 
		   next two lines */


		printf("Hi there, from  %s#\n", inet_ntoa(pin.sin_addr));
		printf("Coming from port %d\n", ntohs(pin.sin_port));


		/* get a message from the client */
		if (do_read(sd_current, msg, 1024) == -1) {
			perror("recv");
			exit(1);
		}

		memset(msg, sizeof(msg), 0);
		sprintf(msg, "%s", msg);
		if (do_write(sd_current, msg, strlen(msg)) == -1) {
			perror("send");
			exit(1);
		}

		/* close up both sockets */
		close(sd_current);
		close(sock);
		//
	} else {
		if ((err =
		     do_connect(sock, (struct sockaddr *) &saddr,
				sizeof(saddr))) < 0) {
			printf("can't connect to %s:%u: %s\n",
			       inet_ntoa(cla.host),
			       ntohs(saddr.sin_port),
			       strerror(get_error(err)));
			return -1;
		}

		snprintf(req, sizeof(req), "%s", "Hello");

		if ((err = do_full_write(sock, req, sizeof(req))) < 0) {
			printf("can't write: %s\n",
			       strerror(get_error(err)));
			return -1;
		}
#define SAMPLE_RATE 1
		unsigned long total = 0, last_time =
		    time(NULL), last_bytes = 0;
		while ((err = do_read(sock, buffer, sizeof(buffer))) > 0) {
			write(1, buffer, err);
			total += err;
			if (time(NULL) - last_time >= SAMPLE_RATE) {
				printf("%ld %ld\n", total,
				       (total - last_bytes) / SAMPLE_RATE);
				last_time = time(NULL);
				last_bytes = total;
			}
		}
	}

	if (cla.lkl)
		lkl_sys_halt();

	return 0;
}

int main(int argc, char **argv)
{
	lkl_ping(argc,argv);

	return 0;
}
