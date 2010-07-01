AM_CFLAGS = -g -Wall -Iinclude -Ilklinclude

GTKFLGS = `pkg-config --cflags gtk+-2.0 gtksourceview-1.0`
GTKLIBS = `pkg-config --libs gtk+-2.0 gtksourceview-1.0`
LKL = lkl/lkl.a

bin_PROGRAMS = bin/hyper bin/hypergui bin/router bin/switch bin/firewall bin/nat bin/hub bin/bridge bin/echo
lib_LIBRARIES = liblklnetconfig.a

liblklnetconfig_a_SOURCES = config/config.c config/parser.l

bin_hyper_SOURCES = hypervisor/hypervisor.c hypervisor/hypervisor_cmd.c console.c autocomplete.c interface.c device.c
bin_hyper_CFLAGS = -DISHYPERVISOR $(AM_CFLAGS)
bin_hyper_LDADD = liblklnetconfig.a
bin_hyper_LINK = $(CCLD) -pthread -lreadline -o $@

bin_hypergui_SOURCES = hypervisor/hypergui.c hypervisor/hypervisor_cmd.c console.c autocomplete.c interface.c device.c gtktopology.c
bin_hypergui_CFLAGS = -DISHYPERVISOR $(AM_CFLAGS) $(GTKFLGS)
bin_hypergui_LDADD = liblklnetconfig.a
bin_hypergui_LINK = $(CCLD) -lm -pthread -lreadline $(GTKLIBS) -o $@

bin_router_SOURCES = devices/router/router_cmd.c devices/router/router.c interface.c devices/switch/switch_cmd.c device.c console.c autocomplete.c
bin_router_CFLAGS = -DISROUTER -DISLKL $(AM_CFLAGS)
bin_router_LDADD = liblklnetconfig.a lkl/lkl.a
bin_router_LINK = $(CCLD) -pthread -lreadline -o $@

bin_switch_SOURCES = devices/switch/switch.c devices/switch/switch_cmd.c interface.c topology.c console.c autocomplete.c devices/router/router_cmd.c device.c
bin_switch_CFLAGS = -DISSWITCH -DISLKL $(AM_CFLAGS)
bin_switch_LDADD = liblklnetconfig.a lkl/lkl.a
bin_switch_LINK = $(CCLD) -pthread -lreadline -o $@

bin_firewall_SOURCES = devices/firewall/firewall.c devices/firewall/firewall_cmd.c ipt_common.c interface.c topology.c console.c autocomplete.c device.c
bin_firewall_CFLAGS = -DISFIREWALL -DISLKL $(AM_CFLAGS)
bin_firewall_LDADD = liblklnetconfig.a lkl/lkl.a
bin_firewall_LINK = $(CCLD) -pthread -lreadline -o $@

bin_nat_SOURCES = devices/nat/nat.c devices/nat/nat_cmd.c ipt_common.c interface.c topology.c console.c autocomplete.c device.c
bin_nat_CFLAGS = -DISNAT -DISLKL $(AM_CFLAGS)
bin_nat_LDADD = liblklnetconfig.a lkl/lkl.a
bin_nat_LINK = $(CCLD) -pthread -lreadline -o $@

bin_hub_SOURCES = devices/hub/hub.c

bin_bridge_SOURCES = devices/bridge/bridge.c packet.c interface.c device.c
bin_bridge_LDADD = liblklnetconfig.a
bin_bridge_LINK = $(CCLD) -pthread -o $@

bin_echo_SOURCES = apps/test.c
bin_echo_CFLAGS = $(AM_CFLAGS)
bin_echo_LDADD = lkl/lkl.a
bin_echo_LINK = $(CCLD) -pthread -o $@
