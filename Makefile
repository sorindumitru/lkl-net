CC=gcc
CFLAGS=-g -Wall -Iinclude

HERE=$(pwd)
LINUX=$(HERE)/linux-2.6

CONFSRC=$(shell ls conf/*.c)
CONFOBJ=$(patsubst %c,%o,$(CONFSRC))

BRIDGESRC=$(shell ls bridge/*.c)
BRIDGEOBJ=$(patsubst %c,%o,$(BRIDGESRC))

all: conf_test bridge

include/asm:
	-mkdir `dirname $@`
	ln -s $(LINUX)/arch/lkl/include/asm include/asm

include/x86:
	-mkdir `dirname $@`
	ln -s $(LINUX)/arch/x86 include/x86

include/asm-generic:
	-mkdir `dirname $@`
	ln -s $(LINUX)/include/asm-generic include/asm-generic

include/linux:
	-mkdir `dirname $@`
	ln -s $(LINUX)/include/linux include/linux

parser: conf/parser.lex 
	flex --outfile=conf/parser.c conf/parser.lex


conf_test: parser $(CONFOBJ)
	$(CC) $(CFLAGS) $(CONFOBJ) conf_test.c -o bin/conf_test 

bridge: $(CONFOBJ) $(BRIDGEOBJ);
	$(CC) $(CFLAGS) $(CONFOBJ) $(BRIDGEOBJ) -o bin/bridge

clean:
	rm conf/parser.c
	rm bin/conf_test
	rm *.o
	rm conf/*.o
