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

int do_filter(struct params *params)
{
	int c;
	int i=0;
	struct iptargs *ipt = malloc(sizeof(*ipt));
	struct argstruct *args = get_args(params);
	ipt->table = "filter";
	optind = 1;
	while ((c = getopt(args->argc, args->argv, "-A:L::s:d:j:")) != -1) {
		printf("CC:%d %c\n", c, c);
		switch(c) {
		case 'A':
			ipt->chain = optarg;
			ipt->op = APPEND;
			break;
		case 'L':
			if (optarg) {
				ipt->chain = strdup(optarg);
			}
			ipt->op = LIST;
			break;
		case 's':
			parse_ip(ipt, 's', optarg);
			break;
		case 'd':
			parse_ip(ipt, 'd', optarg); 
			break;
		case 'j':
			break;
		default:
			printf("Unrecognized option\n");
			break;
		}
	}
	switch (ipt->op) {
	case APPEND:
		break;
	case LIST:
		do_list_entries(ipt);
		break;
	default:
		break;
	}
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
	//entry->ip.src.s_addr = inet_aton(address, &entry->ip.src);
	//entry->ip.smsk.s_addr = inet_aton("255.255.255.0", &entry->ip.smsk);
	entry->target_offset = sizeof(struct ipt_entry);
	entry->next_offset = size+sizeof(struct ipt_entry);
	memcpy(entry->elems, &size, 2);
	entry->ip.invflags = 0x08;
	memcpy(entry->elems+2, append, 30);
	ret = iptc_append_entry(chain, entry, handle);
	
	printf("\n%d\n%d\n", ret, iptc_entry_target_size());
	//iptc_free(handle);
	//free(entry);
	ret = iptc_commit(handle);
	printf("%d %s\n", ret, iptc_strerror(ret));
	return ret;
}
