compile
data=single((rand(100)*25)-80);
data2=single((rand(100)*25)-80);
time=0
while time<1000
    time=time+1;
    [data3 data4 gE] =conductance(data,data2,'gE');
    data=data3;
    data2=data4;
    if (mod (time,10)==0)
        figure(1)
        imagesc(data ,[-80 -55])
        colorbar
        title(time)
        figure(2)
        imagesc(gE)
        drawnow
        disp(time)
        
    end
end
