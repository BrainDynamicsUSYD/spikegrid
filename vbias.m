center=105;
range=20;
trialnum=5;
fname='output/3.txt';
data=importdata(fname);
block(:,:) = data((trialnum*200+1):(trialnum*200+100),(center-range):(center+range));
imagesc(center-range:center+range,1:100,block) %plot with rescaled axis
colorbar
