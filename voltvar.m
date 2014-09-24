%calculates variances in voltage - incredibly slow as there is a rather crazy amount of data to process
for jobno=1:40
    jobno
    data=importdata(sprintf('job-%i/2.txt',jobno));
    res=[];
    parfor step=1:99
        tdata=[];
        baset=step*100;
        for t = 2:100
            tdata(t,:,:)=data(((t-1)*100:t*100)+baset*100,:);
        end
        vars=[];
        for x=1:100
            for y=1:100
                vars(x,y)=var(tdata(:,x,y));
            end
        end
        res(step) =  mean(mean(vars));
    end
    save(sprintf('job-%f.mat',jobno));
end
