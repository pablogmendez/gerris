function xSetActuator(idv,parameters,WorkerID)
if nargin <3
    WorkerID=[];
end
ControllerFile=fullfile(sprintf('%s%d',parameters.problem_variables.SimDirectory,WorkerID),'python','user','controller.py');
if exist(ControllerFile,'file')
    delete(ControllerFile)
    while exist(ControllerFile,'file')
        fprintf('Waiting for controller file to disappear.\n');
        pause(0.1);
    end
end

fid=fopen(ControllerFile,'w');
fprintf(fid,'from math import *\n');
fprintf(fid,'import logging\n');
fprintf(fid,'import ConfigParser\n');
fprintf(fid,'import numpy as np\n');
fprintf(fid,'import math\n');
fprintf(fid,'import csv\n');
fprintf(fid,'import os, os.path\n');
fprintf(fid,'import time\n');
fprintf(fid,'\n');



fprintf(fid,'def init(proc_index):\n');
fprintf(fid,'    pass\n');
fprintf(fid,'\n');

fprintf(fid,'def destroy(proc_index):\n');
fprintf(fid,'    pass\n');
fprintf(fid,'\n');

fprintf(fid,'def actuation(time, step, samples):\n');



fprintf(fid,'    act = 0\n');
fprintf(fid,'    completed_time = samples.completedTime\n');
fprintf(fid,'    if completed_time:\n');
fprintf(fid,'        S = samples.search().byTime(completed_time).byVariable(''T'').asValues()\n');
for i=1:8
fprintf(fid,'        S%d = S[%d]\n',i-1,i-1);
end
fprintf(fid,'        act = %s\n',strrep(idv.formal,'.*','*'));
fprintf(fid,'        if act < 0:\n');
fprintf(fid,'                act=0\n');
fprintf(fid,'        if act > %f:\n',parameters.problem_variables.maxact);
fprintf(fid,'                act=%f\n',parameters.problem_variables.maxact);
fprintf(fid,'        logging.info(''** fixed actuation ** step=%%d - t=%%.3f - act=%%.2f **'' %% (step, completed_time, act))\n');
fprintf(fid,'    return act\n');
    


%%