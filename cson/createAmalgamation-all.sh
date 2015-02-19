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
${AMAL_CORE_C}
${AMAL_SESSION_C}
${AMAL_CGI_C}
"

if [[ ! -f ${AMAL_CORE_C} ]]; then
    ./createAmalgamation-core.sh \
        || wh_die $? "Creation of core amalgamation failed."
fi
if [[ ! -f ${AMAL_SESSION_C} ]]; then
    ./createAmalgamation-session.sh \
        || wh_die $? "Creation of session amalgamation failed."
fi
if [[ ! -f ${AMAL_CGI_C} ]]; then
    ./createAmalgamation-cgi.sh \
        || wh_die $? "Creation of cgi amalgamation failed."
fi

# The ordering of headers is important for the amalgamation build
AMAL_HEADERS="
${AMAL_CORE_H}
${AMAL_SESSION_H}
${AMAL_CGI_H}
"
wh_create_code ${AMAL_HEADERS} > ${AMAL_BASENAME}.h
wh_create_code -i ${AMAL_BASENAME}.h ${AMAL_SOURCES} > ${AMAL_BASENAME}.c

echo "Done:"

ls -la cson_amalgamation.[ch]

CFLAGS="${CFLAGS} -std=c89 -DCSON_ENABLE_CPDO=1"
# reminder: CSON_ENABLE_CPDO is needed for the SESSION bits, not CGI.
wh_try_compilers cson_amalgamation.c
