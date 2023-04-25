clear;
if ~exist("udpportObj",'var')
    udpportObj = udpport("LocalPort",3333);
end
scg=[];
ecg=[];
tstp=[];
tic;
for k=1:20000
    tmp1 = double(split(readline(udpportObj),','));
    scg=[scg;tmp1(3)];
    ecg=[ecg;tmp1(2)];
    tstp=[tstp;tmp1(1)];
end
toc;
tstp=tstp/1000000;
tstp=tstp-tstp(1);
tt=timetable(seconds(tstp),scg-mean(scg),ecg-mean(ecg));
clear("udpportObj");