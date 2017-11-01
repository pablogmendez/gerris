function test_panial(MLC_parameters)

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
panial(MLC_parameters)