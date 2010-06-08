#ifndef IPT_COMMON_H_
#define IPT_COMMON_H_

struct ipt_entry;
struct iptc_handle;

struct argstruct {
	int argc;
	char **argv;
};
struct argstruct *get_args(struct params *params);
void print_header(const char *chain, struct iptc_handle *handle);
void print_entry(const char *chain, const struct ipt_entry *entry, struct iptc_handle *handle);

#endif /* IPT_COMMON_H_ */
