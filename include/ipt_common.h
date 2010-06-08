#ifndef IPT_COMMON_H_
#define IPT_COMMON_H_

#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>

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

//filter and nat commands args
struct iptargs{
	char *table;
	char *chain;
	enum ipt_ops op;
	struct in_addr src;
	struct in_addr dst;
	long src_mask;
	long dst_mask;	
};

extern struct option global_options[];

struct argstruct *get_args(struct params *params);
void print_header(const char *chain, struct iptc_handle *handle);
void print_entry(const char *chain, const struct ipt_entry *entry, struct iptc_handle *handle);

#endif /* IPT_COMMON_H_ */
