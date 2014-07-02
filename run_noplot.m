compileslow;

time=1;
%initial call
[V, gE] = conductance('dummy',{'gE'});

while time<20000
    time=time+1;
    [V, gE] =conductance(V,{'gE'});
end
