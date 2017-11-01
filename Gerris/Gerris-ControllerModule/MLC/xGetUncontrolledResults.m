function [t,dJa,s]=xGetUncontrolledResults()
    
    curdir=pwd;
    workingdir='uncontrolled_logs';
    cd(workingdir);
    
    %% Get sensor values.
    logfile='sensors.txt';
    A=importdata(logfile);
    n=size(A.data,1);
    t=reshape(A.data(:,1),8,n/8)';
    t=t(:,1);
    s=reshape(A.data(:,9),8,n/8)';
    cd(curdir)
    dJa=sum(s,2);
    
    
   
