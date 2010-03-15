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

all: conf_test bridge time_server

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

#.o: %.c $(INC)
#	$(CC) -c $(CFLAGS) $< -o $@

#conf/parser.o:conf/parser.c parser
#	$(CC) $(CFLAGS) -c -o conf/parser.o conf/parser.c

#parser: conf/parser.l
#	flex --outfile=conf/parser.c conf/parser.l

#conf_test: parser $(CONFOBJ)
#	$(CC) $(CFLAGS) $(CONFOBJ) conf_test.c -o bin/conf_test 

time_server: $(CONFOBJ) apps/time_server.o $(INC) $(CROSS)lkl/lkl.a
	$(CC) $(CFLAGS) -o bin/time_server apps/time_server.c lkl/lkl.a -lpthread $(CONFOBJ)

# Conf {

CONF_DIR=conf
CONF_SRC=$(CONF_DIR)/conf_tree.c $(CONF_DIR)/parser.c
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
BRIDGE_SRC=$(BRIDGE_DIR)/bridge.c
BRIDGE_OBJ=$(patsubst %c,%o,$(BRIDGE_SRC))

.PHONY: bridge
bridge: bin/bridge

bin/bridge: $(CONF_OBJ) $(BRIDGE_OBJ)
	$(CC) $(CFLAGS) $(CONF_OBJ) $(BRIDGE_OBJ) -o bin/bridge

# }

# Test {

TEST_DIR=apps;
TEST_SRC=$(TEST_DIR)/ping.c
TEST_OBJ=$(patsubst %c,%o,$(TEST_SRC))

ping: bin/ping

bin/ping: $(INC) $(CROSS)lkl/lkl.a
	gcc -Wall -Iinclude -Ilklinclude -o bin/ping apps/ping.c lkl/lkl.a -lpthread
# }

# Clean up {

.PHONY: clean
clean:
	-rm conf/parser.c
	-rm bin/conf_test
	-rm bridge/*.o
	-rm apps/*.o
	-rm conf/*.o
	-rm bin/bridge
	-rm bin/time_server

clean-all: clean
	-rm -rf lkl
	
# }
