compile;
time=1;
%initial call
[V, out1,out2] = conductance('dummy','STDU1','STDR1');

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
% Excitatory conductance
h=setupplot(out1,3);
h2=setupplot(out2,4);
while time<20000
    time=time+1;
    [V, out1,out2] =conductance(V,'STDU2','STDR2');
    if (mod(time,10)==0)
        set(hV,'CData',V.Vex);
        set(hVi,'CData',V.Vin);
        set(hT,'String',sprintf('Time: %.1f',time));
        set(h,'CData',out1.data);  
        set(h2,'CData',out2.data);  
        drawnow;
    end
end
