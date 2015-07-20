#!/bin/bash
#bash arrays are fun
actualfiles=()
for var in config/*;
do
    #quoting and using a bash array here is very important
    actualfiles+=($(basename $var) "$(head -1 $var | tr -d '/' )")
done
#generate the dialog box - this is some of the most scary bash I have ever written
FILE=$(dialog --title "select a parameter file" --stdout --title "select a parameter file" --menu  \"test\" $(( $(tput lines) -10)) $(($(tput cols)-5)) $(( $(tput lines) -15)) "${actualfiles[@]}")
#correct the file name to use the config directory
if [ -z "${FILE}" ]; then
    #for computers without dialog, specify a default file
    FILE='config/parametersJOHN-travelling.h' 
else
    FILE=$(echo config/$FILE)
fi
#now generate the actual.h file
echo '//disable warnings about float conversion in this file and most importantly what it includes
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
