#ifndef LKL_NET_SWITCH_CMD_H_
#define LKL_NET_SWITCH_CMD_H_

#include <readline/readline.h>
#include <readline/history.h>

#ifndef LKL_NET_CONSOLE_PARAMS
#define LKL_NET_CONSOLE_PARAMS 1

#define MAX_PARAM_NO	8
typedef struct params {
	void *p[MAX_PARAM_NO];
} params;
#endif /* LKL_NET_CONSOLE_PARAMS */

int do_show_bridge_interface(struct params* parameters);
int do_show_mac_table PARAMS((struct params*));
int do_set_stp(struct params* params);
int do_show_ip_route(struct params* params);

#endif /* LKL_NET_SWITCH_CMD_H_ */
