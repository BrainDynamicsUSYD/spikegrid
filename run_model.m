exit=0;
!make evolvegen.c
compile;
time=1;
%outputs={ 'STDU1','STDU2' };
outputs={ 'STDP1','STDP2' };
%outputs={};
%initial call
[V, out1] = conductance('dummy',outputs);
global k
k=0;
% Membrane potential
figure(1);
set(gcf,'keypress','global k;k=1;disp(k)');
hV = imagesc(V.Vex ,[-80 -55]);
gridsize=length(V.Vex(:,1));
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
colormap('default');
colorbar;
hT = title('Time: 1');
figure(2);
set(gcf,'keypress','global k;k=1;disp(k)');
hVi = imagesc(V.Vin ,[-80 -55]);
gridsize=length(V.Vin(:,1));
set(gca,'XLim',[0.5 gridsize+0.5],'YLim',[0.5 gridsize+0.5],'XTick',[],'YTick',[],...
    'XTickLabel',[],'YTickLabel',[],'XGrid','off','YGrid','off');
colormap('default');
title('Vin');
colorbar;
h=[];
for i=1:length(out1)
    h(i)=setupplot(out1{i},i+2,outputs);
end
tic
while time<5000
    time=time+1;
    if (mod(time,10)==0)
        disp(time)
        [V, out1] =conductance(V,outputs);
        set(hV,'CData',V.Vex);
        set(hVi,'CData',V.Vin);
        set(hT,'String',sprintf('Time: %.1f',time));
        for i=1:length(out1)
            set(h(i),'CData',out1{i}.data);  
        end
        drawnow;
    else
        [V, out1] =conductance(V,{});
    end
    if (k==1)
        return
    end
end
toc
