function [min_s,amp_s]=create_sensor_coeffs(fid,MLC_parameters)

s=MLC_parameters.problem_variables.s(MLC_parameters.problem_variables.t>MLC_parameters.problem_variables.control_time,:);
min_s=min(s);
max_s=max(s);
amp_s=max_s-min_s;

if fid>0
for i=1:MLC_parameters.sensors
    fprintf(fid,sprintf('   s%i = (s%i - %f)/(%f)\n',i-1,i-1,min_s(i),amp_s(i)));
end
end
end




   