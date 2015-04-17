files=dir('job.o*');
i=1;
for x=files'

    cmd=sprintf('tail -n 1 %s', x.name );
    [status result] = system(cmd);;
    dummy=textscan(result,'%f');
    nums(i,:)=dummy{1}';
    i=i+1;
end
[means,sems]=grpstats(nums,nums(:,1),{'mean','sem'})

plot(means(:,1),means(:,2))
hold all
plot(means(:,1),means(:,3))
plot(means(:,1),sems(:,2))
plot(means(:,1),sems(:,3))
