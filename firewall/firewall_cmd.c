#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/libiptc.h>
#include <console.h>
#include <netinet/in.h>
#include <ipt_common.h>

enum TARGET {
	ACCEPT,
	DROP,
	DENY,
};

struct target {
	enum TARGET id;
	char *name;
	unsigned char inv;
	unsigned char invflags;
} targets[] = {

};

int do_filter(struct params *params)
{
	int c;
	struct iptargs *ipt = malloc(sizeof(*ipt));
	struct argstruct *args = get_args(params);
	ipt->table = "filter";
	optind = 1;
	while ((c = getopt(args->argc, args->argv, "-A:L::s:d:j:")) != -1) {
		switch(c) {
		case 'A':
			ipt->chain = optarg;
			ipt->op = APPEND;
			break;
		case 'L':
			if (optarg) {
				ipt->chain = strdup(optarg);
			} else {
				ipt->chain = NULL;
			}
			ipt->op = LIST;
			break;
		case 's':
			parse_ip(ipt, 's', optarg);
			ipt->flags &= SRC_F;
			break;
		case 'd':
			parse_ip(ipt, 'd', optarg); 
			break;
		case 'j':
			ipt->target = malloc(30);
			memset(ipt->target, 0, 30);
			memcpy(ipt->target, optarg, strlen(optarg));
			break;
		default:
			printf("Unrecognized option\n");
			break;
		}
	}
	switch (ipt->op) {
	case APPEND:
		do_filter_append_entry(ipt);
		break;
	case LIST:
		do_list_entries(ipt);
		break;
	default:
		break;
	}
	return 0;
}

int do_filter_append_entry(struct iptargs *ipt)
{
	int ret;
	unsigned short size;
	char *chain = ipt->chain;
	struct ipt_entry *entry;
	struct iptc_handle *handle = iptc_init("filter");
	
	size = sizeof(int) + iptc_entry_target_size();
	entry = malloc(sizeof(struct ipt_entry)+size);
	memset(entry, 0, sizeof(struct ipt_entry)+size);
	entry->ip.src.s_addr = ipt->src.s_addr;
	entry->ip.smsk.s_addr = mask_to_addr(ipt->src_mask);
	entry->target_offset = sizeof(struct ipt_entry);
	entry->next_offset = size+sizeof(struct ipt_entry);
	memcpy(entry->elems, &size, 2);
	entry->ip.invflags = 0x08;
	memcpy(entry->elems+2, ipt->target, 30);
	
	ret = iptc_append_entry(chain, entry, handle);
	ret = iptc_commit(handle);
	//iptc_free(handle);

	return ret;
}
