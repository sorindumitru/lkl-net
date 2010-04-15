#include <stdio.h>
#include <stdlib.h>

#include <config.h>
#include <list.h>

#include <asm/lkl_bridge.h>
#include <asm/libnetlink.h>

#include <switch.h>
#include <switch_cmd.h>

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
	lkl_br_dump_bridge_id((unsigned char*) &binfo.bridge_id);
	fflush(stdout);
	printf("\t\t%s", (binfo.stp_enabled ? "yes" : "no"));
	fflush(stdout);
	lkl_br_foreach_port((char*) parameters->p[0], lkl_br_dump_interface, NULL);
	printf("\n");

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
