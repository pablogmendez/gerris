function [ok]=panial(MLC_parameters,varargin)
% PANIAL function that checks that the simulation did not shit itself and
% does what is needed if it did.

    check = 1; % the programm waits until either the sim is finished, either it is
    % frozen
    old_time=-1;
    while check
        new_time=get_time(MLC_parameters);
        if new_time==old_time % finished or stalled
            check=0;
        else
            pause(3);
        end
        if isempty(new_time)
            check=0;
        end
        old_time=new_time;
    end
    if old_time>MLC_parameters.problem_variables.total_time-1
        ok=1;
    else
        ok=0;
        killall;
    end
end

function t=get_time(MLC_parameters)
    log_file=MLC_parameters.problem_variables.outputfile;
    temp_file='time.txt';
    cd cylinder
    system(sprintf('cat %s | tail -n 1 | awk ''{print $4}'' > %s',log_file,temp_file));
    t=importdata(temp_file);
    %delete(temp_file);
    cd ..
end

function killall
temp_file='pid.txt';
sprintf('ps aux | grep gerris | awk ''{print $2}'' > %s',temp_file)
system(sprintf('ps aux | grep gerris | awk ''{print $2}'' > %s',temp_file));
pid_list=importdata(temp_file);
for i=1:length(pid_list)
    sprintf('kill -9 %i',pid_list(i))
    system(sprintf('kill -9 %i',pid_list(i)))
end

end