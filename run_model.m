compile
data=single((rand(100)*25)-80);
time=0
while time<1000
    time=time+1;
    [data2 gE]=conductance(data,'gE');
    data=data2;
    if (mod (time,10)==0)
        figure(1)
        imagesc(data ,[-80 -55])
        colorbar
        title(time)
        figure(2)
        imagesc(gE)
        pause(0.1)
        sprintf('time %i, min %f,max %f',time,min(min(data)),max(max(data)))
    end
end
