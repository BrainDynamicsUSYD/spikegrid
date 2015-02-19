#!/bin/bash
########################################################################
# Generates an "amalgamation build" for cson, including only the components
# needed for adding cson to the Fossil SCM system.
# Output is two files, cson_amalgamation_fossil.{c,h}.
########################################################################

LIB_BASH=createAmalgamation-lib.bash
. ${LIB_BASH} || {
    echo "Could not source config/lib script [${LIB_BASH}]."
    exit 127
}

AMAL_SOURCES="
parser/JSON_parser.h
parser/JSON_parser.c
${dir_src}/cson.c
${dir_src}/cson_lists.h
${dir_src}/cson_sqlite3.c
"
# Reminder: fossil doesn't need cson_session_file.c
# but it is the reference session impl and is used without any sort of
# #if block.

# The ordering of headers is important for the amalgamation build
AMAL_HEADERS="
${inc_cson}/cson.h
${inc_cson}/cson_sqlite3.h
"
#set -x
AMAL_FOSSIL_C=cson_amalgamation_fossil.c
AMAL_FOSSIL_H=cson_amalgamation_fossil.h
wh_create_code \
    ${AMAL_HEADERS} > ${AMAL_FOSSIL_H}
wh_create_code \
    -i ${AMAL_FOSSIL_H} \
    ${AMAL_SOURCES} > ${AMAL_FOSSIL_C}

perl -i -p \
    -e 's|\s*#\s*define\s*CSON_ENABLE_SQLITE3.*|#define CSON_ENABLE_SQLITE3 1|;' \
    -e 's|\s*#\s*define\s*CSON_CGI_ENABLE_POST_FORM_URLENCODED.*|#define CSON_CGI_ENABLE_POST_FORM_URLENCODED 0|;' \
    ${AMAL_FOSSIL_H}

echo "Done:"

for i in ${AMAL_FOSSIL_H}; do
    {
    echo '#ifdef FOSSIL_ENABLE_JSON'
    cat <<EOF
#ifndef CSON_FOSSIL_MODE
#define CSON_FOSSIL_MODE
#endif
EOF
    cat $i
    echo '#endif /* FOSSIL_ENABLE_JSON */'
    } > xxx
    mv xxx $i
done 
for i in ${AMAL_FOSSIL_C}; do
    {
    echo '#ifdef FOSSIL_ENABLE_JSON'
    cat $i
    echo '#endif /* FOSSIL_ENABLE_JSON */'
    } > xxx
    mv xxx $i
done

ls -la ${AMAL_FOSSIL_C} ${AMAL_FOSSIL_H}

CFLAGS="${CFLAGS} -std=c89 -DCSON_ENABLE_SQLITE3=1 -DFOSSIL_ENABLE_JSON" 
wh_try_compilers ${AMAL_FOSSIL_C}

myfsl=$HOME/cvs/fossil/fossil/src
if [[ -d $myfsl ]]; then
    echo "Copying to fossil tree [${myfsl}] ..."
    h=$myfsl/cson_amalgamation.h
    cp ${AMAL_FOSSIL_H} $h
    c=$myfsl/cson_amalgamation.c
    sed -e "s|${AMAL_FOSSIL_H}|cson_amalgamation.h|" \
        ${AMAL_FOSSIL_C} > $c
fi
