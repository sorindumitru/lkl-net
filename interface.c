#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <interface.h>
#include <linux/if.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <linux/sockios.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



interface_t* alloc_interface()
{
	interface_t* interface = malloc(sizeof(interface_t));
	interface->address.s_addr = 0;
	return interface;
}

/*int get_interface_index(char *ifname)
{
	int err, sock = lkl_sys_socket(PF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;
	int ifname_siz;
	memset(&ifr,0,sizeof(struct ifreq));
	if (sock < 0){
		printf("LKL init :: change interface name negative value for \n");
		return -sock;
	}
	ifname_siz = (IFNAMSIZ > strlen(ifname))?strlen(ifname):IFNAMSIZ;
	memcpy(ifr.ifr_name,ifname,ifname_siz);
	err=lkl_sys_ioctl(sock, SIOCGIFINDEX, (long)&ifr);
	if (err < 0){
		printf("LKL init :: could not get if index NO SUCK INTERFACE NAME %s\n",strerror(-err));	
		return -err;
	}
	lkl_sys_close(sock);
	return ifr.ifr_ifindex;	
}*/

#ifdef ISLKL
void lkl_change_ifname(int ifindex, char *newname)
{
	int err, sock = lkl_sys_socket(PF_INET, SOCK_DGRAM, 0);
	int ifname_siz;
	char *oldname;
	struct ifreq ifr;
	memset(&ifr,0,sizeof(struct ifreq));
	if (sock < 0){
		printf("LKL init :: change interface name negative value for \n");
		return;
	}

	ifr.ifr_ifindex = ifindex;
	err=lkl_sys_ioctl(sock, SIOCGIFNAME, (long)&ifr);
	if (err < 0){
		printf("LKL init :: could not find out interface name\n");
		return;
	}
	oldname=(char*)malloc(strlen(ifr.ifr_name));
	memcpy(oldname,ifr.ifr_name,strlen(ifr.ifr_name));
	memset(&ifr,0,sizeof(struct ifreq));
	ifname_siz = (IFNAMSIZ > strlen(newname))?strlen(newname):IFNAMSIZ;
	memcpy(ifr.ifr_name,oldname,strlen(oldname));
	memcpy(ifr.ifr_newname,newname, ifname_siz);
	lkl_printf("oldname =%s, newname=%s\n",oldname,newname);
	err=lkl_sys_ioctl(sock, SIOCSIFNAME, (long)&ifr);
	if (err < 0){
		printf("LKL init :: could not change if name %s\n",strerror(-err));	
		return;
	}
	lkl_sys_close(sock);
}

int lkl_init_interface(const interface_t* interface)
{
	int ifindex;
	struct tun_device* td = malloc(sizeof(*td));

	/* TODO: initializa from interface_t */	
	td->type = TUN_HUB;
	td->port = interface->port;
	td->address = interface->gateway.s_addr;
		
	
	if ((ifindex=lkl_add_eth_tun(interface->dev, (char*) interface->mac, 32, td)) < 0) {
		printf("LKL init :: could not bring up interface %s\n",interface->dev);
		return -1;
	}
	
	lkl_change_ifname(ifindex, interface->dev);	
	
	if (interface->address.s_addr == 0) {
		printf("LKL init :: warning! interface address not defined\n");
	} else {
		if (lkl_if_set_ipv4(ifindex, interface->address.s_addr, interface->netmask_len) < 0) {
			printf("LKL init :: could not set IPv4 address\n");
			return -1;
		}
	}

	if (lkl_if_up(ifindex) < 0) {
		printf("LKL init :: could not bring up interface\n");
		return -1;
	}

	return ifindex;
}

int lkl_list_interfaces( int max_if_no)
{
	int err, sock = lkl_sys_socket(PF_INET, SOCK_DGRAM, 0);
	struct ifreq *ifr;
	struct ifreq ifr2;
	struct ifconf ifc;
	struct sockaddr_in* inaddr;

	int i,ifno;

	if (sock < 0){
		printf("LKL init :: change interface name negative value for \n");
		return sock;
	}
	ifr = (struct ifreq*)malloc(max_if_no*sizeof(struct ifreq));
	ifc.ifc_len = max_if_no * sizeof(struct ifreq);
	ifc.ifc_req = ifr;
	err=lkl_sys_ioctl(sock, SIOCGIFCONF, (long)&ifc);
	if (err < 0){
		printf("LKL init :: could not get interfaces\n");
		return -err;
	}

	if (ifc.ifc_len >= max_if_no * sizeof(struct ifconf)){
		free(ifr);
		return lkl_list_interfaces(2*max_if_no);
	}

	ifno=ifc.ifc_len/sizeof(struct ifreq);
	for(i=0;i<ifno;i++){
		memset(&ifr2,0,sizeof(struct ifreq));
		memcpy(ifr2.ifr_name,ifr[i].ifr_name,strlen(ifr[i].ifr_name));
		err = lkl_sys_ioctl(sock,SIOCGIFFLAGS,(long)&ifr2);	
		inaddr = &(ifr[i].ifr_addr);
		lkl_printf("%s %s %s\n",ifr[i].ifr_name,inet_ntoa(inaddr->sin_addr),((ifr2.ifr_flags &IFF_UP)?"UP":"DOWN"));
	}
	lkl_sys_close(sock);
	free(ifr);
	return 0;	
}
#endif

void dump_interface(int fd, interface_t *interface)
{

}
