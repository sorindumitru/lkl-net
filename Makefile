CC=gcc
CFLAGS=-g -Wall -Ilklinclude -Iinclude

HERE=$(PWD)
LINUX=$(HERE)/../linux-2.6

CFLAGS_LIN=$(shell apr-config --includes --cppflags)
CFLAGS_OS=$(CFLAGS_LIN)

EXTRA_FLAGS=-gstabs+

CONFSRC=$(shell ls conf/*.c)
CONFOBJ=$(patsubst %c,%o,$(CONFSRC))

BRIDGESRC=$(shell ls bridge/*.c)
BRIDGEOBJ=$(patsubst %c,%o,$(BRIDGESRC))

all: conf_test bridge time_server

lklinclude/asm:
	-mkdir -p `dirname $@`
	ln -s $(LINUX)/arch/lkl/include/asm lklinclude/asm

lklinclude/x86:
	-mkdir -p `dirname $@`
	ln -s $(LINUX)/arch/x86 lklinclude/x86

lklinclude/asm-generic:
	-mkdir -p `dirname $@`
	ln -s $(LINUX)/include/asm-generic lklinclude/asm-generic

lklinclude/linux:
	-mkdir -p `dirname $@`
	ln -s $(LINUX)/include/linux lklinclude/linux

INC=lklinclude/asm lklinclude/asm-generic lklinclude/x86 lklinclude/linux

$(CROSS)lkl/.config: .config
	mkdir -p lkl && \
	cp $< $@
	
$(CROSS)lkl/lkl.a: lkl/.config
	cd $(LINUX) && \
	$(MAKE) O=$(HERE)/$(CROSS)lkl ARCH=lkl \
	CROSS_COMPILE=$(CROSS) LKLENV_CFLAGS="$(CFLAGS_OS)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	lkl.a

%.o: %.c $(INC)
	$(CC) -c $(CFLAGS) $< -o $@

conf/parser.o:conf/parser.c parser
	$(CC) $(CFLAGS) -c -o conf/parser.o conf/parser.c

parser: conf/parser.l
	flex --outfile=conf/parser.c conf/parser.l

conf_test: parser $(CONFOBJ)
	$(CC) $(CFLAGS) $(CONFOBJ) conf_test.c -o bin/conf_test 

bridge: parser $(CONFOBJ) $(BRIDGEOBJ);
	$(CC) $(CFLAGS) $(CONFOBJ) $(BRIDGEOBJ) -o bin/bridge

time_server: $(CONFOBJ) apps/time_server.o $(INC) $(CROSS)lkl/lkl.a
	$(CC) $(CFLAGS) -o bin/time_server apps/time_server.c lkl/lkl.a -lpthread $(CONFOBJ)

clean:
	rm conf/parser.c
	rm bin/conf_test
	rm bridge/*.o
	rm apps/*.o
	rm conf/*.o
	rm bin/bridge
	rm bin/time_server
