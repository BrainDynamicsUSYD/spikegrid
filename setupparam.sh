#/bin/bash
files=config/*
actualfiles=$(for var in config/*; do echo $var;basename $var ;done)
FILE=$(dialog --title "select a parameter file" --stdout --title "select a parameter file" --menu  "test" $(( $(tput lines) -10)) $(($(tput cols)-5)) $(( $(tput lines) -15))  $actualfiles )
echo '#include "'${FILE}'"' > whichparam.h
