compile;
time=1;
outputs={ 'Coupling1','Coupling2','gE'};
%initial call
[V, out1] = conductance('dummy',outputs);

% Membrane potential
figure(1);
hV = imagesc(V.Vex ,[-80 -55]);
gridsize=length(V.Vex(:,1));
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
colormap('default');
colorbar;
hT = title('Time: 1');
figure(2);
hVi = imagesc(V.Vin ,[-80 -55]);
gridsize=length(V.Vin(:,1));
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
colormap('default');
title('Vin');
colorbar;
h=[];
for i=1:length(out1)
    h(i)=setupplot(out1{i},i+2);
end
while time<20000
    time=time+1;
    [V, out1] =conductance(V,outputs);
    if (mod(time,10)==0)
        set(hV,'CData',V.Vex);
        set(hVi,'CData',V.Vin);
        set(hT,'String',sprintf('Time: %.1f',time));
        for i=1:length(out1)
            set(h(i),'CData',out1{i}.data);  
        end
        drawnow;
    end
end
