size=45;
sampletime=5270;
inv=reshape(vret,sampletime,size*size);%flatten
p=randperm(size*size);
permed=inv(:,p);
unshape=reshape(permed,sampletime,size,size);%flatten - unshape has neurons out of position
[p1,ind]=sort(p); %how to restore original locations
unpermed=reshape(permed(:,ind),sampletime,size,size); 
%get three points exact to start with
actuallocations=[];
[a1 b1] = ind2sub([size size],ind(1));
[a2 b2] = ind2sub([size size],ind(2));
[a3 b3] = ind2sub([size size],ind(3));
actuallocations(1,:)=[a1 b1];
actuallocations(2,:)=[a2 b2];
actuallocations(3,:)=[a3 b3];
k1=permed(:,1);
k2=permed(:,2);
k3=permed(:,3);
corrsk1=[];
corrsk2=[];
corrsk3=[];
%calculate correlations to each of the known points
parfor i=1:size*size
    if mod(i,100) == 0
        i
    end
    testdata=permed(:,i);
    lagcount=40;
    c1=xcorr(testdata,k1,lagcount);
    corrsk1(i)=sum(c1((lagcount+2):end)' .* (exp(-(1:lagcount)/20)-exp(-(1:lagcount)/5)));
    c2=xcorr(testdata,k2,lagcount);
    corrsk2(i)=sum(c2((lagcount+2):end)' .* (exp(-(1:lagcount)/20)-exp(-(1:lagcount)/5)));
    c3=xcorr(testdata,k3,lagcount);
    corrsk3(i)=sum(c3((lagcount+2):end)' .* (exp(-(1:lagcount)/20)-exp(-(1:lagcount)/5)));
end
disp(sprintf('correlation done'))
%next step - use the correlations and order them to provide distance estimates
[s1,ind1]=sort(corrsk1,'descend');
[s2,ind2]=sort(corrsk2,'descend');
[s3,ind3]=sort(corrsk3,'descend');
[f1,g1]=sort(ind1);
[f2,g2]=sort(ind2);
[f3,g3]=sort(ind3);
finallocations=[];
for i=1:size*size
    if mod(i,100) == 0
        i
    end
    d1=sqrt(g1(i)/pi); %roughly the number of points that are closer
    d2=sqrt(g2(i)/pi);
    d3=sqrt(g3(i)/pi);
    dest=@(x) abs(dist(x,actuallocations(1,:))-d1)+ abs(dist(x,actuallocations(2,:))-d2) + abs(dist(x,actuallocations(3,:))-d3); %function to minimise
    guess=[];
    if d1 < d2 && d1 < d3
        guess=actuallocations(1,:);
    elseif d2<d1 && d2<d3
        guess=actuallocations(2,:);
    elseif d3<d1 && d3<d2
        guess=actuallocations(3,:);
    else 
        guess=[40 40];
    end
    opts=optimoptions(@lsqnonlin,'Algorithm','Levenberg-Marquardt','Display','off');
    nonlinguess=lsqnonlin(dest,guess,[],[],opts);
    finallocations(i,:)=nonlinguess;
end
errrors=[]
%now calculate error for each estimate
for i=1:size*size
    [a b] = ind2sub([size size],ind(i));
    errors(i)=dist(finallocations(i,:),[a b]);
end
errorlocs=reshape(errors(:,ind),size,size); 
imagesc(errorlocs);
errors2=[];
%now calculate error for each estimate as comparison
for i=1:size*size
    [a b] = ind2sub([size size],ind(i));
    [c d] = ind2sub([size size],permed(i));
    errors2(i)=dist([c d],[a b]);
end
errorlocs2=reshape(errors2(:,ind),size,size); 
figure
imagesc(errorlocs2);
disp(mean(mean(errorlocs)))
disp(mean(mean(errorlocs2)))
