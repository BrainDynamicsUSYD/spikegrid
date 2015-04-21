vars=$(
for x in job.o* 
do
    #jobno=$(echo $x | tr '-' ' ' | awk '{print $2}')
    #first=$(tail -1 $x | awk '{print $1}' | tr  '=' ' ' | awk '{print $2}')
    #second=$(tail -1 $x | awk '{print $2}' | tr  '=' ' ' | awk '{print $2}')
    #echo  $jobno  $first $second
    tail -1 $x
done | sort -n) 
echo "$vars" | tr ' ' ','
echo -e "plot '-' using 1:2  smooth unique , '-' using 1:3 smooth unique \n" "$vars" "\n" e "\n""$vars"| gnuplot -persist

