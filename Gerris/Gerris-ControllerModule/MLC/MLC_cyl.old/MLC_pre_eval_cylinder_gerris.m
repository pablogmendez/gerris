function preev=MLC_pre_eval_cylinder_gerris(ind,MLC_parameters,show)
preev=1;

s=MLC_parameters.problem_variables.s;

if MLC_parameters.controls==1
    m=readmylisp_to_formal_MLC(ind); %% transform individual
    
    m1=m;
    m2=m;
elseif MLC_parameters.controls==2
    m=readmylisp_to_formal_MLC(ind); %% transform individual
    m1=m{1};
    m2=m{2};
    
     
end



for i=MLC_parameters.sensors:-1:1
    m1=strrep(m1,sprintf('S%d',i-1),sprintf('s(:,%d)',i));
    m2=strrep(m2,sprintf('S%d',i-1),sprintf('s(:,%d)',i));
end

b=s(:,1)*0;
eval(sprintf('b=%s+s(:,1)*0;',m1));
b(b>MLC_parameters.problem_variables.actmax)=MLC_parameters.problem_variables.actmax;
b(b<0)=0;

if nargin>2
    plot(b,'b')
end

if (sum(diff(b)==0)/length(b)) > 0.80
    preev=0;
end

b=s(:,1)*0;
eval(sprintf('b=%s+s(:,1)*0;',m2));
if  MLC_parameters.controls==1
    b=b* MLC_parameters.problem_variables.mode_sym;
end
b(b>MLC_parameters.problem_variables.actmax)=MLC_parameters.problem_variables.actmax;
b(b<0)=0;

if nargin>2
    hold on
    plot(b,'r')
    hold off
end

if (sum(diff(b)==0)/length(b)) > 0.80
    preev=0;
end

if preev==0
    fprintf('rejected\n')
end



