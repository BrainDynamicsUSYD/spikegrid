function [hv] = setupplot(t,i)
gridsize=length(t.data(:,1));
figure(i)
hv=imagesc(t.data,[t.min t.max]);
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
