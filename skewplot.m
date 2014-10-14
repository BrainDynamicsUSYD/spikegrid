%calculates the mean lag between one neuron and all the others
%and uses that to plot a raster
fname='local_output/1.txt';
fid=fopen(fname);
tline=fgetl(fid);
mylen=200;
mydata=[];
for figno=1:64
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
        if (count==mylen)
            break;
        end
    end
    %now sort by firing
    spiketimes=cell(100);
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

    fc = zeros(100);
    for x=1:100
        for y=1:100
            e=spiketimes(x,y);
            elem=e{1};
            fc(x,y)=length(elem);
        end
    end
    figure(1)
    subaxis(16,4,figno,'Padding',0,'Spacing',0)
    valid=[];
    sz=size(fc);
    count=1;
    for x=1:100
        for y=1:100
            if (mod(x,2)~=0 ||  mod(y,2)~=0)
                valid(count)=sub2ind(sz,x,y);
                count=count+1;
            end
        end
    end
    reshaped = reshape(fc,1,100*100);
    reshaped = reshaped(valid);
   % reshaped = reshaped(reshaped ~= 0);
    mymax=mylen/30;

    if(length(reshaped) == 0)
        continue
    end
    mydata(figno)=var(reshaped);
    ksdensity(reshaped,linspace(0,mymax,211),'bandwidth',0.5);
    xlim([0 mymax])
    ylim([0 0.8])
    set(gca,'XTickLabel',[])
    set(gca,'YTickLabel',[])
    drawnow
    hold off
end
figure(2)
plot(mydata);
fclose(fid);
