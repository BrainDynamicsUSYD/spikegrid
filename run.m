compile
data=single((rand(100)*25)-80);
time=0
while time<1000
    time=time+1;
    data2=conductance(data);
    data=data2;
    if (mod (time,10)==0)
        imagesc(data ,[-80 -55])
        colorbar
        title(time)
        pause(0.1)
        sprintf('time %i, min %f,max %f',time,min(min(data)),max(max(data)))
    end
end
