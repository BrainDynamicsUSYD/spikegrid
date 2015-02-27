#!/bin/bash
########################################################################
# Generates an "amalgamation build" for whio's core + session
# components. Output is two files, cson_amalgamation_session.{c,h}.
########################################################################

LIB_BASH=createAmalgamation-lib.bash
. ${LIB_BASH} || {
    echo "Could not source config/lib script [${LIB_BASH}]."
    exit 127
}

########################################################################
# enableWhio: we cannot add whio_amalgamation to the amalgamation
# build just yet because whio and cpdo both internally use
# separate copies of the same printf() implementation. The public
# APIs have different names but the internal types in both
# copies collide.
enableWhio=0

AMAL_SOURCES="
${dir_src}/cson_session.c
${dir_src}/cson_session_file.c
${dir_src}/cson_cpdo.c
${dir_src}/cson_session_cpdo.c
${dir_src}/cson_sqlite3.c
cpdo_amalgamation.c
"
if [[ 1 -eq ${enableWhio} ]]; then
    AMAL_SOURCES="${AMAL_SOURCES} whio_amalgamation.c"
fi

# The ordering of headers is important for the amalgamation build
AMAL_HEADERS="
${inc_cson}/cson_session.h
${inc_cson}/cson_sqlite3.h
${inc_wh}/cpdo/cpdo_amalgamation.h
${inc_cson}/cson_cpdo.h
"
if [[ 1 -eq ${enableWhio} ]]; then
    AMAL_HEADERS="${AMAL_HEADERS} ${inc_wh}/whio/whio_amalgamation.h"
fi

#set -x
wh_create_code \
    -i cson_amalgamation_core.h \
    ${AMAL_HEADERS} > ${AMAL_SESSION_H}
wh_create_code \
    -i ${AMAL_SESSION_H} \
    ${AMAL_SOURCES} > ${AMAL_SESSION_C}

echo "Done:"

ls -la ${AMAL_SESSION_C} ${AMAL_SESSION_H}

CFLAGS="${CFLAGS} -std=c89 -DCSON_ENABLE_CPDO=1" 
wh_try_compilers ${AMAL_SESSION_C}
