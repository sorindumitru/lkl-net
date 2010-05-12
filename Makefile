CC=gcc
FLEX=flex
CFLAGS=-g -Wall -Ilklinclude -Iinclude
LDLIBS=-pthread -lreadline

# Environment {

HERE=$(PWD)
LINUX=$(HERE)/../linux-2.6
MKDIR=mkdir -p

# }

APR_CONF=$(shell apr-config --includes --cppflags)

EXTRA_FLAGS=-gstabs+

all: bridge hub test switch router hypervisor

# Include LKL {

lklinclude/asm:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/arch/lkl/include/asm lklinclude/asm

lklinclude/x86:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/arch/x86 lklinclude/x86

lklinclude/asm-generic:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/include/asm-generic lklinclude/asm-generic

lklinclude/linux:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/include/linux lklinclude/linux

INC=lklinclude/asm lklinclude/asm-generic lklinclude/x86 lklinclude/linux


# }

# LKL {

$(CROSS)lkl/.config: .config
	mkdir -p lkl && \
	cp $< $@
	
$(CROSS)lkl/lkl.a: lkl/.config
	cd $(LINUX) && \
	$(MAKE) O=$(HERE)/$(CROSS)lkl ARCH=lkl \
	CROSS_COMPILE=$(CROSS) LKLENV_CFLAGS="$(APR_CONF)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	lkl.a

# }

# Conf {

CONF_DIR=conf
CONF_SRC=$(CONF_DIR)/config.c $(CONF_DIR)/parser.c
CONF_OBJ=$(patsubst %c,%o,$(CONF_SRC))

# }

# Flex parser {

conf/parser.c: conf/parser.l
	$(FLEX) --outfile=$@ $<

conf/parser.o: conf/parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

PARSER=conf/parser.o

# }

# Console {

CONSOLE_SRC=console.c autocomplete.c

# }

# Bridge {

BRIDGE_DIR=bridge
BRIDGE_SRC=$(BRIDGE_DIR)/bridge.c packet.c interface.c device.c
BRIDGE_OBJ=$(patsubst %c,%o,$(BRIDGE_SRC))

.PHONY: bridge
bridge: bin/bridge

bin/bridge: $(CONF_OBJ) $(BRIDGE_OBJ)
	$(CC) $(CFLAGS) -c interface.c -o interface.o
	$(CC) $(CFLAGS) $(CONF_OBJ) $(BRIDGE_OBJ) -o bin/bridge $(LDLIBS)

# }

# Switch {

SWITCH_DIR=switch
SWITCH_SRC=$(SWITCH_DIR)/switch.c interface.c topology.c $(CONSOLE_SRC) switch/switch_cmd.c router/router_cmd.c device.c
SWITCH_OBJ=$(patsubst %c,%o,$(SWITCH_SRC))

.PHONY: switch
switch: bin/switch

bin/switch: $(INC) $(CONF_OBJ) $(SWITCH_OBJ) $(CROSS)lkl/lkl.a
	$(CC) $(CFLAGS) -c console.c -DISSWITCH -o console.o
	$(CC) $(CFLAGS) -c autocomplete.c -o autocomplete.o
	$(CC) $(CFLAGS) -DISLKL -c interface.c -o interface.o
	$(CC) $(CFLAGS) $(CONF_OBJ) $(SWITCH_OBJ) -o bin/switch lkl/lkl.a $(LDLIBS) -DISLKL

# }

# Router {

ROUTER_DIR=router
ROUTER_SRC=$(ROUTER_DIR)/router.c interface.c $(CONSOLE_SRC) switch/switch_cmd.c router/router_cmd.c device.c
ROUTER_OBJ=$(patsubst %c,%o,$(ROUTER_SRC))

.PHONY: router
router: bin/router

bin/router: $(INC) $(CONF_OBJ) $(ROUTER_OBJ) $(CROSS)lkl/lkl.a
	$(CC) $(CFLAGS) -c console.c -DISROUTER -o console.o
	$(CC) $(CFLAGS) -DISLKL -c interface.c -o interface.o
	$(CC) $(CFLAGS) $(CONF_OBJ) $(ROUTER_OBJ) -o bin/router lkl/lkl.a $(LDLIBS) -DISLKL

# }

# Hypervisor {

HYPERVISOR_DIR=hypervisor
HYPERVISOR_SRC=$(HYPERVISOR_DIR)/hypervisor.c $(HYPERVISOR_DIR)/hypervisor_cmd.c $(CONSOLE_SRC) interface.c device.c
HYPERVISOR_OBJ=$(patsubst %c,%o,$(HYPERVISOR_SRC))

.PHONY: hypervisor
hypervisor: bin/hypervisor

bin/hypervisor: $(CONF_OBJ) $(HYPERVISOR_OBJ)
	$(CC) $(CFLAGS) -c console.c -DISHYPERVISOR -o console.o
	$(CC) $(CFLAGS) -c interface.c -o interface.o
	$(CC) $(CFLAGS) $(CONF_OBJ) $(HYPERVISOR_OBJ) -o bin/hypervisor $(LDLIBS) -DISHYPERVISOR=1

# }

# Test {

TEST_DIR=apps;
TEST_SRC=$(TEST_DIR)/test.c device.c
TEST_OBJ=$(patsubst %c,%o,$(TEST_SRC))

.PHONY: test
test: bin/test

bin/test: apps/test.c $(INC) $(CROSS)lkl/lkl.a
	$(CC) $(CFLAGS) -o bin/test apps/test.c lkl/lkl.a -lpthread
# }

# Hub {

HUB_DIR=hub
HUB_SRC=$(HUB_DIR)/hub.c
HUB_OBJ=$(patsubst %c,%o,$(HUB_SRC))

.PHONY: hub
hub: bin/hub
bin/hub: $(HUB_OBJ) 
	$(CC) $(CFLAGS) $< -o $@

# }

# Clean up {

.PHONY: clean
clean:
	-rm *.o
	-rm conf/parser.c
	-rm bridge/*.o
	-rm apps/*.o
	-rm conf/*.o
	-rm bin/bridge
	-rm bin/switch
	-rm bin/hub
	-rm switch/*.o
	-rm router/*.o

clean-all: clean
	-rm -rf lkl
	
# }
