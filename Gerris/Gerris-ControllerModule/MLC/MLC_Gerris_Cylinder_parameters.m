

        parameters.size=50;
        parameters.sensors=8;
        parameters.sensor_spec=0;
        parameters.controls=1;
        parameters.sensor_prob=0.33;
        parameters.leaf_prob=0.3;
        parameters.range=100;
        parameters.precision=4;
        parameters.opsetrange=[1:3 9]; 
        parameters.formal=1;
        parameters.end_character='';
        parameters.individual_type='tree';


        %%  GP algortihm parameters (CHANGE IF YOU KNOW WHAT YOU DO)
        parameters.maxdepth=15;
        parameters.maxdepthfirst=5;
        parameters.mindepth=2;
        parameters.mutmindepth=2;
        parameters.mutmaxdepth=15;
        parameters.mutsubtreemindepth=2;
        parameters.generation_method='mixed_ramped_gauss';
        parameters.gaussigma=3;
        parameters.ramp=[2:6];
        parameters.maxtries=10;
        parameters.mutation_types=1:4;


        %%  Optimization parameters
        parameters.elitism=10;
        parameters.probrep=0.1;
        parameters.probmut=0.4;
        parameters.probcro=0.5;
        parameters.selectionmethod='tournament';
        parameters.tournamentsize=7;
        parameters.lookforduplicates=1;
        parameters.simplify=0;
        parameters.cascade=[1 1];

        %%  Evaluator parameters 
        %parameters.evaluation_method='standalone_function';
        %parameters.evaluation_method='standalone_files';
        parameters.evaluation_method='mfile_standalone';
        parameters.evaluation_function='MLC_Gerris_cylinder_evaluator';
        parameters.indfile='ind.dat';
        parameters.Jfile='J.dat';
        parameters.exchangedir=fullfile(pwd,'evaluator0');
        parameters.evaluate_all=0;
        parameters.ev_again_best=0;
        parameters.ev_again_nb=5;
        parameters.ev_again_times=5;
        parameters.artificialnoise=0;
        parameters.execute_before_evaluation='';
        parameters.badvalue=10^36;
        parameters.badvalues_elim='first';
        %parameters.badvalues_elim='none';
        %parameters.badvalues_elim='all';
        parameters.preevaluation=1;
        parameters.preev_function='MLC_Gerris_cylinder_pre_evaluator';
        parameters.problem_variables.GfsFile='cylinder_060.000.gfs';
        parameters.problem_variables.SimDirectory='/home/thomas/Documents/00_gits_reps/Gerris-ControllerModule-MLC/MLC/cylinder_control';
        parameters.problem_variables.start_time=60;
        parameters.problem_variables.total_time=130;
        parameters.problem_variables.maxact=20;
        parameters.problem_variables.gamma=0.3;
        parameters.problem_variables.LowStd=parameters.problem_variables.maxact/5;
        %% MLC behaviour parameters 
        parameters.save=1;
        parameters.saveincomplete=1;
        parameters.verbose=3;
        parameters.fgen=250; 
        parameters.show_best=1;
        parameters.savedir=fullfile(pwd,'save_GP');










