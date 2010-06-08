#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <config.h>
#include <list.h>

#include <asm/env.h>
#include <asm/eth.h>
#include <asm/libiptc.h>
#include <console.h>
#include <netinet/in.h>
#include <ipt_common.h>
#include <nat.h>

//check validity of command
static int check_ipt_command(struct iptargs *ipt)
{
	//check basic data about targets and allowed chains
	
	if ((ipt->chain != NULL) && strcmp(ipt->chain,"PREROUTING")!=0 && strcmp(ipt->chain,"POSTROUTING")!=0 && strcmp(ipt->chain,"OUTPUT")!=0){
		printf("Wrong chain for NAT\n");
		return 1;
	}
	if (ipt->target != NULL){
		if ( (strcmp(ipt->target,"DNAT")==0) && (strcmp(ipt->chain,"POSTROUTING")==0) ){
			printf("Cannot use DNAT target for POSTROUTING chain\n");
			return 1;
		}

		if( ((strcmp(ipt->target,"SNAT")==0) || (strcmp(ipt->target,"MASQUERADE")==0))&& (strcmp(ipt->chain,"POSTROUTING")!=0)){
			printf("CANNOT use SNAT target for other chain then POSTROUTING\n");
			return 1;
		}
		
		if (strcmp(ipt->target,"REDIRECT")==0 && strcmp(ipt->chain,"POSTROUTING")==0){
			printf("Cannot use REDIRECT target with POSTROUTING chain\n");
			return 1;
		}
		
		if ((strcmp(ipt->target,"REJECT")==0) && (strcmp(ipt->chain,"OUTPUT")!=0)){
			printf("Use REJECT target only with OUTPUT chain\n");
			return 1;
		}
	}

	//TODO check other stuff
	printf("eop\n");
	return 0;
}

int do_nat(struct params *params)
{
	int c;
	int i=0;
	struct iptargs *ipt = malloc(sizeof(*ipt));
	struct argstruct *args = get_args(params);
	memset(ipt,0,sizeof(ipt));
	ipt->table = "nat";
	ipt->chain = NULL;
	optind = 1;
	
	while ((c = getopt(args->argc, args->argv, "-A:L::s:d:j:")) != -1) {
		printf("CC:%d %c\n", c, c);
		switch(c) {
		case 'A':
			ipt->chain = strdup(optarg);
			printf("Here\n");
			ipt->op = APPEND;
			break;
		case 'L':
			if (optarg) {
				printf("parameter for list\n");
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
	
	if (check_ipt_command(ipt))
		return 1;	
	
	switch (ipt->op) {
	case APPEND:
		do_append_nat_entry(ipt);
		break;
	case LIST:
		do_list_entries(ipt);
		break;
	default:
		break;
	}
	return 0;
}

int do_append_nat_entry(struct iptargs *ipt)
{
	int ret;
	unsigned short size;

	struct ipt_entry *entry;
	struct iptc_handle *handle = iptc_init(ipt->table);
	
	size = sizeof(int) + iptc_entry_target_size();
	entry = malloc(sizeof(struct ipt_entry)+size);
	memset(entry, 0, sizeof(struct ipt_entry)+size);
	entry->target_offset = sizeof(struct ipt_entry);
	entry->next_offset = size+sizeof(struct ipt_entry);
	memcpy(entry->elems, &size, 2);
	entry->ip.invflags = 0x08;
	memcpy(entry->elems+2, ipt_ops_to_name[ipt->op], strlen(ipt_ops_to_name[ipt->op]));
	
	ret = iptc_append_entry(ipt->chain, entry, handle);
	
	printf("\n%d\n%d\n", ret, iptc_entry_target_size());
	//iptc_free(handle);
	//free(entry);
	ret = iptc_commit(handle);
	printf("%d %s\n", ret, iptc_strerror(ret));
	return ret;
}
