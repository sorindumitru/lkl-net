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

struct argstruct *get_args(struct params *params)
{
	int i;
	struct argstruct *args = malloc(sizeof(*args));
	args->argc = params->count;
	args->argv = malloc(args->argc*sizeof(char*));
	for(i=0;i<args->argc;i++){
		args->argv[i] = strdup(params->p[i]);
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

int do_list_entries(struct iptargs *ipt)
{
	struct iptc_handle *handle;
	handle = iptc_init(ipt->table);
	const char *this;

	for (this=iptc_first_chain(handle); this; this=iptc_next_chain(handle)) {
		const struct ipt_entry *i;
		unsigned int num;

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
	char *netmask = malloc(32);
	address = inet_ntop(AF_INET, (void*) &addr, address, 32);
	netmask = inet_ntop(AF_INET, (void*) &mask, netmask, 32);
	printf("-%s %s/%s ", address, netmask);
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
