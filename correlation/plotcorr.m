corrmat=[];
for x=2:49
    for y=2:49
   %     corr= corrcoef(reshape(vret(:,(x-1):(x+1),(y-1):(y+1)),50,9));%),vret(:,x+1,y));
    %    corrmat(x,y)=mean(mean(corr-eye(9)));
    end
end
%figure
%imagesc(corrmat)


corrmat2=[];
testy=25;
testx=25;
for x=1:45
    for y=1:45
        count=40;
        corrs=(xcorr(vret(:,x,y),vret(:,testx,testy),count,'coeff'));
        c=corrs((count+2):end)' .* (exp(-(1:count)/20)-exp(-(1:count)/5)) ;
        corrmat2(x,y)=sum(c);
    end
end
figure
imagesc(corrmat2)
title('correlation with neuron near middle')
