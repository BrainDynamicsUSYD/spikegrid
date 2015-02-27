#!/do/not/sh
# To be sourced by createAmalgamation*.sh. Contains shared vars/functions
# for those scripts.

# Various directories, relative to this dir.
inc_wh=include/wh
inc_cson=${inc_wh}/cson
inc_cpdo=${inc_wh}/cpdo
dir_src=.


# AMAL_BASENAME=base name of generated output files.
AMAL_BASENAME=cson_amalgamation
# AMAL_CORE_x=C/header file for the core whio components.
AMAL_CORE_C=${AMAL_BASENAME}_core.c
AMAL_CORE_H=${AMAL_BASENAME}_core.h
# AMAL_SESSION_x=C/header file for the "utility" whio components.
AMAL_SESSION_C=${AMAL_BASENAME}_session.c
AMAL_SESSION_H=${AMAL_BASENAME}_session.h
# AMAL_CGI_x=C/header file for the EPFS whio components.
AMAL_CGI_C=${AMAL_BASENAME}_cgi.c
AMAL_CGI_H=${AMAL_BASENAME}_cgi.h

function wh_die()
{
    rc=$1
    shift
    echo "FATAL: $@" 1>&2
    exit $rc
}

########################################################################
# Reads from stdin, removes any #include lines which "appear" to point
# to wh-related headers, and sends them to stdout.
function wh_stripinc()
{
    sed \
        -e '/# *include *[^<].*cson.*\.h/d' \
        -e '/# *include *["<].*JSON_parser\.h/d' \
        -e '/# *include *["<].*tokenize_path\.h/d' \
        -e '/# *include *["<].*whuuid.[hc]/d' \
        -e '/# *include *["<]wh\//d'
}

########################################################################
# Outputs some C Header prefix code, reads file $1, and pipes it
# through wh_stripinc(), sending the results to stdout.
function wh_filter_code()
{
    echo "/* begin file $1 */"
    false && {
        # The source file/line number info is occassinoally
        # useful but breaks debuggers when an app links to the
        # amalgamation.
        echo "#line 2 \"$1\"" # the number is a kludge :/
    }
    wh_stripinc < $1
    echo "/* end file $1 */"
}

########################################################################
# For each file name passed to this function, check if it exists. If not,
# exit with a fatal error.
function wh_confirm_exists()
{
    local i
    for i in "$@"; do
        test -e $i && continue
        wh_die 127 "required file [$i] not found."
    done
}

########################################################################
# Combines a set of .c or .h files into a single file, finagling some
# of the output for reasons of its own.
#
# Usage:
#  wh_create_code [-i HEADER_FILE_TO_INCLUDE] ...source files... >  OUTPUT_FILE_NAME
#
# The -i parameter should be used when creating the .c part of the
# amalgamation, e.g.:
#
#  wh_create_code amal.c -i amal.h ...list of files to filter...
#
# -i may be specified multiple times to include multiple headers.
#
# When creating the associated amal.h:
#
#  wh_create_code ...list of files to filter... > amal.h
function wh_create_code()
{
#    local outfile=$1
#    shift
#    local timestamp=$(date)
#    echo "Creating ${outfile}..." 1>&2
    echo "/* auto-generated! Do not edit! */"
    {
#        echo "#line 1 \"${outfile}\""
#        echo "#if !defined(_POSIX_C_SOURCE)"
#        echo "#define _POSIX_C_SOURCE 200112L /* needed for ftello() and friends */"
#        echo "#endif"
#        echo "#if !defined(_ISOC99_SOURCE)"
#        echo "#define _ISOC99_SOURCE 1 /* needed for snprintf() */"
#        echo "#endif"
        [[ x != "x${incl}" ]] && echo "${incl}"

        local skip=0
        for h in "$@"; do
            if [[ "-i" = "$h" ]]; then
                skip=1
                continue
            fi
            if [[ 1 = $skip ]]; then
                echo "#include \"$h\""
                skip=0
                continue
            else
                wh_confirm_exists "$h"
                wh_filter_code "$h"
            fi
        done
    }
}

########################################################################
# Assumes that $@ is a complete compilation command for compiling an
# amalgamation build. $@ is executed and on error the app exits
# with that error code.
function wh_try_compile()
{
    local cmd="$@"
    echo "Trying to compile: $cmd"
    $cmd || {
        local err=$?
        echo "Exiting with error code $err."
        exit $err
    }
    echo "It worked!"
}

########################################################################
# Tries to compile $@ using gcc and tcc (if available). Exits with
# an error if compilation fails.
function wh_try_compilers()
{
    local CC=$(which gcc)
    local src="$@"
    if [[ -x "${CC}" ]]; then
        wh_try_compile ${CC} ${CPPFLAGS} ${CFLAGS} -c -pedantic -Wall ${src}
    fi
    false && {
        # cson_cgi won't yet compile in tcc due to c99 VLAs.
        CC=$(which tcc)
        if [[ -x "${CC}" ]]; then
            wh_try_compile ${CC} -c -Wall ${src}
        fi
    }
    true
}
