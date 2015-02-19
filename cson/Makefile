#!/usr/bin/make -f
########################################################################
# Main makefile for libcson.
#
# Important targets for users not hacking this source tree:
#
#  all: do everything
#  amal: build the amalgamation build
#  run: run tests (it's not called 'test' because we have a binary with
#       that name)
########################################################################
include config.make # see that file for certain configuration options.

CSON_ENABLE_CGI ?= 1


libcson.LIB.OBJECTS := \
	cson.o \
	parser/JSON_parser.o \
	cson_session.o \
	cson_session_file.o

CFLAGS_C99 = $(filter-out -std=c89,$(CFLAGS)) -std=c99

ifeq (0,1)
# sanity check for long double support
  CPPFLAGS += -DHAVE_LONG_DOUBLE
  CPPFLAGS += -DCSON_DOUBLE_T_PFMT=\".8Lf\"
endif

SQLITE3_H := $(firstword $(wildcard $(addsuffix /sqlite3.h,. /usr/include /usr/local/include $(HOME)/include)))
# too damned slow: $(shell locate sqlite3.h 2>/dev/null | grep -w include >/dev/null 2>&1 && echo 1)

########################################################################
# Set up a local/embedded copy of sqlite3 for some of my test purposes...
# i found out afterwards: http://www.sqlite.org/compile.html#omitfeatures
# The use of -DSQLITE_OMIT_xxx here isn't supposed to work, but it seems
# to do what i want (remove the requirement for -ldl).
ifeq (2,$(words $(wildcard ./sqlite3.c ./sqlite3.h)))
SQLITE3.O := ./sqlite3.o
CPDO_ENABLE_SQLITE3 := 1
LDFLAGS_SQLITE3 := -L. -lsqlite3
sqlite3.c:
sqlite3.h:
sqlite3.o: sqlite3.c sqlite3.h
sqlite3.o: CPPFLAGS:=\
	-I. \
	$(CPPFLAGS) \
	-DSQLITE_THREADSAFE=0 \
	-DSQLITE_TEMP_STORE=3 \
	-DSQLITE_OMIT_LOAD_EXTENSION=1
libsqlite3.a: sqlite3.o
	$(AR) crs $@ sqlite3.o
# C99: sqlite3.h does not compile cleanly in C89 mode
# due to its use of (long long).
#CPPFLAGS:=-I. $(filter-out -I.,$(CPPFLAGS))
$(SQLITE3.O) cpdo_amalgamation.o cson_sqlite3.o: CPPFLAGS:=-I. $(CFLAGS_C99)
cpdo_amalgamation.o: libsqlite3.a
endif
# /local sqlite3
########################################################################

ifeq (,$(SQLITE3_H))
  CPDO_ENABLE_SQLITE3 ?= 0
else
  CPDO_ENABLE_SQLITE3 ?= 1
endif

ifeq (1,$(CPDO_ENABLE_SQLITE3))
    LDFLAGS_SQLITE3 ?= -lsqlite3
else
    LDFLAGS_SQLITE3 ?=
endif

BIN_MYSQL5_CONFIG := $(call ShakeNMake.CALL.FIND_FILE,mysql_config)
ifneq (,$(BIN_MYSQL5_CONFIG))
    CPDO_ENABLE_MYSQL5 ?= 1
else
    CPDO_ENABLE_MYSQL5 ?= 0
    LDFLAGS_MYSQL5 :=
endif

ifeq (1,$(CPDO_ENABLE_MYSQL5))
    LDFLAGS_MYSQL5 := $(shell $(BIN_MYSQL5_CONFIG) --libs)
    CFLAGS_MYSQL5 := $(shell $(BIN_MYSQL5_CONFIG) --cflags)
endif

ifneq (00,$(CPDO_ENABLE_SQLITE3)$(CPDO_ENABLE_MYSQL5))
  libcson.LIB.OBJECTS += \
	cson_session_cpdo.o \
	cson_cpdo.o \
	cpdo_amalgamation.o \
	$(SQLITE3.O)
  CSON_ENABLE_CPDO := 1
else
  CSON_ENABLE_CPDO := 0
endif

CPPFLAGS.CPDO := -DCSON_ENABLE_CPDO=$(CSON_ENABLE_CPDO)
ifeq (1,$(CPDO_ENABLE_SQLITE3))
    CPPFLAGS.CPDO += -DCPDO_ENABLE_SQLITE3=1
  session/sessions.sqlite3: session/EMPTY.sessions.sqlite3
	cp $^ $@
  cson_session_cpdo.o: session/sessions.sqlite3
endif
ifeq (1,$(CPDO_ENABLE_MYSQL5))
    CPPFLAGS.CPDO += -DCPDO_ENABLE_MYSQL5=1
endif
cson_session_cpdo.o cpdo_amalgamation.o cson_cpdo.o: CPPFLAGS+=$(CPPFLAGS.CPDO)

#libcson.LIB.OBJECTS := $(patsubst %,$(SRC_DIR)/%,$(libcson.LIB.OBJECTS))
cson.o: cson_list.h
cson_list.h:
BINS_LDFLAGS :=
ifeq (1,$(CPDO_ENABLE_SQLITE3))
    cson_sqlite3.o test-sq3.o:  CPPFLAGS+=-DCSON_ENABLE_SQLITE3=1
    libcson.LIB.OBJECTS += cson_sqlite3.o
    libcson.DLL.LDFLAGS += $(LDFLAGS_SQLITE3)
    BINS_LDFLAGS += $(LDFLAGS_SQLITE3)
endif

ifeq (1,$(CPDO_ENABLE_MYSQL5))
    libcson.DLL.LDFLAGS += $(LDFLAGS_MYSQL5)
    BINS_LDFLAGS += $(LDFLAGS_MYSQL5)
endif


########################################################################
# libwhio-based session management...
CSON_ENABLE_ZLIB ?= 0# zlib is optionally used by whio, but cson doesn't use it
CSON_ENABLE_WHIO ?= 0# enables/disables the whio_ht-based cson_sessmgr implementation.
ifeq (1,$(CSON_ENABLE_WHIO))
    IO_AMAL.C := whio_amalgamation.c
    IO_AMAL.H := include/wh/whio/whio_amalgamation.h
    IO_AMAL.O := whio_amalgamation.o
    cson_session.o: CPPFLAGS+=-DCSON_ENABLE_WHIO=$(CSON_ENABLE_WHIO)
    $(IO_AMAL.O) cson_session_whio_ht.o: CPPFLAGS+=-DWHIO_CONFIG_ENABLE_ZLIB=$(CSON_ENABLE_ZLIB)
    $(IO_AMAL.O): $(IO_AMAL.C)
    $(IO_AMAL.C): $(IO_AMAL.H)
    libcson.LIB.OBJECTS += cson_session_whio_ht.o $(IO_AMAL.O)
    libcson.LIB.OBJECTS += cson_session_whio_epfs.o
    session/sessions.whio_ht: session/EMPTY.sessions.whio_ht
	cp $^ $@
    session/sessions.whio_epfs: session/EMPTY.sessions.whio_epfs.gz
	gzip -c -d < $^ > $@
    cson_session_whio_ht.o: session/sessions.whio_ht
    cson_session_whio_epfs.o: session/sessions.whio_epfs
    libwhio.LIB.OBJECTS := $(IO_AMAL.O)
    ifeq (1,$(CSON_ENABLE_ZLIB))
      libwhio.LIB.LDFLAGS := -lz
    endif
    $(eval $(call ShakeNMake.CALL.RULES.LIBS,libwhio))
    cson_session_whio_ht.o: $(libwhio.LIB)
    CLEAN_FILES += $(libwhio.LIB)
endif
ifeq (1,$(CSON_ENABLE_ZLIB))
   libcson.DLL.LDFLAGS += -lz
endif
.PHONY: whio
whio:
	$(MAKE) all CSON_ENABLE_WHIO=1 CSON_ENABLE_ZLIB=1
# /whio
########################################################################


libcson.DLL.OBJECTS := $(libcson.LIB.OBJECTS)
libcson.DLL: $(libcson.LIB.OBJECTS)
$(eval $(call ShakeNMake.CALL.RULES.LIBS,libcson))
$(eval $(call ShakeNMake.CALL.RULES.DLLS,libcson))

test.BIN.OBJECTS := test.o
test.BIN.LDFLAGS := $(BINS_LDFLAGS)
$(eval $(call ShakeNMake.CALL.RULES.BIN,test))
test.o: CFLAGS:=$(CFLAGS_C99)
test.o: cson.c cson_list.h # test.c imports these files directly

json-parser.BIN.OBJECTS := json-parser.o $(libcson.LIB)
json-parser.BIN.LDFLAGS := $(BINS_LDFLAGS)
#json-parser.o: CFLAGS:=$(CFLAGS_C99)
$(eval $(call ShakeNMake.CALL.RULES.BIN,json-parser))
$(test.BIN): $(libcson.LIB) 
$(json-parser.BIN): $(libcson.DLL) 
all: $(test.BIN) $(json-parser.BIN) $(test-sq3.BIN)

########################################################################
# cson_cgi...
ifeq (1,$(CSON_ENABLE_CGI))
  CGI_DIR := ./cgi
  CSON_CGI.OBJECTS := \
	$(patsubst %,$(CGI_DIR)/%, \
		cson_cgi.o \
	)

  libcson_cgi.LIB.OBJECTS := $(libcson.LIB.OBJECTS) $(CSON_CGI.OBJECTS)
#  libcson_cgi.DLL.OBJECTS := $(libcson_cgi.LIB.OBJECTS)
  CLEAN_FILES += $(CGI_DIR)/*.o
  $(eval $(call ShakeNMake.CALL.RULES.LIBS,libcson_cgi))
#  $(eval $(call ShakeNMake.CALL.RULES.DLLS,libcson_cgi))
  $(libcson_cgi.LIB): $(libcson_cgi.LIB.OBJECTS)
#  $(libcson_cgi.DLL): $(libcson_cgi.DLL.OBJECTS)

  cgi-test.BIN.OBJECTS := cgi-test.o
  cgi-test.BIN.LDFLAGS := $(BINS_LDFLAGS) -L. -lcson_cgi $(LDFLAGS_MYSQL5) $(LDFLAGS_SQLITE3) 

  cgi-test.o: CFLAGS:=$(CFLAGS_C99)
  $(eval $(call ShakeNMake.CALL.RULES.BIN,cgi-test))
  ifneq (,$(FCGI.LDFLAGS))
    libcson_cgi.DLL.LDFLAGS += $(FCGI.LDFLAGS)
    $(CGI_DIR)/cson_cgi.o: CPPFLAGS+=$(FCGI.CPPFLAGS) -DCSON_CGI_ENABLE_FCGI=1
    $(cgi-test.BIN): LDFLAGS+=$(FCGI.LDFLAGS)
  endif

  $(CGI_DIR)/cson_cgi.o cson_session.o cson_session_cpdo.o: CPPFLAGS+=-DCSON_ENABLE_CPDO=$(CSON_ENABLE_CPDO)
  all: $(libcson_cgi.LIB)
  all: $(cgi-test.BIN)
endif
# /cson_cgi
########################################################################

ifeq (1,$(CPDO_ENABLE_SQLITE3))
    test-sq3.BIN.OBJECTS := test-sq3.o $(libcson.LIB)
    test-sq3.BIN.LDFLAGS := $(BINS_LDFLAGS)
    $(eval $(call ShakeNMake.CALL.RULES.BIN,test-sq3))
    test-sq3.o: CFLAGS+=-std=c99
    $(test-sq3.BIN): $(libcson.LIB)
    all: $(test-sq3.BIN)
endif

ifeq (1,$(CSON_ENABLE_CPDO))
    test-cpdo.BIN.OBJECTS := test-cpdo.o $(libcson.LIB)
    test-cpdo.BIN.LDFLAGS := $(BINS_LDFLAGS)
    $(eval $(call ShakeNMake.CALL.RULES.BIN,test-cpdo))
    test-cpdo.o: CPPFLAGS+=-DCSON_ENABLE_CPDO=1 -DCPDO_ENABLE_SQLITE3=$(CPDO_ENABLE_SQLITE3) -DCPDO_ENABLE_MYSQL5=$(CPDO_ENABLE_MYSQL5)
    test-cpdo.o: CFLAGS:=$(CFLAGS_C99)
    $(test-cpdo.BIN): $(libcson.LIB)
    all: $(test-cpdo.BIN)

endif

ifeq (1,$(CSON_ENABLE_CPDO))
    select-to-json.BIN.OBJECTS := select-to-json.o $(libcson.LIB)
    select-to-json.BIN.LDFLAGS := $(BINS_LDFLAGS)
    $(eval $(call ShakeNMake.CALL.RULES.BIN,select-to-json))
    select-to-json.o: CPPFLAGS+=-DCSON_ENABLE_CPDO=1 -DCPDO_ENABLE_SQLITE3=$(CPDO_ENABLE_SQLITE3) -DCPDO_ENABLE_MYSQL5=$(CPDO_ENABLE_MYSQL5)
#    select-to-json.o: CFLAGS+=-std=c99
    $(select-to-json.BIN): $(libcson.LIB)
    all: $(select-to-json.BIN)
endif

########################################################################
# Create cson_lists.h from the cson_list.h supermacro. We need
# this for building the amalgamation.
LISTS_H := cson_lists.h
$(LISTS_H): cson_list.h Makefile
	@echo "Creating $@..."
	@echo "/* Auto-generated from cson_list.h. Edit at your own risk! */" > $@
	@gcc -E -DLIST_T=cson_value_list -DVALUE_T=cson_value\ \* cson_list.h -DVALUE_T_IS_PTR=1 >> $@
	@gcc -E -DLIST_T=cson_kvp_list -DVALUE_T=cson_kvp\ \* cson_list.h -DVALUE_T_IS_PTR=1 >> $@
	@sed -e '/^#/d' < $@ > $@.foo && mv $@.foo $@
test.o: $(LISTS_H) # b/c test.c #includes cson.c directly
cson.o: $(LISTS_H)
# CLEAN_FILES += $(LISTS_H)

########################################################################
# Amalgamation build: stuff all required public headers into a single
# megaheader and all required sources (and private headers) into a
# single C file. This requires a good deal of hackery due to how
# cson.c uses cson_list.h to create list implementation code.
amal:
	bash ./createAmalgamation.sh
CLEAN_FILES += $(wildcard cson_amalgamation_*.[cho])


########################################################################
# create emacs tags...
bin_etags := $(shell which etags 2>/dev/null)
ifneq (,$(bin_etags))
    tagfile := TAGS
    FORCE:
    .PHONY: FORCE tags
    $(tagfile): FORCE
	find . -name '*.[ch]' -print | $(bin_etags) -f $@ -
    tags: $(tagfile)
    CLEAN_FILES += $(tagfile)
    #$(wildcard *.o): $(tagfile)
    #cson.o: $(tagfile)
endif
# ^^^ bin_etags

########################################################################
# run sanity tests...
run: $(json-parser.BIN)
	@echo "Running MUST-PASS tests..."
	@echo -e "\t./$(test.BIN)"; \
		./$(test.BIN) 1>/dev/null 2>&1 \
		|| { x=$$?; echo "Basic test app failed with code [$$x]."; exit $$x; }
	@for i in json/pass-*.json checker/test/pass*.json; do \
		echo -e "\t$$i"; \
		./$(json-parser.BIN) -f $$i 1>/dev/null || exit; \
	done
	@echo "Running MUST-FAIL tests..."
	@for i in json/fail-*.json checker/test/fail*.json; do \
		echo -ne "\t$$i"; \
		./$(json-parser.BIN) -f $$i 1>/dev/null 2>&1 && exit 100; \
		echo " successfully failed."; \
	done; \
	true

$(filter-out cson_amalgamation.o,$(wildcard *.o)): Makefile config.make common.make
