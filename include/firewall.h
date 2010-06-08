#ifndef LKL_NET_FIREWALL_H_
#define LKL_NET_FIREWALL_H_

#include <params.h>

struct iptargs;

int do_filter(struct params *params);
int do_filter_append_entry(struct iptargs *ipt);

#endif /* LKL_NET_FIREWALL_H_ */
