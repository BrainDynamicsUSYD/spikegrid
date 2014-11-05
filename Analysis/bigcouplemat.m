gridsize = 40;
couplerange = 15;
wplus = 1.0;
outmat=zeros(gridsize*gridsize);
for i=0:(gridsize-1)
    disp(i)
    for j=0:(gridsize-1)
        for off1=-couplerange:couplerange
            for off2=-couplerange:couplerange
                rsq=off1*off1+off2*off2;
                if rsq < couplerange*couplerange
                    ni=wrap(i+off1,gridsize);
                    nj=wrap(j+off2,gridsize);
                    if (mod(i,2)==0 && mod(j,2)==0)
                        outmat(i*gridsize+j+1,ni*gridsize+nj+1)=-0.39*exp(-rsq/90);
                    else
                        outmat(i*gridsize+j+1,ni*gridsize+nj+1)=0.2*exp(-rsq/12);
                    end
                end
            end
        end
    end
end
figure(1)
imagesc(outmat)
figure(2)
semilogx(-sort(-abs(eig(outmat))))



