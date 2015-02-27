#!/usr/bin/make -f
default: all

CFLAGS+=-fPIC

Makefile:
WHIO.A := ../libwhio.a
WHIO.O := ../whio_amalgamation.o
WHIO.C := ../whio_amalgamation.c
WHIO.H := ../include/wh/whio/whio_amalgamation.h

$(WHIO.A): Makefile $(WHIO.C) $(WHIO.H)
	$(MAKE) -C .. $(notdir $@) CSON_ENABLE_ZLIB=1 CSON_ENABLE_WHIO=1

whio-ht-tool.o: CFLAGS+=-std=c99 -I../include
#whio-ht-tool: LDFLAGS+=-L.. -lwhio -lz# just in case whio is built with it
whio-ht-tool: Makefile
	$(CC) -o $@ whio-ht-tool.o -L.. -lwhio -lz
whio-ht-tool: whio-ht-tool.o

#whio-ht-tool: whio-ht-tool.o $(WHIO.O)
all: whio-ht-tool

CLEAN_FILES += $(wildcard *~ *.o) whio-ht-tool
clean:
	-rm -f foo-bar $(CLEAN_FILES)
