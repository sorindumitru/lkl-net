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

struct option global_options[] = {
	{.name = "append",     .flag = NULL,        .has_arg = 1,  .val = 'A'},
	{.name = "list",       .flag = NULL,        .has_arg = 2,  .val = 'L'},
	{.name = "src",        .flag = NULL,        .has_arg = 1,  .val = 's'},
	{.name = "dst",        .flag = NULL,        .has_arg = 1,  .val = 'd'},
	{.name = "jump",       .flag = NULL,        .has_arg = 1,  .val = 'j'},
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
	int netmask = atoi(strtok(NULL, "/"));
	if (dest == 's') {
		inet_pton(AF_INET, address, &ipt->src);
		ipt->src_mask = netmask;
	} else {
		inet_pton(AF_INET, address, &ipt->src);
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
			print_entry(this, i, handle);
			i = iptc_next_rule(i, handle);
		}
	}
	
	return 0;
}

void print_ip(const char* prefix, struct in_addr addr, struct in_addr mask)
{
	char *address = malloc(32);
	int netmask;
	memset(address, 0, 32);
	memset(netmask, 0, 32);
	address = inet_ntop(AF_INET, (void*) &addr, address, 32);
	netmask = addr_to_mask(mask.s_addr);
	
	printf("-%s %s/%d ", prefix, address, netmask);
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
	printf("-j %s", target); 
	printf("\n");
}
