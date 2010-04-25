#ifndef LKL_NET_ROUTER_H_
#define LKL_NET_ROUTER_H_

#include <readline/readline.h>
#include <readline/history.h>

#ifndef LKL_NET_CONSOLE_PARAMS
#define LKL_NET_CONSOLE_PARAMS 1

#define MAX_PARAM_NO	8
typedef struct params {
	void *p[MAX_PARAM_NO];
} params;
#endif /* LKL_NET_CONSOLE_PARAMS */

extern int do_show_ip_route(struct params* params);
extern int do_add_route(struct params* params);
extern int do_remove_route(struct params* params);

#endif /* LKL_NET_ROUTER_H_ */
