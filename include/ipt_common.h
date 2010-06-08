#ifndef IPT_COMMON_H_
#define IPT_COMMON_H_

#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
	enum ipt_ops op;
	struct in_addr src;
	struct in_addr dst;
	long src_mask;
	long dst_mask;	
};

extern struct option global_options[];

struct argstruct *get_args(struct params *params);
void parse_ip(struct iptargs *ipt, char dest, char *addr);
int addr_to_mask(unsigned int mask);
unsigned int mask_to_addr(int mask);
	
int do_list_entries(struct iptargs *ipt);

void print_ip(const char* prefix, struct in_addr addr, struct in_addr mask);
void print_header(const char *chain, struct iptc_handle *handle);
void print_entry(const char *chain, const struct ipt_entry *entry, struct iptc_handle *handle);

#endif /* IPT_COMMON_H_ */
