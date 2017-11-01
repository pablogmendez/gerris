function ok=MLC_Gerris_cylinder_evaluator(idv,parameters)
    ok=1;
%% Remove constants by construction
    if ~strfind(idv.value,'S')
        ok=0;
        return;
    end

%% Get expected control values
    [t,~,s]=xGetUncontrolledResults();
    ControlLaw=idv.formal;
    for i=1:8
        ControlLaw=strrep(ControlLaw,sprintf('S%d',i-1),sprintf('s(:,%d)',i));
    end
    b=0;
    eval(sprintf('b=%s;',ControlLaw));
    b=b+s(:,1)*0;
    b(b<0)=0;
    b(b>parameters.problem_variables.maxact)=parameters.problem_variables.maxact;

%% Filters
%     if numel(b(b>2.5))<(0.05 * numel (t))
%         ok=0;
%     end
% 
%     if numel(find(diff(b)==0))
%         ok=0;
%     end

if std(b)<parameters.problem_variables.LowStd
    ok=0;
    idv.comment='Pre_eval_fail: STD(b) too low';
    
end
