#ifndef LKL_NET_ROUTER_H_
#define LKL_NET_ROUTER_H_

#include <params.h>

#include <readline/readline.h>
#include <readline/history.h>

extern int do_show_ip_route(struct params* params);
extern int do_add_route(struct params* params);
extern int do_remove_route(struct params* params);
extern int do_add_interface(struct params *params);
extern int do_set_interface_up(struct params *params);
extern int do_set_interface_down(struct params *params);
extern int do_change_if_address(struct params *params);
extern int do_list_router_interfaces(struct params *params);
extern int do_delete_interface(struct params *params);
#endif /* LKL_NET_ROUTER_H_ */
