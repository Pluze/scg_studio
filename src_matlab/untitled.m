clear;
load("a.mat");
mmean=mean(a);
mstd=std(a);
max_corr=0;
for i=1:250
    tmp=0;
    for j=1:5000-i
        tmp=tmp+(a(j)-mmean)*(a(j+i)-mmean);

    end
    tmp=tmp/(5000-i)*mstd*mstd;
    if tmp>max_corr&&i>20
        max_corr=tmp;
        m=i;
    end
    acorrr(i)=tmp;
end
