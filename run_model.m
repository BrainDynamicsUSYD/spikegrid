compile;
precision = 'single';
data =((rand(100)*25)-80);
time=1;

if strcmp(precision,'single')==1
    data = single(data);
    time = single(time);
end

% Membrane potential
figure(1);
hV = imagesc(data ,[-80 -55]); colorbar;
hT = title('Time: 1');

% Excitatory conductance
[data2, gE] =conductance(data,'gE');
figure(2);
hG = imagesc(gE);

while time<1000
    time=time+1;
    [data2, gE] =conductance(data,'gE');
    data=data2;
    if (mod(time,10)==0)
        set(hV,'CData',data);
        set(hT,'String',sprintf('Time: %.1f',time));
        set(hG,'CData',gE);  
        pause(0.01);
    end
end
