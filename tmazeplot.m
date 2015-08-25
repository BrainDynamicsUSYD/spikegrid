data=importdata('r.txt');
acc=accumarray(data(:,1),data(:,2));
plot((1:2:60),acc(1:2:60))
