#ifndef LKL_NET_HYPERVISOR_CMD_H_
#define LKL_NET_HYPERVISOR_CMD_H_

#include <params.h>

#include <readline/readline.h>
#include <readline/history.h>

extern int do_create_link(struct params *params);
extern int do_show_devices(struct params *params);
extern int do_show_device(struct params *params);
extern int do_create_router(struct params *params);
extern int do_create_switch(struct params *params);
extern int do_telnet(struct params *params);
extern int do_boot_up(struct params *params);
extern int do_dump_hyper_config(struct params *params);
extern int do_kill_all(struct params *params);

#endif /* LKL_NET_HYPERVISOR_CMD_H_ */
