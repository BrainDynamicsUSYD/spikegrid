data=importdata('times')
times=data.data;
revs=data.textdata;
output=[];
outcount=1;
current=[];
curcount=1;
errors=[];
for i=2:length(times)
    if (strcmp(revs(i),revs(i-1))==0)
        curcount=1;
        output(outcount)=mean(current);
        errors(outcount)=std(current)/sqrt(curcount-1);
        outcount=outcount+1;
        current=[];
    end
    current(curcount)=times(i);
    curcount=curcount+1;
end
output(outcount)=mean(current);
errors(outcount)=std(current)/sqrt(curcount-1);
errorbar(1:length(output),output,errors);

