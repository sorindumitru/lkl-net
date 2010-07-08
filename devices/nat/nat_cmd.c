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

//#define IP_NAT_RANGE_MAP_IPS 3
#define IP_NAT_RANGE_PROTO_SPECIFIED 5
#define IPT_SNAT_OPT_SOURCE 0x01

static const struct option NAT_opts[] = {
	{ "to-source", 1, NULL, 'S' },
	{ "to-destination", 1, NULL, 'Z' },
};

int do_append_nat_entry(struct iptargs *ipt,struct ipt_natinfo *target);

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

static struct ipt_natinfo *append_range(struct ipt_natinfo *info, const struct nat_nf_nat_range *range)
{
	unsigned int size;

	/* One rangesize already in struct ipt_natinfo */
	size = XT_ALIGN(sizeof(*info) + info->mr.rangesize * (sizeof(*range) - 1));

	info = realloc(info, size);
	if (!info)
		printf("\n\nAPPEND RANGE:: out of memory\n");

	info->t.target_size = size;
	info->mr.range[info->mr.rangesize] = *range;
	info->mr.rangesize++;

	return info;
}

static struct ipt_natinfo *parse_to(char *arg,struct ipt_natinfo *info)
{
	struct nat_nf_nat_range range;
	char *dash;
	struct in_addr *ip = (struct in_addr*)malloc(sizeof(struct in_addr));
	char *addr,*addr1;

	memset(&range, 0, sizeof(range));
	
	range.flags |= IP_NAT_RANGE_MAP_IPS;// | IP_NAT_RANGE_PROTO_SPECIFIED;
	dash = strchr(arg, '-');
	if (dash){
		addr = strdup(strtok(arg,"-"));
		addr1 = strdup(strtok(NULL,"\0"));
	}else
		addr = strdup(arg);
	
	ip->s_addr =inet_addr(addr);
	if (!ip)
		printf("Wrong source address\n");
	range.min_ip = ip->s_addr;
	if (dash) {
		ip->s_addr = inet_addr(addr1);
		if (!ip)
			printf("Wrong source address\n");
		range.max_ip = ip->s_addr;
	} else
		range.max_ip = range.min_ip;

	free(addr);
	if (dash)
		free(addr1);

        range.min = htons(0);
        range.max = htons(65535);

	return append_range(info, &range);
}

static int NAT_target_parse(char c,char *to_address,struct ipt_natinfo **target)
{

	switch (c) {
	case 'S':
		*target = parse_to(to_address, *target);
		return 1;
	case 'Z':
		*target = parse_to(to_address, *target);
		return 1;
	default:
		return 0;
	}
}		

int do_nat(struct params *params)
{
	int c,optindex;
	struct iptargs *ipt = malloc(sizeof(struct iptargs));
	struct argstruct *args = get_args(params);
	struct ipt_natinfo *target;

	memset(ipt,0,sizeof(struct iptargs));
	ipt->table = "nat";
	ipt->chain = NULL;
	optind = 1;
	
	while ((c = getopt_long(args->argc, args->argv, "A:L::s:d:j:o:i:S:D:F:",global_options,&optindex)) != -1) {
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
				size = IPT_ALIGN(sizeof(struct nat_nf_nat_multi_range)) + sizeof(struct nat_xt_entry_target);
				target = malloc(size);
				memset(target,0,size);
				target->t.target_size = size;
				if ( strlen(optarg)<XT_FUNCTION_MAXNAMELEN)
					memcpy(target->t.name,optarg,strlen(optarg));
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
			if(!NAT_target_parse(c,optarg,&target))
				printf("SNAT_target_parse error\n");
			break;
		case 'Z' :
			if(!NAT_target_parse(c,optarg,&target))
				printf("DNAT_target_parse error\n");
			break;
		case 'D':
			ipt->chain = optarg;
			ipt->rulenum = strtoul(args->argv[optind++], NULL, 10);
			ipt->op = DELETE;
			break;
		case 'F':
			if (optarg) {
				ipt->chain = optarg;
			}
			ipt->op = FLUSH;
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
	case DELETE:
		do_delete_entry(ipt);
		break;
	case FLUSH:
		do_flush_entries(ipt);
		break;
	default:
		break;
	}
	return 0;
}

int do_append_nat_entry(struct iptargs *ipt,struct ipt_natinfo *target)
{
	int ret;
	unsigned short size;

	char *chain = ipt->chain;

	struct ipt_entry *entry;
	struct iptc_handle *handle = iptc_init(ipt->table);
	size = sizeof(*target);

	entry = malloc(sizeof(struct ipt_entry)+target->t.target_size);
	memset(entry, 0, sizeof(struct ipt_entry)+target->t.target_size);
	iptargs_to_ipt_entry(ipt,entry);
	printf("%s\n", target->t.name);
	
	//memcpy(entry->elems, &, sizeof(unsigned short));
	//memcpy(entry->elems+sizeof(unsigned short), target, size);
	
	entry->target_offset = sizeof(struct ipt_entry);
	entry->next_offset = entry->target_offset + target->t.target_size;
	//entry->ip.flags |= IPT_SNAT_OPT_SOURCE;
	//target->t.revision = 1;
	printf("SIZE = %d %d\n", target->t.target_size, target->mr.rangesize);
	memcpy(entry->elems, target, target->t.target_size);
	ret = iptc_append_entry(chain, entry, handle);
	if (ret)
		printf("Append successfully\n");
	else 
		printf("ERROR %d %s\n", ret, iptc_strerror(ret));
	ret = iptc_commit(handle);
	if (ret)
		printf("Commit successfully\n");
	else 
		printf("ERROR %d %s\n", ret, iptc_strerror(ret));
	iptc_free(handle);
	return ret;
}
