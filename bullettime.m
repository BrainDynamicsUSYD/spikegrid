center=105;
range=20;
trialcount=30;
block=zeros(100,range*2+1,trialcount);
trialnum=190;
fname='output/4.txt';
%data=importdata(fname);
for i=1:trialcount
    acttrialnum=trialnum+i*4;
    block(:,:,i) = (data((acttrialnum*200+1):(acttrialnum*200+100),(center-range):(center+range)));
end
%block(block<-70.1)=0;
%block(block>-69.9)=0;
level=-70;
block(block<level)=1;
block(block>level)=0;
out=zeros(100,range*2+1);
for i=1:trialcount
    out = out + block(:,:,i)*i*4;
end
imagesc(center-range:center+range,1:100,out) %plot with rescaled axis
colorbar
