function panial
timelimit=0.007;
fprintf('Panial is on\n')
while 1
    try 
    fprintf('Panial is')
    if ~exist('cylinder_control/log.txt','file')
        pause(1)
        fprintf(' waiting for log to appear\n');
        pause(1)
        continue
    end
    system('tail -n 20 cylinder_control/log.txt > last.txt');
    pause(1)
    A=importdata('last.txt');
    delete last.txt
    if isa(A,'cell')
    for i=1:20
        ok(i)=0;
        if ~isempty(strfind(A{i},'Controller - Waiting for call request'));
            ok(i)=1;
        end
    end
    
    
   ids=strfind(A{end},':');
   time_check='not checked';
   if ids(1)==14 && ids(2)==17
      log_time=A{end}(1:ids(2)+2);
      if now-datenum(log_time)>timelimit  
          savelog('timelimit');
          system('killall gerris2D');
          
          timelimit=0.007*2;
      else
          timelimit=0.007;
      end
      time_check=sprintf('%s minute ago',datestr(now-datenum(log_time),'MM'));
   end
   
   
    if sum(ok)==20
        savelog('waitingcallrequest');
        system('killall gerris2D');
    end
    end
    d=dir(fullfile('error_logs','*.txt'));
    if isempty(d)
        n_logs=0;
    else
        n_logs=numel(d);
    end
    
    fprintf(' on (last time on log: %s) - (%d panial(s) used)\n',time_check,n_logs)
    pause(1)
    catch err
        fprintf(err.message);
    end
            
end
end

function savelog(reason)
    if ~exist('error_logs','dir')
        mkdir('error_logs');
    end
    if exist('cylinder_control/log.txt','file')
        copyfile('cylinder_control/log.txt',fullfile('error_logs',sprintf('%s-log-%s.txt',datestr(now,'yyyymmdd-HHMMSS'),reason)));
        delete('cylinder_control/log.txt');
    end
end
        