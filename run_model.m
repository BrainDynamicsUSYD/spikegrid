compile;
time=1;
%initial call
[V, gE] = conductance('dummy','gE');

% Membrane potential
figure(1);
hV = imagesc(V.Vex ,[-80 -55]);
gridsize=length(V.Vex(:,1));
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
colormap('default');
colorbar;
hT = title('Time: 1');

% Excitatory conductance
figure(2);
hG = imagesc(gE.data,[gE.min gE.max]);
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');

while time<20000
    time=time+1;
    [V, gE] =conductance(V,'gE');
    if (mod(time,10)==0)
        set(hV,'CData',V.Vex);
        set(hT,'String',sprintf('Time: %.1f',time));
        set(hG,'CData',gE.data);  
        drawnow;
    end
end
