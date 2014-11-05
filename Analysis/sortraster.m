%calculates the mean lag between one neuron and all the others
%and uses that to plot a raster
fname='local_output/1.txt';
fid=fopen(fname);
tline=fgetl(fid);
for figno=1:16
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
        if (count==200)
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

    ty=1;
    tx=1;
    count = 1;
    for x=1:100
        for y=1:100
            e=spiketimes(x,y);
            elem=e{1};
            if length(elem) > count
                count=length(elem);
                tx=x;
                ty=y;
            end
        end
    end
    tf=spiketimes(tx,ty);
    tfire=tf{1};
    diffs=zeros(100);
    for x=1:100
        for y=1:100
            this_tfire=tfire;
            e=spiketimes(x,y);
            elem=e{1};
            diffs(x,y)=var(diff(elem));
            deltas=[];
            for c=1:length(tfire)
                t=elem(elem<this_tfire(c)) - this_tfire(c);
                elem=elem(elem>=this_tfire(c));
                deltas=[deltas (-t)];
            end
            if length(deltas) > 1
                diffs(x,y)=mean(deltas);
            else
                diffs(x,y)=Inf;
            end
        end
    end
    subaxis(8,2,figno)
    reshaped = reshape(diffs,1,100*100);
    [sorted,ind] = sort(reshaped);
    for x=1:20:(100*70)
        spikes = spiketimes(ind(x));
        elem=spikes{1};
        scatter(elem,ones(length(elem),1)*(x),'k.');
        hold on
    end
    scatter(tfire,ones(length(tfire),1));
    title(sprintf('%s',fname))
    drawnow
    hold off
end
fclose(fid);
