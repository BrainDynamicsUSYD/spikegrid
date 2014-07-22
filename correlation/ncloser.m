function n = ncloser(nth)
dists=[];
count=1;
for x = 1:45
    for y=1:45
        dists(count)=dist([x y],[25 25]);
        count=count+1;
    end
end
sorted=sort(dists);
n=sorted(nth);
