#!/bin/bash
########################################################################
# Generates an "amalgamation build" for whio's core library
# components. Output is two files, whio_amalgamation_core.{c,h}.
########################################################################

LIB_BASH=createAmalgamation-lib.bash
. ${LIB_BASH} || {
    echo "Could not source config/lib script [${LIB_BASH}]."
    exit 127
}

CSON_SRC="
parser/JSON_parser.h
parser/JSON_parser.c
${dir_src}/cson.c
${dir_src}/cson_lists.h
"

# The ordering of headers is important for the amalgamation build
CSON_HEADERS="
${inc_cson}/cson.h
"

AMAL_HEADERS="${CSON_HEADERS}"
AMAL_SOURCES="${CSON_SRC}"

wh_create_code ${AMAL_HEADERS} > ${AMAL_CORE_H}
wh_create_code -i ${AMAL_CORE_H} ${AMAL_SOURCES} > ${AMAL_CORE_C}

echo "Done:"

ls -la ${AMAL_CORE_C} ${AMAL_CORE_H}

CFLAGS="${CFLAGS} -std=c89" 
wh_try_compilers ${AMAL_CORE_C}
