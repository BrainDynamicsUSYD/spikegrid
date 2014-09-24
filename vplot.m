%takes some preprocessed data and creates a plot of variances over time
resn=[];
for jn=1:40
    fname=sprintf('job-%f.mat',jn);
    load(fname);
    resn(jn,:)=res;
end
plot(resn);
