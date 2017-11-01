function preev=MLC_pre_eval_cylinder_gerris_filter(ind,MLC_parameters,show)
preev=1;
alfa=MLC_parameters.problem_variables.filter_alpha;
s=MLC_parameters.problem_variables.s;
[min_s,amp_s]=create_sensor_coeffs(0,MLC_parameters);
s=(s-repmat(min_s,[size(s,1),1]))./repmat(amp_s,[size(s,1),1]);

min_act=3;

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
b2=b;
if alfa>0
    low=b(1);
    b(1)=0;
    for k=2:length(b)
        low=alfa*low+(1-alfa)*b(k);
        b(k)=b(k)-low;
    end   
end
    
b(b>MLC_parameters.problem_variables.actmax)=MLC_parameters.problem_variables.actmax;
b(b<0)=0;

if nargin>2
    plot(1:length(b),b,'b',1:length(b),b2,'--b')
end

if (sum(diff(b)==0)/length(b)) > 0.80
    preev=0;
elseif max(b(:)) < min_act
    preev=0;
end

b=s(:,1)*0;
eval(sprintf('b=%s+s(:,1)*0;',m2));
if  MLC_parameters.controls==1
    b=b* MLC_parameters.problem_variables.mode_sym;
end

b2=b;
if alfa>0
    low=b(1);
    b(1)=0;
    for k=2:length(b)
        low=alfa*low+(1-alfa)*b(k);
        b(k)=b(k)-low;
    end   
end

b(b>MLC_parameters.problem_variables.actmax)=MLC_parameters.problem_variables.actmax;
b(b<0)=0;

if nargin>2
    hold on
    plot(1:length(b),b,'r',1:length(b),b2,'--r')
    hold off
end

if (sum(diff(b)==0)/length(b)) > 0.80
    preev=0;
    elseif max(b(:)) < min_act
    preev=0;
end

if preev==0
    fprintf('rejected\n')
end



