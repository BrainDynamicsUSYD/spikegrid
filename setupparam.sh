#/bin/bash
files=config/*
actualfiles=$(for var in config/*; do echo $var;basename $var ;done)
FILE=$(dialog --title "select a parameter file" --stdout --title "select a parameter file" --menu  "test" $(( $(tput lines) -10)) $(($(tput cols)-5)) $(( $(tput lines) -15))  $actualfiles )
if [ -z "${FILE}"]; then
    FILE='config/parametersJOHN-travelling.h'
fi
echo '//disable warnings about float conversion in this file only
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif' > whichparam.h
echo '#include "'${FILE}'"' >> whichparam.h
echo '#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif' >> whichparam.h
