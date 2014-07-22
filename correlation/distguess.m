size=45;
testpoint=vret(:,25,25);
corrsk1=[];
inds=[];
count=1;
corrs=[];
for x=1:size
    for y=1:size
        lagcount=80;
        c1=xcorr(testpoint,vret(:,x,y),lagcount,'coeff');
        corrsk1(count)=sum(c1((lagcount+2):end)' .* (exp(-(1:lagcount)/20)-exp(-(1:lagcount)/5)));
        inds(x,y)=count;
        count=count+1;
    end
end
disp(sprintf('correlation done'))
%next step - use the correlations and order them to provide distance estimates
[s1,ind1]=sort(corrsk1,'descend');
[f1,g1]=sort(ind1);
out=[];
for x=1:size
    x
    for y=1:size
        d1=ncloser(g1(inds(x,y))); %roughly the number of points that are closer
        actdist=dist([x,y],[25 25]);
        out(x,y)=(actdist-d1);
        dista(x,y)=d1;
    end
end
contourf(out)
figure
imagesc(dista)
mean(mean(abs(out)))
