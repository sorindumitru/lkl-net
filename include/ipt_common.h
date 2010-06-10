#ifndef IPT_COMMON_H_
#define IPT_COMMON_H_

#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define XT_FUNCTION_MAXNAMELEN 30
#define VERSION "1.0"

struct ipt_entry;
struct iptc_handle;

struct argstruct {
	int argc;
	char **argv;
};

enum ipt_ops{
	APPEND,
	REPLACE,
	DELETE,
	LIST,
	FLUSH
};

extern char *ipt_ops_to_name[];

#define SRC_F	1<<1
#define DST_F	1<<2	

//filter and nat commands args
struct iptargs{
	unsigned int flags;
	char *table;
	char *chain;
	char *target;
	unsigned int rulenum;
	enum ipt_ops op;
	struct in_addr src;
	struct in_addr dst;
	long src_mask;
	long dst_mask;
	char *in_if;
	char *in_if_mask;
	char *out_if;
	char *out_if_mask;
};

struct iptc_entry_match {//the struct xt_entry_match& struct xt_entry_target equivalent
	unsigned short target_size;
	char name[XT_FUNCTION_MAXNAMELEN];
	unsigned char data[0];
};

union iptc_nf_conntrack_man_proto
{
	/* Add other protocols here. */
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
		unsigned short key;	/* GRE key is 32bit, PPtP only uses 16bit */
	} gre;
};

struct iptc_nf_nat_range //nf_nat_range
{
	/* Set to OR of flags above. */
	unsigned int flags;

	/* Inclusive: network order. */
	unsigned int min_ip, max_ip;

	/* Inclusive: network order */
	union iptc_nf_conntrack_man_proto min, max;
};

struct iptc_xtables_target{
	char *version;
	struct iptc_xtables_target *next;
	char *name;
	size_t size;/* Size of target data. */
	size_t userspacesize;/* Size of target data relevent for userspace comparison purposes */
	unsigned int option_offset;
	struct iptc_entry_match *t; //xt_entry_target *t;
	unsigned int tflags;
	unsigned int used;
	//unsigned int loaded; /* simulate loading so options are merged properly */
};

struct iptc_nf_nat_multi_range
{
	unsigned int rangesize; /* Must be 1. */

	/* hangs off end. */
	struct iptc_nf_nat_range range[1];
};

struct iptc_ipt_natinfo
{
	struct iptc_entry_match t;
	struct iptc_nf_nat_multi_range mr;
};

extern struct option global_options[];

struct argstruct *get_args(struct params *params);
void parse_ip(struct iptargs *ipt, char dest, char *addr);
int addr_to_mask(unsigned int mask);
unsigned int mask_to_addr(int mask);
	
int do_list_entries(struct iptargs *ipt);
int do_flush_entries(struct iptargs *ipt);
int do_delete_entry(struct iptargs *ipt);

void print_ip(const char* prefix, struct in_addr addr, struct in_addr mask);
void print_header(const char *chain, struct iptc_handle *handle);
void print_entry(const char *chain, const struct ipt_entry *entry, struct iptc_handle *handle);
int ipt_parse_interface(char *arg, char *vianame, char *mask);
void iptargs_to_ipt_entry(struct iptargs *ipt,struct ipt_entry *e);

#endif /* IPT_COMMON_H_ */
