#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/lkl_bridge.h>
#include <asm/libnetlink.h>

#include <switch.h>
#include <switch_cmd.h>

void lkl_br_show_timer(const struct br_timeval *tv)
{
	printf("%4i.%.2i", (int)tv->tv_sec, (int)tv->tv_usec/10000);
}

static int dump_info(const char *br, struct bridge_info *bri)
{
	int err;

	printf("%s\n", br);
	printf(" bridge id\t\t");
	lkl_br_dump_bridge_id((unsigned char *)&bri->bridge_id);
	printf("\n designated root\t");
	lkl_br_dump_bridge_id((unsigned char *)&bri->designated_root);
	printf("\n root port\t\t%4i\t\t\t", bri->root_port);
	printf("path cost\t\t%4i\n", bri->root_path_cost);
	printf(" max age\t\t");
	lkl_br_show_timer(&bri->max_age);
	printf("\t\t\tbridge max age\t\t");
	lkl_br_show_timer(&bri->bridge_max_age);
	printf("\n hello time\t\t");
	lkl_br_show_timer(&bri->hello_time);
	printf("\t\t\tbridge hello time\t");
	lkl_br_show_timer(&bri->bridge_hello_time);
	printf("\n forward delay\t\t");
	lkl_br_show_timer(&bri->forward_delay);
	printf("\t\t\tbridge forward delay\t");
	lkl_br_show_timer(&bri->bridge_forward_delay);
	printf("\n aging time\t\t");
	lkl_br_show_timer(&bri->ageing_time);
	printf("\n hello timer\t\t");
	lkl_br_show_timer(&bri->hello_timer_value);
	printf("\t\t\ttcn timer\t\t");
	lkl_br_show_timer(&bri->tcn_timer_value);
	printf("\n topology change timer\t");
	lkl_br_show_timer(&bri->topology_change_timer_value);
	printf("\t\t\tgc timer\t\t");
	lkl_br_show_timer(&bri->gc_timer_value);
	printf("\n flags\t\t\t");
	if (bri->topology_change)
		printf("TOPOLOGY_CHANGE ");
	if (bri->topology_change_detected)
		printf("TOPOLOGY_CHANGE_DETECTED ");
	printf("\n");
	printf("\n");
	printf("\n");

	err = lkl_br_foreach_port(br, lkl_dump_port_info, NULL);
	if (err < 0)
		printf("can't get ports: %s\n", strerror(-err));
	
	return 0;
}

int do_show_mac_table(struct params* parameters)
{
	lkl_br_showmacs((char *) parameters->p[0]);
	return 0;
}

int do_show_bridge_interface(struct params* parameters)
{
	struct bridge_info binfo;

	lkl_br_get_bridge_info((char*) parameters->p[0], &binfo);

	printf("bridge name\t\tbridge id\t\tSTP enabled\t\tinterfaces\n%s\t\t",(char*) parameters->p[0]);
	fflush(stdout);
	lkl_br_dump_bridge_id((unsigned char*) &binfo.bridge_id);
	fflush(stdout);
	printf("\t\t%s", (binfo.stp_enabled ? "yes" : "no"));
	fflush(stdout);
	lkl_br_foreach_port((char*) parameters->p[0], lkl_br_dump_interface, NULL);
	printf("\n");

	return 0;
}

int do_show_stp(struct params *params)
{
	struct bridge_info binfo;
	int err;

	if ((err = lkl_br_get_bridge_info(params->p[0], &binfo))) {
		printf("could not get bridge info : %s\n", strerror(err));
		return 1;
	}

	dump_info(params->p[0],&binfo);
	
	return 0;
}

int do_set_stp(struct params* params)
{
	int value = 1, err;
	if (strcmp((char*) params->p[1],"on")) {
		value = 0;
	}

	err = lkl_br_set_stp((char*)params->p[0], value);
	if (err < 0) {
		printf("LKL init :: could not set stp on %s :: %d\n", (char*)params->p[0], err);
	}

	return 0;
}
