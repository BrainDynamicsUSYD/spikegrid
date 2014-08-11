compile;
[V,out1]=conductance('dummy',{});
validout=[];

for i=1:1000
    [V,out1]=conductance(V,{});
    validout(i,:,:)=V.Vex> -55;
end
vars=[];
for x=1:100
    for y=1:100
        vars(x,y)=var(validout(:,x,y));
    end
end
disp(mean(mean(vars)))
