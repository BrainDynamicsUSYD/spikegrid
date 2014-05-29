compile;
gridsize = 100;
precision = 'double';
V =((rand(gridsize)*25)-80);
V2 =((rand(gridsize)*25)-80);
time=1;

if strcmp(precision,'single')==1
    V = single(V);
    V2 = single(V2);
    time = single(time);
end

% Membrane potential
figure(1);
hV = imagesc(V ,[-80 -55]);
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
colormap('default');
colorbar;
hT = title('Time: 1');

% Excitatory conductance
[V_,V2_, gE] =conductance(V,V2,'gE'); % Was gI before. Why?
figure(2);
hG = imagesc(gE.data,[gE.min gE.max]);
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');

while time<20000
    time=time+1;
    [V, V2,gE] =conductance(V,V2,'gE');
    if (mod(time,10)==0)
        set(hV,'CData',V);
        set(hT,'String',sprintf('Time: %.1f',time));
        set(hG,'CData',gE.data);  
        drawnow;
    end
end
