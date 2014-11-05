opengl software
U=0.13;
D=0.2;
F=0.45;
totalu=[];
totalr=[];
count=0;
dts=linspace(0,0.5,20);
ns=1:20;
for dt=dts
    count=count+1;
    rvals=[];
    uvals=[];
    uvals(1)=U;
    rvals(1)=1;
    for n=ns(2:end)
        uvals(n)=U+uvals(n-1)*(1-U)*exp(-dt/F);
        rvals(n)=1+(rvals(n-1)-rvals(n-1)*uvals(n-1)-1)*exp(-dt/D);
    end
    totalu(count,:)=uvals(:);
    totalr(count,:)=rvals(:);
end
mesh(ns,dts,totalu.*totalr);
ylabel('dt')
xlabel('n')
