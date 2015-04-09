vars=$(
for x in job.o* 
do
    jobno=$(echo $x | tr '-' ' ' | awk '{print $2}')
    first=$(tail -1 $x | awk '{print $1}' | tr  '=' ' ' | awk '{print $2}')
    second=$(tail -1 $x | awk '{print $2}' | tr  '=' ' ' | awk '{print $2}')
    echo  $jobno  $first $second
done | sort -n) 
echo "$vars" V | tail
echo -e "plot '-' using 1:2  , '-' using 1:3  \n" "$vars" "\n" e "\n""$vars"| gnuplot -persist
