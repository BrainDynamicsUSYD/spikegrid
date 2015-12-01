data=importdata('osc-data');
offset=[];
offseterrs=[];
means=[];
errs=[];
actuals=[];
count=0;
xcs=[];
for i=1:2:1000 %every second trial - avoids bias from most recent trial
    subdata=data(data(:,1)==i,:);
    if (size(subdata,1)>1)
        count=count+1;
        xcs(count)=i;
        means(count)=mean(subdata(:,3));
        errs(count)=std(subdata(:,3))/sqrt(size(subdata,1));
        actuals(count)=subdata(1,4);
        offset(count)=mean(subdata(:,2));
        offseterrs(count)=std(subdata(:,2))/sqrt(size(subdata,1));
    end
end
hold off
errorbar(xcs,offset,offseterrs,'.');
hold all
errorbar(xcs,means,errs,'.');
%plot(xcs,actuals)
xlim([0,350])
xlabel('trial number')
ylabel('probability of output along left path')
saveas(gcf,'~/model-osc.png')

