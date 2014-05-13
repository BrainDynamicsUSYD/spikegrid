compile;
precision = 'single';
data =((rand(100)*25)-80);
datalayer2 =((rand(100)*25)-80);
time=1;

if strcmp(precision,'single')==1
    data = single(data);
    datalayer2 = single(datalayer2);
    time = single(time);
end

% Membrane potential
figure(1);
hV = imagesc(data ,[-80 -55]); colorbar;
hT = title('Time: 1');

% Excitatory conductance
[data2,datalayer22, gE] =conductance(data,datalayer2,'gE');
figure(2);
hG = imagesc(datalayer2);

while time<1000
    time=time+1;
    [data2, datalayer22,gE] =conductance(data,datalayer2,'gE');
    data=data2;
    datalayer2=datalayer22;
    if (mod(time,10)==0)
        set(hV,'CData',data);
        set(hT,'String',sprintf('Time: %.1f',time));
        set(hG,'CData',datalayer2);  
        pause(0.01);
    end
end
