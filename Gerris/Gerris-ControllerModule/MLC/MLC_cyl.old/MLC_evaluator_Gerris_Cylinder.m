function J=MLC_evaluator_Gerris_Cylinder(ind,MLC_parameters,i,fig)
verb=MLC_parameters.verbose;

%% set control time start by replacing in Python script
system(sprintf(['sed -e ''s:actuationStartTime = ..*:actuationStartTime'...
    ' = %.2f:'' < %s > test.txt'],...
    MLC_parameters.problem_variables.control_time,...
    MLC_parameters.problem_variables.Python_script));

while ~exist('test.txt','file')
        pause(0.1);
        fprintf('Waiting for dummy file to appear.')
end
movefile('test.txt',MLC_parameters.problem_variables.Python_script);

%% set max act start by replacing in Python script
system(sprintf(['sed -e ''s:maxact =..*:maxact'...
    ' = %.2f:'' < %s > test.txt'],...
    MLC_parameters.problem_variables.actmax,...
    MLC_parameters.problem_variables.Python_script));

while ~exist('test.txt','file')
        pause(0.1);
        fprintf('Waiting for dummy file to appear.')
end
movefile('test.txt',MLC_parameters.problem_variables.Python_script);

%% set high pass filter start by replacing in Python script
system(sprintf(['sed -e ''s:alfa =..*:alfa'...
    ' = %.2f:'' < %s > test.txt'],...
    MLC_parameters.problem_variables.filter_alpha,...
    MLC_parameters.problem_variables.Python_script));

while ~exist('test.txt','file')
        pause(0.1);
        fprintf('Waiting for dummy file to appear.')
end
movefile('test.txt',MLC_parameters.problem_variables.Python_script);

%% set final time by replacing in Gerris script
system(sprintf(['sed -e ''s:  GfsTime { i =.*:  GfsTime { i = 2029 t = 50'...
    ' end = %.2f dtmax = 0.05 }:'' < %s > test.txt'],...
    MLC_parameters.problem_variables.total_time,...
    MLC_parameters.problem_variables.Gerris_script));
while ~exist('test.txt','file')
        pause(0.1);
        fprintf('Waiting for dummy file to appear.')
end
movefile('test.txt',MLC_parameters.problem_variables.Gerris_script);


%% Write MLC in python imported file

try 
    %cleaning previous contoller
    delete(MLC_parameters.problem_variables.Python_module);
    while exist(MLC_parameters.problem_variables.Python_module,'file')
        pause(0.1);
    end
catch
    fprintf('Could not remove previous controller. Probably already removed\n')
end




%% Actuaction
% Three modes availables. If parameters.control==1, either 'symmetric' or
% 'antisymmetric'. If parameters.control==2, each low commands a specific
% actuator. 
% 'symmetric': both actuators act when >0
% 'antisymmetric': up actuator when >0, down actuator otherwise.

if MLC_parameters.controls==1
    m=readmylisp_to_formal_MLC(ind); %% transform individual
    m=strrep(m,'S','s');
    m=strrep(m,'.*','*');
    m1=m;
    m2=m;
elseif MLC_parameters.controls==2
    m=readmylisp_to_formal_MLC(ind); %% transform individual
    m1=m{1};
    m2=m{2};
    
    m1=strrep(m1,'S','s');
    m1=strrep(m1,'.*','*');
    
    m2=strrep(m2,'S','s');
    m2=strrep(m2,'.*','*');    
end
    
if MLC_parameters.controls==2
    mode_sym=1;
else
    mode_sym=MLC_parameters.problem_variables.mode_sym;
end

if verb>3; fprintf('Creating controller python module\n');end
fid =fopen(MLC_parameters.problem_variables.Python_module,'w');
fprintf(fid,        'from numpy import *\n');
fprintf(fid,        '\n');
fprintf(fid,        'def actuationUp(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15):\n');
create_sensor_coeffs(fid,MLC_parameters);
fprintf(fid,sprintf('   act=%s\n',m2));
fprintf(fid,        '   return act\n');
fprintf(fid,        '\n');
fprintf(fid,        'def actuationDown(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15):\n');
create_sensor_coeffs(fid,MLC_parameters);
fprintf(fid,sprintf('   act=%f *( %s )\n',mode_sym,m1));
fprintf(fid,        '   return act\n');
fclose(fid);
pause(0.1);


while ~exist(MLC_parameters.problem_variables.Python_module,'file')
        pause(0.1);
        fprintf('Waiting for python module to appear.')
end

%% run simulation

done=0;
tries=0;
while ~done

tic;
cd cylinder
system('make clean');
k1= strfind(MLC_parameters.problem_variables.Gerris_script,'/');
k2= strfind(MLC_parameters.problem_variables.logfile,'/');
system(sprintf('mpirun -np 8 gerris2D  %s >%s 2>log_err.txt &',...
    MLC_parameters.problem_variables.Gerris_script(k1(end)+1:end),...
    MLC_parameters.problem_variables.outputfile));
cd ..
t=toc;

pause(5)
done=panial(MLC_parameters);
tries=tries+1;

if tries>5
    done=1;
end

end

if verb>3; fprintf('Simulation finished in %.2f seconds\n',t);end

%% Get J

[t,s,b]=getfromlogfile(MLC_parameters.problem_variables.logfile);

prefix_file=datestr(now,'YYYYmmdd-HHMMSS');
save(fullfile(MLC_parameters.savedir,[prefix_file '_data.mat']),'ind','t','s','b');
copyfile('cylinder/tracer.mpg',fullfile(MLC_parameters.savedir,sprintf('%s_tracer.mpg',prefix_file)));



subplot(4,2,1)
plot(t,s(:,1:8))
set(gca,'ylim',[0 0.5])
subplot(4,2,3)
plot(t,s(:,9:16))
set(gca,'ylim',[0 0.5],'yscale','log')
subplot(4,2,5)
plot(t,b)





s0=MLC_parameters.problem_variables.s;
t0=MLC_parameters.problem_variables.t;
[~,idx]=intersect(t0,t);
s0=s0(idx,:);



J1=1/t(end)*trapz(t,sum(s(:,1:8).^2,2).*(t>MLC_parameters.problem_variables.control_time));
J2=1/t(end)*trapz(t,sum(b.^2,2));
J0=1/t(end)*trapz(t,sum(s0(:,1:8).^2,2).*(t>MLC_parameters.problem_variables.control_time));
J=(J1+MLC_parameters.problem_variables.gamma*J2)/J0;

if t(end)<0.9*MLC_parameters.problem_variables.total_time
    J=MLC_parameters.badvalue;
end
    

subplot(4,2,7)
plot(t,cumtrapz(t,sum(s(:,1:8).^2,2).*(t>MLC_parameters.problem_variables.control_time))./cumtrapz(t,sum(s0(:,1:8).^2,2).*(t>MLC_parameters.problem_variables.control_time)),'k','linewidth',2);hold on
plot(t,cumtrapz(t,(sum(s(:,1:8).^2,2)+MLC_parameters.problem_variables.gamma*sum(b.^2,2)).*(t>MLC_parameters.problem_variables.control_time))./cumtrapz(t,sum(s0(:,1:8).^2,2).*(t>MLC_parameters.problem_variables.control_time)),'r','linewidth',1.2);hold off

%plot(t,1/t(end)*cumtrapz(t,sum(s(:,1:8).^2,2).*(t>MLC_parameters.problem_variables.control_time)),'b','linewidth',1.2);
%plot(t,1/t(end)*cumtrapz(t,MLC_parameters.problem_variables.gamma*sum(b.^2,2).*(t>MLC_parameters.problem_variables.control_time)),'r','linewidth',2);hold off
set(gca,'yscale','log')

subplot(4,2,2)
plot(t,s(:,1)-s0(:,1),'k');hold on
plot(t,cumtrapz(t,s(:,1)-s0(:,1)),'r');hold off
subplot(4,2,4)
plot(t,s(:,2)-s0(:,2),'k');hold on
plot(t,cumtrapz(t,s(:,2)-s0(:,2)),'r');hold off
subplot(4,2,6)
plot(t,s(:,3:8)-s0(:,3:8))
subplot(4,2,8)
plot(t,s(:,9:16)-s0(:,9:16))

for i=1:8
    subplot(4,2,i)
    set(gca,'xlim',[min(t) max(t)])
end



drawnow







