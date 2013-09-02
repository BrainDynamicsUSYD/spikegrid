clear all
set(0,'DefaultAxesFontSize',10,'DefaultTextFontSize',10);
f=[dir('out/*centers')]% ;dir('out/data/*centers')];
hold off
cmap=jet(200);
valid={};
vcount=1;
for x=1:length(f)
    fname=sprintf('out/%s',f(x).name);
    
    stat=importdata(fname);
    if isempty(stat)
        dummy____=fname
        continue;
    end
    
    xd=stat(:,1);
    errors=stat(:,2);
    stat=xd(xd>0);
    if length(stat)>0
        maxx=length(stat);
        sf=stat';%/stat(1)*1;
        range_=floor(logspace(0,log10(maxx),50));
        sf=sf(range_);
        %Problem - fitting is to highly weighted towards high tau (to many
        %points -  try log spacing of points to fix)
        fitstat=polyfit(log(range_),log(sf),1);
        if (fitstat(1)<110 && fitstat(1)>0 )
            disp(sprintf('%s MSD gradient %f',fname, fitstat(1)));
            if (length(strfind(f(x).name,'long')) >0  )
                errorbar(range_,sf,errors(range_),cmap(floor(fitstat(1)*50)+1,:),'linewidth',1)
                ax=get(gcf,'CurrentAxes');
                set(ax,'XScale','log','YScale','log')
                %loglog(range_,sf,'color',cmap(floor(fitstat(1)*50)+1,:),'linewidth',1)
            else
                errorbar(range_,sf,errors(range_),'color',cmap(floor(fitstat(1)*50)+100,:),'linewidth',1)
                ax=get(gcf,'CurrentAxes');
                set(ax,'XScale','log','YScale','log')
                %loglog(range_,sf,'color',cmap(floor(fitstat(1)*50)+100,:),'linewidth',1)
            end
            hold on
            %loglog(range_,range_.^fitstat(1)/10 )
            
            xlabel('\tau')
            ylabel('MSD (\tau)')
            valid{vcount}=f(x).name;
       %     valid{vcount*2}=sprintf('%s%s',f(x).name,'fit');
            vcount = vcount + 1;
        end
    end
end
%valid{2*vcount-1}='random walk';
%loglog(range,range)
legend(valid,'Location','NorthWest')
