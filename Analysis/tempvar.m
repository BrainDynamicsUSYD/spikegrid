!make evolvegen.c
compile;
[V,out1]=conductance('dummy',{});
validout=[];
markfreq=500;
means=[];
for i=0:(markfreq*10-1)
    [V,out1]=conductance(V,{});
    validout(mod(i,markfreq)+1,:,:)=V.Vex> -55;
    if (mod(i,markfreq)==0 && i>0)
        disp(i)
        vars=[];
        for x=1:100
            for y=1:100
                vars(x,y)=var(validout(:,x,y));
            end
        end
    validout=[];
    means(i/markfreq)=mean(mean(vars));
    end
end
plot(means)
