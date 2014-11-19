function [hv] = setupplot(t,i,names)
gridsize=length(t.data(:,1));
figure(i)
set(gcf,'keypress','global k;k=1;disp(k)');
hv=imagesc(t.data,[t.min t.max]);
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
title(names{i-2});
