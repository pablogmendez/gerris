function [t,b,ts,s]=yRetrieveActuationFromLog(parameters,WorkerID)
    if nargin<2
        WorkerID='';
    end
    curdir=pwd;
    cd(sprintf('%s%d',parameters.problem_variables.SimDirectory,WorkerID));
    
    system('cat log.txt | grep act= > actuation.txt')
    pause(1);
    
    A=importdata('actuation.txt');
     delete actuation.txt
     
    for i=1:length(A);
        a=A{i};
        idx=strfind(a,'**');
        idx1=strfind(a,'step=');
        idx2=strfind(a,' t=');
        idx3=strfind(a,'act=');
        if numel(idx)~=4 || numel(idx1)~=1 || numel(idx2)~=1 || numel(idx3)~=1
            step(i)=-1;
            t(i)=-1;
            act(i)=-10^36;
        else
            step(i)=str2double(a(idx1+5:idx2-3));
            t(i)=str2double(a(idx2+3:idx3-4));
            act(i)=str2double(a(idx3+4:idx(end)-2));
        end
        
    end
    [~,idx]=unique(step);
    t=t(idx);
    b=act(idx);
    b=b(t>=0);
    t=t(t>=0);
    
    logfile=fullfile('results','sensors.txt');
    A=importdata(logfile);
    n=size(A.data,1);
    ts=reshape(A.data(:,1),8,n/8)';
    ts=ts(:,1);
    
    s=reshape(A.data(:,9),8,n/8)';
   
    
    cd(curdir);
        
        
    
