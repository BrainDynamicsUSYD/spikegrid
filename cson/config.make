########################################################################
# This file should be included by all other Makefiles in the project.
# It can be tweaked to enable or disable certain compile-time library
# features.
default: all
all:

########################################################################
# You can touch these:
########################################################################
ShakeNMake.QUIET := 0# set to 1 to enable "quiet mode"

########################################################################
# Set ENABLE_DEBUG to 1 to enable whefs debugging information.
DEBUG ?= 1
ENABLE_DEBUG ?= $(DEBUG)

########################################################################
# Set USE_PTHREADS to 1 to enable the pthreads-based mutex in the
# paging allocator. (Not currently used.)
USE_PTHREADS := 1


GCC_CFLAGS := -pedantic -Wall -Werror -fPIC

CFLAGS += $(GCC_CFLAGS)
CXXFLAGS += $(GCC_CFLAGS)

########################################################################
# Don't touch anything below this line unless you need to tweak it to
# build on your box:
CONFIG.MAKEFILE := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
$(CONFIG.MAKEFILE):
TOP_SRCDIR_REL := $(dir $(CONFIG.MAKEFILE))
TOP_SRCDIR_REL := $(patsubst %/,%,$(TOP_SRCDIR_REL))# horrible kludge
#TOP_SRCDIR := $(shell cd -P $(TOP_SRCDIR_REL) && pwd)
TOP_SRCDIR := $(realpath $(TOP_SRCDIR_REL))
#$(error TOP_SRCDIR_REL=$(TOP_SRCDIR_REL)   TOP_SRCDIR=$(TOP_SRCDIR))
TOP_INCDIR := $(TOP_SRCDIR_REL)/include
INCLUDES += -I. -I$(TOP_INCDIR)
CPPFLAGS += $(INCLUDES)

SRC_DIR ?= .#$(PWD)
ShakeNMake.CISH_SOURCES := \
	$(wildcard $(SRC_DIR)/*.cpp) \
	$(wildcard $(SRC_DIR)/*.c) \
	$(wildcard $(SRC_DIR)/parser/*.c) \
	$(wildcard *.cpp)

CFLAGS += -std=c89

########################################################################
# common.make contains config-independent make code.
include $(TOP_SRCDIR_REL)/common.make
ALL_MAKEFILES := $(PACKAGE.MAKEFILE) $(ShakeNMake.MAKEFILE) $(CONFIG.MAKEFILE)
$(ALL_MAKEFILES):

########################################################################
# Try FastCGI (or not).
# Some nimrod defined the following in fcgi_stdio.h:
#
# DLLAPI size_t     FCGI_fwrite(void *ptr, size_t size, size_t nmemb, FCGI_FILE *fp);
#
# Notice the non-const first parameter, which is in direct conflict
# with every other write() function on the planet. i.e. it's not drop-in replacable
# with fwrite() like it's supposed to be.
SEARCH_PATH.H := $(HOME)/include /usr/local/include /usr/include 
SEARCH_PATH.DLL := $(HOME)/lib /usr/local/lib /usr/lib
FCGI.H := $(call ShakeNMake.CALL.FIND_FILE,fcgi_stdio.h,$(SEARCH_PATH.H))
TRY_FCGI ?= 0
ifneq (0,$(TRY_FCGI))
ifeq (.,$(TOP_SRCDIR_REL))
  ifneq (,$(FCGI.H))
    FCGI.LIB := $(call ShakeNMake.CALL.FIND_FILE,libfcgi.so,$(SEARCH_PATH.DLL))
    ifeq (,$(FCGI.LIB))
      FCGI.LIB := $(call ShakeNMake.CALL.FIND_FILE,libfcgi.a,$(SEARCH_PATH.DLL))
    endif
    ifneq (,$(FCGI.LIB))
        FCGI.CPPFLAGS := -I$(dir $(FCGI.H))
        FCGI.LDFLAGS := -L$(dir $(FCGI.LIB)) -lfcgi
        $(info Enabling FCGI support.)
    endif
  endif
  ifeq (,$(FCGI.LIB))
    $(info FCGI not found. Disabling.)
  endif
endif
endif

########################################################################
ifeq (1,$(ENABLE_DEBUG))
  CPPFLAGS += -UNDEBUG -DDEBUG=1
  CFLAGS += -g
  CXXFLAGS += -g
else
  CFLAGS += -O2
  CPPFLAGS += -UDEBUG -DNDEBUG=1
endif

CLEAN_FILES += $(wildcard *.o *~)
