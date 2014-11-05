%Script showing how to extract some spiking data into matlab - calculates some funny variances at  the moment
vfrates=[];
vstates=[];
parfor j=1:40
    fname=sprintf('job-%i/1.txt',j);
    fid=fopen(fname);
    tline=fgetl(fid);
    lines={};
    count=1;
    %read in all spikes as (x1,x2,.....) (y1,y2,....)
    while ischar(tline)
        if length(tline>0)
            data=textscan(tline,'%d%d','Delimiter',',:');
        else
            data=[ { [] },{ [] } ];
        end
        lines{count}=[data{1}+1, data{2}+1];
        count = count+1;
        tline=fgetl(fid);
    end
    %now sort by firing
    spiketimes=cell(101);
    for c=1:(count-1)
        curline=lines{c};
        if (length(curline)>0)
            xcs=curline(:,1);
            ycs=curline(:,2);
            for sc=1:length(xcs)
                xc=xcs(sc);
                yc=ycs(sc);
                oldspikes=spiketimes(xc,yc);
                spiketimes(xc,yc) = {[oldspikes{1} c]};
            end
        end
    end

    for t=1:10
        varfrates=zeros(101);
        varstates=zeros(101);
        for x=1:100
            for y=1:100
                elem=spiketimes(x,y);
                tt=elem{1};
                tt=tt(tt<t*1000 & tt>(t-1)*1000);
                zz=zeros(1000,1);
                zz(mod(tt,1000))=1
                varstates(x,y)=var(zz);
                varfrates(x,y)=var(diff(tt));
            end
        end
        varfrate=mean(mean(varfrates(~isnan(varfrates))));
        varstate=mean(mean(varstates));
        vfrates(j,t)=varfrate;
        vstates(j,t)=varstate;
    end

end
figure(1)
plot(vfrates')
figure(2)
plot(vstates')

