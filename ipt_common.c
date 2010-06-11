#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/libiptc.h>
#include <console.h>
#include <arpa/inet.h>
#include <ipt_common.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ	16
#endif

struct option global_options[] = {
	{.name = "append",     .flag = NULL,        .has_arg = 1,  .val = 'A'},
	{.name = "list",       .flag = NULL,        .has_arg = 2,  .val = 'L'},
	{.name = "src",        .flag = NULL,        .has_arg = 1,  .val = 's'},
	{.name = "dst",        .flag = NULL,        .has_arg = 1,  .val = 'd'},
	{.name = "jump",       .flag = NULL,        .has_arg = 1,  .val = 'j'},
	{.name = "in_if",      .flag = NULL,        .has_arg = 1,  .val = 'i'},
	{.name = "out_if",     .flag = NULL,        .has_arg = 1,  .val = '0'}, 
	{.name = "to-source",  .flag = NULL,        .has_arg = 1,  .val = 'S'},
	{.name = "to-destination",  .flag = NULL,        .has_arg = 1,  .val = 'Z'},
	{.name = "delete",       .flag = NULL,      .has_arg = 1,  .val = 'D'},
	{.name = "flush",        .flag = NULL,      .has_arg = 1,  .val = 'F'},	
	{NULL},
};

char *ipt_ops_to_name[] = {"APPEND","REPLACE","DELETE","LIST","FLUSH"};

struct argstruct *get_args(struct params *params)
{
	int i;
	struct argstruct *args = malloc(sizeof(*args));
	args->argc = params->count+1;
	args->argv = malloc(args->argc*sizeof(char*));
	args->argv[0] = strdup("filter");
	for(i=1;i<args->argc;i++){
		args->argv[i] = strdup(params->p[i-1]);
	}

	return args;
}

void parse_ip(struct iptargs *ipt, char dest, char *addr)
{
	char *address = strtok(addr,"/");
	int netmask;
	printf("address=%s\n",address);
	if (strlen(address) == strlen(addr))
		netmask = 32;
	else
		netmask = atoi(strtok(NULL, "/"));

	printf("netmask=%d\n",netmask);
	if (dest == 's') {
		inet_pton(AF_INET, address, &ipt->src);
		ipt->src_mask = netmask;
	} else {
		inet_pton(AF_INET, address, &ipt->dst);
		ipt->dst_mask = netmask;
	}
}

unsigned int mask_to_addr(int mask)
{
	int i=1;
	unsigned int addr = 1;

	if (mask == 32) {
		return 0xFFFFFFFFU;
	}

	for(i=1;i<=mask;i++){
		addr *= 2;
	}

	return addr-1;
}

int addr_to_mask(unsigned int mask)
{
	int i;
	unsigned int bits;
	unsigned int hmask = mask;
	if (mask == 0xFFFFFFFFU) {
		return 32;
	}

	i    = 32;
	bits = 0x7FFFFFFF;
	while (--i >= 0 && hmask != bits) {
		bits >>= 1;
	}
	return i;
}

int do_list_entries(struct iptargs *ipt)
{
	struct iptc_handle *handle;
	handle = iptc_init(ipt->table);
	const char *this;

	for (this=iptc_first_chain(handle); this; this=iptc_next_chain(handle)) {
		const struct ipt_entry *i;
		unsigned int num;

		if (ipt->chain && strcmp(ipt->chain, this) != 0)
			continue;

		print_header(this, handle);
		i = iptc_first_rule(this, handle);
		num = 0;
		while (i) {
			num++;
			printf("%d\t | ", num);
			print_entry(this, i, handle);
			i = iptc_next_rule(i, handle);
		}
	}

	iptc_free(handle);
	
	return 0;
}

int do_flush_entries(struct iptargs *ipt)
{
	struct iptc_handle *handle;
	handle = iptc_init(ipt->table);
	if (!ipt->chain){
		char *this;
		for (this=iptc_first_chain(handle); this; this=iptc_next_chain(handle)) {
			iptc_flush_entries(this, handle);
		}
	} else {
		iptc_flush_entries(ipt->chain, handle);
	}

	iptc_commit(handle);
	iptc_free(handle);
	 
	return 0;
}

int do_delete_entry(struct iptargs *ipt)
{
	struct iptc_handle *handle;
	ipt_chainlabel chain;
	int ret;
	
	memset(chain,0,32);
	memcpy(chain,ipt->chain,strlen(ipt->chain));
	handle = iptc_init(ipt->table);
	ret = iptc_delete_num_entry(chain, ipt->rulenum,handle);
	if (!ret){
		printf("Could not delete rule\n");
		return ret;
	}
	ret = iptc_commit(handle);
	if (!ret){
		printf("Could not commit delete rule\n");
		return ret;
	}
	iptc_free(handle);
	return 1;
}

void print_ip(const char* prefix, struct in_addr addr, struct in_addr mask)
{
	char *address = malloc(32);
	int netmask;
	memset(address, 0, 32);
	address = inet_ntop(AF_INET, (void*) &addr, address, 32);
	netmask = addr_to_mask(mask.s_addr);
	
	printf("-%s %s/%d ", prefix, address, netmask);

	free(address);
}

void print_header(const char *chain, struct iptc_handle *handle)
{
	struct ipt_counters counters;
	const char *pol = iptc_get_policy(chain, &counters, handle);
	printf("Chain %s", chain);
	printf(" (policy %s", pol);
	printf(" %ld packets, %ld bytes)\n", (long int) counters.pcnt, (long int) counters.bcnt);
}

void print_entry(const char* chain, const struct ipt_entry *entry, struct iptc_handle*handle)
{
	const char *target;
	printf("-A %s ", chain);
	target = iptc_get_target(entry, handle);
	print_ip("s", entry->ip.src, entry->ip.smsk);
	print_ip("d", entry->ip.dst, entry->ip.dmsk);
	printf("-j %s", target); 
	printf("\n");
}

int ipt_parse_interface(char *arg, char *vianame, char *mask)
{
	unsigned int vialen = strlen(arg);

	mask = (char*)malloc(IFNAMSIZ);
	vianame = (char*)malloc(IFNAMSIZ);
	
	memset(mask, 0, IFNAMSIZ);
	memset(vianame, 0, IFNAMSIZ);
	
	if (vialen + 1 > IFNAMSIZ){
		printf("Wrong name for via Interface\n");
		return 1;
	}
	
	strcpy(vianame, arg);
	if (vialen == 0)
		memset(mask, 0, IFNAMSIZ);
	
	memset(mask, 0xFF, vialen + 1);
	memset(mask + vialen + 1, 0, IFNAMSIZ - vialen - 1);
	return 0;
}

void iptargs_to_ipt_entry(struct iptargs *ipt,struct ipt_entry *e)
{
	memset(e,0,sizeof(*e));
	e->ip.src = ipt->src;
	e->ip.dst = ipt->dst;
	e->ip.smsk.s_addr = mask_to_addr(ipt->src_mask);
	e->ip.dmsk.s_addr = mask_to_addr(ipt->dst_mask);
	memcpy(e->ip.iniface,ipt->in_if,IFNAMSIZ);
	memcpy(e->ip.outiface,ipt->out_if,IFNAMSIZ);
	memcpy(e->ip.iniface_mask,ipt->in_if_mask,IFNAMSIZ);
	memcpy(e->ip.outiface_mask,ipt->out_if_mask,IFNAMSIZ);
} 

/*int do_delete_entry(struct iptargs *ipt)
{
	struct iptc_handle *handle;
	ipt_chainlabel chain;
	int ret;

	memcpy(&chain,ipt->chain,strlen(ipt->chain));
	handle = iptc_init(ipt->table);
	ret = iptc_delete_num_entry(chain, ipt->rulenum,handle)
	if (!ret){
		printf("Could not delete rule\n");
		return ret;
	}
	ret = ret = iptc_commit(handle);
	if (!ret){
		printf("Could not commit delete rule\n");
		return ret;
	}
	iptc_free(handle);
	return 1;
}*/
