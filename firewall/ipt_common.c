#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/libiptc.h>
#include <console.h>
#include <ipt_common.h>

void print_header(const char *chain, struct iptc_handle *handle)
{
	struct ipt_counters counters;
	const char *pol = iptc_get_policy(chain, &counters, handle);
	printf("Chain %s", chain);
	printf(" (policy %s", pol);
	printf(" %ld packets, %ld bytes)\n", (long int) counters.pcnt, (long int) counters.bcnt);
}

void print_entry(const char* chain, const struct ipt_entry *entry, struct iptc_handle *handle)
{
	const char *target;
	printf("-A %s ", chain);
	target = iptc_get_target(entry, handle);
	printf("-j %s", target); 
	printf("\n");
}
