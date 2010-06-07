#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/libiptc.h>
#include <console.h>

static void print_header(const char *chain, struct iptc_handle *handle);
static void print_entry(const struct ipt_entry *entry, struct iptc_handle *handle);

int do_list_entries(struct params *params)
{
	struct iptc_handle *handle;
	handle = iptc_init("filter");
	const char *this;

	for (this=iptc_first_chain(handle); this; this=iptc_next_chain(handle)) {
		const struct ipt_entry *i;
		unsigned int num;

		print_header(this, handle);
		i = iptc_first_rule(this, handle);
		num = 0;
		printf("%p\n", i);
		while (i) {
			num++;
			print_entry(i, handle);
			i = iptc_next_rule(i, handle);
		}
	}

	//iptc_free(handle);
	
	return 0;
}

int do_append_entry(struct params *params)
{
	int ret;
	unsigned short size;
	char *chain = params->p[0];
	char *address = params->p[1];
	struct ipt_entry *entry;
	struct iptc_handle *handle = iptc_init("filter");
	char *append = malloc(30);
	memset(append,0,30);
	memcpy(append, "ACCEPT", strlen("ACCEPT"));
	
	size = sizeof(int) + iptc_entry_target_size();
	entry = malloc(sizeof(struct ipt_entry)+size);
	memset(entry, 0, sizeof(struct ipt_entry)+size);
	entry->ip.src.s_addr = inet_aton(address);
	entry->ip.smsk.s_addr = inet_aton("255.255.255.0");
	entry->target_offset = sizeof(struct ipt_entry);
	entry->next_offset = size+sizeof(struct ipt_entry);
	memcpy(entry->elems, &size, 2);
	memcpy(entry->elems+2, append, 30);
	ret = iptc_append_entry(chain, entry, handle);
	
	printf("\n%d\n%d\n", ret, iptc_entry_target_size());
	//iptc_free(handle);
	//free(entry);
	return ret;
}

static void print_header(const char *chain, struct iptc_handle *handle)
{
	struct ipt_counters counters;
	const char *pol = iptc_get_policy(chain, &counters, handle);
	printf("Chain %s", chain);
	printf(" (policy %s", pol);
	printf(" %ld packets, %ld bytes)\n", (long int) counters.pcnt, (long int) counters.bcnt);
}

static void print_entry(const struct ipt_entry *entry, struct iptc_handle *handle)
{
	printf("AAA\n\n");
}
