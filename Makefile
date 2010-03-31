CC=gcc
FLEX=flex
CFLAGS=-g -Wall -Ilklinclude -Iinclude

# Environment {

HERE=$(PWD)
LINUX=$(HERE)/../linux-2.6
MKDIR=mkdir -p

# }

APR_CONF=$(shell apr-config --includes --cppflags)

EXTRA_FLAGS=-gstabs+

all: conf_test bridge hub test

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

.PHONY: conf_test
conf_test: bin/conf_test

bin/conf_test: $(CONF_OBJ)
	$(CC) $(CFLAGS) $(CONF_OBJ) conf_test.c -o bin/conf_test

# }

# Flex parser {

conf/parser.c: conf/parser.l
	$(FLEX) --outfile=$@ $<

conf/parser.o: conf/parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

PARSER=conf/parser.o

# }

# Bridge {

BRIDGE_DIR=bridge
BRIDGE_SRC=$(BRIDGE_DIR)/bridge.c packet.c
BRIDGE_OBJ=$(patsubst %c,%o,$(BRIDGE_SRC))

.PHONY: bridge
bridge: bin/bridge

bin/bridge: $(CONF_OBJ) $(BRIDGE_OBJ)
	$(CC) $(CFLAGS) $(CONF_OBJ) $(BRIDGE_OBJ) -o bin/bridge

# }

# Switch {

SWITCH_DIR=switch
SWITCH_SRC=$(SWITCH_DIR)/switch.c interface.c topology.c
SWITCH_OBJ=$(patsubst %c,%o,$(SWITCH_SRC))

.PHONY: switch
switch: bin/switch

bin/switch: $(INC) $(CONF_OBJ) $(SWITCH_OBJ) $(CROSS)lkl/lkl.a
	$(CC) $(CFLAGS) $(CONF_OBJ) $(SWITCH_OBJ) -o bin/switch lkl/lkl.a -pthread

# }

# Test {

TEST_DIR=apps;
TEST_SRC=$(TEST_DIR)/test.c
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
	-rm bin/conf_test
	-rm bridge/*.o
	-rm apps/*.o
	-rm conf/*.o
	-rm bin/bridge
	-rm bin/switch
	-rm bin/hub

clean-all: clean
	-rm -rf lkl
	
# }
