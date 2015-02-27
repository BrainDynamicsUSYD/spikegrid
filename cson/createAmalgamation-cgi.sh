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

AMAL_SOURCES="
cgi/whuuid.h
cgi/whuuid.c
cgi/cson_cgi.c
"

# The ordering of headers is important for the amalgamation build
AMAL_HEADERS="
${inc_cson}/cson_cgi.h
"

#set -x
wh_create_code \
    -i ${AMAL_SESSION_H} \
    ${AMAL_HEADERS} > ${AMAL_CGI_H}
wh_create_code -i ${AMAL_CGI_H} ${AMAL_SOURCES} > ${AMAL_CGI_C}

echo "Done:"

ls -la ${AMAL_CGI_C} ${AMAL_CGI_H}

CFLAGS="${CFLAGS} -std=c89 -DCSON_ENABLE_CPDO=1"
# reminder: CSON_ENABLE_CPDO is needed for the SESSION bits, not CGI.
wh_try_compilers ${AMAL_CGI_C}
