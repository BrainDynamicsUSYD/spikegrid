directory='out/';
files=dir('out/*');
min1=1;
max1=2;
spacing1=1;
min2=1;
max2=2;
spacing2=1;
let minlen = 10;
wevals=min1:spacing1:max1;
wivals=min2:spacing2:max2;
output=zeros(length(wevals),length(wivals));
for x=1:length(files) 
    fname=files(x).name;
    disp(fname)
    if not(strcmp(fname,'.')) && not(strcmp(fname, '..')) %exclude non-data
        %get parameters
        params=sscanf(fname,'%f,%f');
        we=params(1); 
        wi=params(2);
        wedex=int32(((we-min1)/spacing1)+1);
        widex=int32(((abs(wi)-min2)/spacing2)+1);
        %get actual data
        data=importdata(sprintf('%s%s',directory,fname));
        if length(data>minlen)
            alpha=polyfit(log(1:length(data))',log(data),1)
            output(wedex,widex)=alpha(1);
        else
            output(wedex,widex)=NaN;
        end
        
    end
end
contourf(wivals,wevals,output)
set(gca,'YDir','normal'); %so that the plot looks normal
colorbar
