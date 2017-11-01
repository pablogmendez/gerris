function [t,x,y,s,b,dJa,dJb]=xGetResults(idv,parameters,WorkerID)
    if nargin <3
        WorkerID=[];
    end
   
    curdir=pwd;
    workingdir=sprintf('%s%d',parameters.problem_variables.SimDirectory,WorkerID);
    
    cd(workingdir);
    
    %% Get sensor values.
    logfile=fullfile('results','sensors.txt');
    A=importdata(logfile);
    n=size(A.data,1);
    t=reshape(A.data(:,1),8,n/8)';
    t=t(:,1);
    
    s=reshape(A.data(:,9),8,n/8)';
    x=A.data(1:8,2);
    y=A.data(1:8,3);
    
    
    
    cd(curdir)
    dJa=sum(s,2);
    
    ControlLaw=idv.formal;
    for i=1:8
        ControlLaw=strrep(ControlLaw,sprintf('S%d',i-1),sprintf('s(:,%d)',i));
    end
    b=0;
    eval(sprintf('b=%s;',ControlLaw));
    b=b+s(:,1)*0;
    b(b<0)=0;
    b(b>parameters.problem_variables.maxact)=parameters.problem_variables.maxact;
    
    dJb=(b/parameters.problem_variables.maxact).^2;
    
  
    
    
    
