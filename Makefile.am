AM_CFLAGS = -g -Wall -Iinclude -Ilklinclude

GTKFLGS = `pkg-config --cflags gtk+-2.0`
GTKLIBS = `pkg-config --libs gtk+-2.0`
LKL = lkl/lkl.a

bin_PROGRAMS = hyper hypergui router switch firewall nat hub bridge
lib_LIBRARIES = liblklnetconfig.a

liblklnetconfig_a_SOURCES = config/config.c config/parser.l

hyper_SOURCES = hypervisor/hypervisor.c hypervisor/hypervisor_cmd.c console.c autocomplete.c interface.c device.c
hyper_CFLAGS = -DISHYPERVISOR $(AM_CFLAGS)
hyper_LDADD = liblklnetconfig.a
hyper_LINK = $(CCLD) -pthread -lreadline -o $@

hypergui_SOURCES = hypervisor/hypergui.c hypervisor/hypervisor_cmd.c console.c autocomplete.c interface.c device.c gtktopology.c
hypergui_CFLAGS = -DISHYPERVISOR $(AM_CFLAGS) $(GTKFLGS)
hypergui_LDADD = liblklnetconfig.a
hypergui_LINK = $(CCLD) -pthread -lreadline $(GTKLIBS) -o $@

router_SOURCES = devices/router/router_cmd.c devices/router/router.c interface.c devices/switch/switch_cmd.c device.c console.c autocomplete.c
router_CFLAGS = -DISROUTER -DISLKL $(AM_CFLAGS)
router_LDADD = liblklnetconfig.a lkl/lkl.a
router_LINK = $(CCLD) -pthread -lreadline -o $@

switch_SOURCES = devices/switch/switch.c devices/switch/switch_cmd.c interface.c topology.c console.c autocomplete.c devices/router/router_cmd.c device.c
switch_CFLAGS = -DISSWITCH -DISLKL $(AM_CFLAGS)
switch_LDADD = liblklnetconfig.a lkl/lkl.a
switch_LINK = $(CCLD) -pthread -lreadline -o $@

firewall_SOURCES = devices/firewall/firewall.c devices/firewall/firewall_cmd.c ipt_common.c interface.c topology.c console.c autocomplete.c device.c
firewall_CFLAGS = -DISFIREWALL -DISLKL $(AM_CFLAGS)
firewall_LDADD = liblklnetconfig.a lkl/lkl.a
firewall_LINK = $(CCLD) -pthread -lreadline -o $@

nat_SOURCES = devices/nat/nat.c devices/nat/nat_cmd.c ipt_common.c interface.c topology.c console.c autocomplete.c device.c
nat_CFLAGS = -DISNAT -DISLKL $(AM_CFLAGS)
nat_LDADD = liblklnetconfig.a lkl/lkl.a
nat_LINK = $(CCLD) -pthread -lreadline -o $@

hub_SOURCES = devices/hub/hub.c

bridge_SOURCES = devices/bridge/bridge.c packet.c interface.c device.c
bridge_LDADD = liblklnetconfig.a
bridge_LINK = $(CCLD) -pthread -o $@
