#!/bin/bash
########################################################################
# Generates an "amalgamation build" for whio. Output is two files,
# ${AMAL_C} and ${AMAL_H}. If all goes well they can be compiled
# as-is to get a standalone whefs/whio library.
#
########################################################################

parts="core session cgi all"
for p in $parts; do
    script=./createAmalgamation-$p.sh
    $script || {
        err=$?
        echo "FAILED: [${script}]"
        exit $err
    }
done
ls -ls cson_amalgamation*.[ch]

exit 0

