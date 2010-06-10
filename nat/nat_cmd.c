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

struct _xt_align {
	unsigned char u8;
	unsigned short u16;
	unsigned int u32;
	unsigned long u64;
};
#define XT_ALIGN(s) (((s) + (__alignof__(struct _xt_align)-1)) 	\
			& ~(__alignof__(struct _xt_align)-1))
#define IP_NAT_RANGE_MAP_IPS 3
#define IP_NAT_RANGE_PROTO_SPECIFIED 5

#define IPT_SNAT_OPT_SOURCE 0x01

#define XT_ALIGN(s) (((s) + (__alignof__(struct _xt_align)-1)) 	\
			& ~(__alignof__(struct _xt_align)-1))
static const struct option NAT_opts[] = {
	{ "to-source", 1, NULL, 'S' },
	{ "to-destination", 1, NULL, 'D' },
};

int do_append_nat_entry(struct iptargs *ipt,struct iptc_xtables_target *target);

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

	return 0;
}

static struct iptc_ipt_natinfo *append_range(struct iptc_ipt_natinfo *info, const struct iptc_nf_nat_range *range)
{
	unsigned int size;

	/* One rangesize already in struct ipt_natinfo */
	size = XT_ALIGN(sizeof(*info) + info->mr.rangesize * sizeof(*range));

	info = realloc(info, size);
	if (!info)
		printf("\n\nAPPEND RANGE:: out of memory\n");

	info->mr.range[info->mr.rangesize] = *range;
	info->mr.rangesize++;

	return info;
}

static struct iptc_entry_match *parse_to(char *arg, int portok, struct iptc_ipt_natinfo *info)
{
	struct iptc_nf_nat_range range;
	char *dash, *error;
	struct in_addr *ip;

	memset(&range, 0, sizeof(range));

	range.flags |= IP_NAT_RANGE_MAP_IPS;
	dash = strchr(arg, '-');

	if (dash)
		*dash = '\0';

	ip->s_addr =inet_addr(arg);
	if (!ip)
		printf("Wrong source address\n");
	range.min_ip = ip->s_addr;
	if (dash) {
		ip = inet_addr(dash+1);
		if (!ip)
			printf("Wrong source address\n");
		range.max_ip = ip->s_addr;
	} else
		range.max_ip = range.min_ip;

	return &(append_range(info, &range)->t);
}

static int NAT_target_parse(int c, struct iptc_entry_match **target,char *to_address)
{
	//struct iptc_ipt_natinfo *info = (void *)*target;
	int portok = 1;
	printf(">>>>>>>>>>>target c=%s<<<<<<<<<<<<<\n",c);
	
	switch (c) {
		case 'S':
			printf(">>>>>>>>>>>SNAT target<<<<<<<<<<<<<\n");
			//*target = parse_to(optarg, portok, info);
			return 1;
		default:
			return 0;
	}
}		

int do_nat(struct params *params)
{
	int c,optindex;
	int i=0;
	struct iptargs *ipt = malloc(sizeof(struct iptargs));
	struct argstruct *args = get_args(params);
	struct iptc_xtables_target *target;
	struct ipt_entry *fw,*e=NULL;
	memset(ipt,0,sizeof(struct iptargs));
	ipt->table = "nat";
	ipt->chain = NULL;
	optind = 1;

	memset(&fw, 0, sizeof(fw));
	
	while ((c = getopt_long(args->argc, args->argv, "-A:L::s:d:j:o:i:S:D:",global_options,&optindex)) != -1) {
		printf("#CC:%d %c#\n", c, c);
		switch(c) {
		case 'A':
			ipt->chain = strdup(optarg);
			printf("chain=%s\n",ipt->chain);
			ipt->op = APPEND;
			break;
		case 'L':
			if (optarg) {
				ipt->chain = strdup(optarg);
			}
			ipt->op = LIST;
			break;
		case 's':
			ipt->flags|=SRC_F;
			parse_ip(ipt, 's', optarg);
			break;
		case 'd':
			ipt->flags|=DST_F;
			parse_ip(ipt, 'd', optarg); 
			break;
		case 'j':
			if (!optarg){
				printf("Must specify target\n");
				return 1;
			}
			ipt->target = strdup(optarg);
			if ((optarg)&&(strcmp(optarg,"SNAT")==0||strcmp(optarg,"DNAT")==0)){
				size_t size;
				target = malloc(sizeof(struct iptc_xtables_target));
				memset(target,0,sizeof(struct iptc_xtables_target));
				target->name = strdup(optarg);
				target->version = strdup(VERSION);
				target->size = IPT_ALIGN(sizeof(struct iptc_nf_nat_range));

				size = IPT_ALIGN(sizeof(struct iptc_entry_match))+ target->size;
				target->t = calloc(1, size);
			}
			break;
		case 'o':
			if (ipt_parse_interface(optarg,ipt->out_if,ipt->out_if_mask))
				return 1;
			break;
		case 'i':
			if (ipt_parse_interface(optarg,ipt->out_if,ipt->out_if_mask))
				return 1;
			break;
		case 'S' :
			printf("SNAT c=%c\n");
			NAT_target_parse(c,&target->t,optarg);
			break;
		case 'D' :
			printf("DNAT\n");
			break;
		default:
			printf("not recognized\n");
			break;
		}
	}
	
	if (check_ipt_command(ipt))
		return 1;	
	
	switch (ipt->op) {
	case APPEND:
		do_append_nat_entry(ipt,target);
		break;
	case LIST:
		do_list_entries(ipt);
		break;
	default:
		break;
	}
	return 0;
}

int do_append_nat_entry(struct iptargs *ipt,struct iptc_xtables_target *target)
{
	int ret;
	unsigned short size;
	char *chain = ipt->chain;
	struct ipt_entry *entry;
	struct iptc_handle *handle = iptc_init(ipt->table);
	size = sizeof(struct iptc_entry_match)+sizeof(struct iptc_nf_nat_multi_range);
	entry = malloc(sizeof(struct ipt_entry)+size);
	entry->target_offset = sizeof(struct ipt_entry)+sizeof(struct iptc_entry_match);
	entry->next_offset = size+sizeof(struct ipt_entry);
	memcpy(entry->elems, target->t, sizeof(*target->t));
	entry->ip.invflags = 0x77;
	
	ret = iptc_append_entry(chain, entry, handle);
	ret = iptc_commit(handle);
	iptc_free(handle);
	return ret;
}
