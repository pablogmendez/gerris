function xSetFinalTime(parameters,verbose,WorkerID)
if nargin <2
    verbose=0;
end
if nargin <3
    WorkerID=[];
end
GfsFile=fullfile(sprintf('%s%d',parameters.SimDirectory,WorkerID),parameters.GfsFile);

if exist(GfsFile,'file')    
    delete(GfsFile);
    while exist(GfsFile,'file')
        pause(0.1);
        fprintf('Waiting for GFS file to disappear.');
    end
    if verbose
        fprintf('Old simulation file removed.\n');
    end
end

    system(sprintf(['sed -e ''s:  GfsTime { i =.*:  GfsTime { i = 3306 t = 60'...
    ' end = %.2f dtmax = 0.025 }:'' < %s > %s'],...
    parameters.total_time,...
    sprintf('%s.unchanged',GfsFile),...
    GfsFile...
    ));
while ~exist(GfsFile,'file')
        pause(0.1);
        fprintf('Waiting for GFS file to appear.');
end
pause(0.1);
if verbose
    fprintf('%s generated with %.2f final time.\n',...
        parameters.GfsFile,parameters.total_time);
end    
end


