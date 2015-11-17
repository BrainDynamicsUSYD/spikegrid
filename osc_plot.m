data=importdata('osc-data');
offset=[];
offseterrs=[];
means=[];
errs=[];
actuals=[];
count=0;
for i=1:2:1000
    subdata=data(data(:,1)==i,:);
    if (size(subdata,1)>1)
        count=count+1;
        means(count)=mean(subdata(:,3));
        errs(count)=std(subdata(:,3))/sqrt(size(subdata,1));
        actuals(count)=subdata(1,4);
        offset(count)=mean(subdata(:,2));
        offseterrs(count)=std(subdata(:,2))/sqrt(size(subdata,1));
    end
end
hold off
errorbar(means,errs,'.');
hold all
errorbar(offset,offseterrs,'.');
plot(actuals)

