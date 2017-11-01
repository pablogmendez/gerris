function [t,sensors,act]=getfromlogfile(logfile)
    if nargin<1
        logfile='cylinder/actuation.csv';
    end
    testfile='test.txt';
    logup='log_actUp.txt';
    logdown='log_actDown.txt';
    cur_t=[];
    cur_sensors=[];
    cur_actup=[];
    cur_actdown=[];
    
        
        

        
        
    
    
    system(sprintf('sed -n ''s/.*actuationUp://p''  < %s > %s',logfile,testfile));
    system(sprintf('sed ''/[a-z]/d'' < %s > %s',testfile,logup));
    delete(testfile);
    

    system(sprintf('sed -n ''s/.*actuationDown://p''  < %s > %s',logfile,testfile));
    system(sprintf('sed ''/[a-z]/d'' < %s > %s',testfile,logdown));
    delete(testfile);
    
    try
    A_=importdata(logup);
   
    sim_step=A_(:,1);
    [~,idx]=unique(sim_step);
    A=A_(idx,:);
    actup=A(:,3);
    tup=A(:,2);
    if length(actup)>length(cur_actup)
        cur_actup=actup;
        cur_tup=tup;
    end
    
    B_=importdata(logdown);
    sim_step=B_(:,1);
    [~,idx]=unique(sim_step);
    B=B_(idx,:);
    t=B(:,2);
    actdown=B(:,3);
    sensors=B(:,4:end);
    
    if length(t)>length(cur_t)
        cur_t=t;
        cur_actdown=actdown;
        cur_sensors=sensors;
    end
    catch
    end
    
    [t,idx1,idx2]=intersect(cur_t,cur_tup);
    cur_actup=cur_actup(idx2);
    cur_actdown=cur_actdown(idx1);
    
    act=[cur_actup cur_actdown];
   
    
    sensors=cur_sensors(idx1,:);
    delete(logup);
    delete(logdown);
    
    
    
