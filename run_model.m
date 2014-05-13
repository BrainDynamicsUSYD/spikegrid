compile
data=single((rand(100)*25)-80);
data2=single((rand(100)*25)-80);
time=0
while time<1000
    time=time+1;
    [data3 data4 gE gI] =conductance(data,data2,'gE','gI');
    data=data3;
    data2=data4;
    if (mod (time,10)==0)
        figure(1)
        imagesc(data ,[-80 -55])
        colorbar
        title(time)
        figure(2)
        imagesc(gE)
        colorbar
        title('gE')
        figure(3)
        imagesc(gI)
        title('gI')
        colorbar
        drawnow
        disp(time)
        
    end
end
