#ifndef _NAT_H_
#define _NAT_H_ 1

#include <params.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ   16
#endif

#define XT_FUNCTION_MAXNAMELEN 30
//starting to add
struct nat_xt_entry_target {
	unsigned short target_size;
	char name[XT_FUNCTION_MAXNAMELEN];
	//unsigned short revision;
	unsigned char data[0];
};

/*union nat_nf_conntrack_man_proto
{
	// Add other protocols here. //
	unsigned short all;
	struct {
		unsigned short port;
	} tcp;
	struct {
		unsigned short port;
	} udp;
	struct {
		unsigned short id;
	} icmp;
	struct {
		unsigned short port;
	} dccp;
	struct {
		unsigned short port;
	} sctp;
	struct {
		unsigned short key;	// GRE key is 32bit, PPtP only uses 16bit //
	} gre;
};*/

struct nat_nf_nat_range //nf_nat_range
{
	/* Set to OR of flags above. */
	unsigned int flags;

	/* Inclusive: network order. */
	unsigned int min_ip, max_ip;

	/* Inclusive: network order */
	//union nat_nf_conntrack_man_proto min, max;
	unsigned short min, max;
};

struct nat_nf_nat_multi_range
{
	unsigned int rangesize; /* Must be 1. */

	/* hangs off end. */
	struct nat_nf_nat_range range[1];
};

struct ipt_natinfo
{
	struct nat_xt_entry_target t;
	struct nat_nf_nat_multi_range mr;
};
//done adding
int do_nat(struct params *params);


#endif
