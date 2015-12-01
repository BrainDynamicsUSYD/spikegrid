center=105;
range=20;
block=zeros(100,range*2+1);
averagetrials=50;
trialnum=10;
poolsize=matlabpool('size'); %in some cases there can be a whole bunch of data so use parfor to read it quicker
if (poolsize < 1)
    matlabpool(10)
end
parfor t=1:averagetrials
    fname=sprintf('job-%i/2.txt',t);
    data=importdata(fname);
    block = block + (data((trialnum*300+1):(trialnum*300+100),(center-range):(center+range)))/averagetrials;
end
imagesc(center-range:center+range,1:100,block) %plot with rescaled axis
colorbar
