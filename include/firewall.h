#ifndef LKL_NET_FIREWALL_H_
#define LKL_NET_FIREWALL_H_

#include <params.h>

int do_filter(struct params *params);
int do_list_entries(const char *table, struct params *params);
int do_append_entry(struct params *params);

#endif /* LKL_NET_FIREWALL_H_ */
