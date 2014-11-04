function r = wrap(n,size)
if (n<0) r = size+n;
elseif (n>=size) r = n-size;
else r=n;
end
