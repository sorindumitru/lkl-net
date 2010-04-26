#ifndef LKL_NET_ROUTER_H_
#define LKL_NET_ROUTER_H_

#include <params.h>

#include <readline/readline.h>
#include <readline/history.h>

extern int do_show_ip_route(struct params* params);
extern int do_add_route(struct params* params);
extern int do_remove_route(struct params* params);

#endif /* LKL_NET_ROUTER_H_ */
