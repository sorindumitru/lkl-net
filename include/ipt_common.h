#ifndef IPT_COMMON_H_
#define IPT_COMMON_H_

#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ   16
#endif

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
	char in_if[IFNAMSIZ];
	char in_if_mask[IFNAMSIZ];
	char out_if[IFNAMSIZ];
	char out_if_mask[IFNAMSIZ];
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
