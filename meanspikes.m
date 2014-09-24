%calculates the mean lag between one neuron and all the others
fname='output/1.txt';
fid=fopen(fname);
tline=fgetl(fid);
for figno=1:30
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
        if (count==1000)
            break;
        end
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

    ty=26;
    tx=22;
    tf=spiketimes(tx,ty);
    tfire=tf{1};
    diffs=zeros(101);
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
                deltas=[deltas t];
            end
            diffs(x,y)=mean(deltas);
        end
    end
    plot(sort(reshape(diffs,1,101*101)))
    hold on
    drawnow
end
fclose(fid);
