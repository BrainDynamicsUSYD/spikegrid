function d=dist(x_,x__)
size=45;
xx = x__(1);
yy = x__(2);
x = x_(1);
y = x_(2);
dx=xx-x;
dy=yy-y;
if (dx>size/2)
    dx=dx-size;
elseif (dx<-size/2)
    dx=dx+size;
end
if (dy>size/2)
    dy=dy-size;
elseif (dy<-size/2)
    dy=dy+size;
end
d=sqrt(dx*dx+dy*dy);
