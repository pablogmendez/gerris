#! /bin/bash

N_threads=${1:-4}
echo "preparing cylinder_run for" $N_threads "threads" 

case $N_threads in
1)
    N_split_domains=1
    N_split_proc=0
;;
2)
    N_split_domains=1
    N_split_proc=1
;;
4)
    N_split_domains=1
    N_split_proc=2
;;
8)
    N_split_domains=2
    N_split_proc=3
;;
16)
    N_split_domains=2
    N_split_proc=4
;;
esac


#split the domain N times so, 2 boxes become 2*N^2 boxes
gerris2D -m -s $N_split_domains cylinder_control.gfs > cylinder_control_s2.gfs

#partition the domain in N exp 2 processor groups
gerris2D -p $N_split_proc cylinder_control_s2.gfs > cylinder_control_p2.gfs

mv cylinder_control_p2.gfs cylinder_run.gfs
rm cylinder_control_s2.gfs

echo "#! /bin/bash"> exec_from_scratch.sh
echo "rm -rf results/*" >> exec_from_scratch.sh
echo "mpirun -np" $N_threads "gerris2D cylinder_run.gfs 2>log.txt" >> exec_from_scratch.sh
chmod +x exec_from_scratch.sh

echo "#! /bin/bash"> exec_from_steady_state.sh
echo "rm -rf results/*" >> exec_from_steady_state.sh
echo "mpirun -np" $N_threads "gerris2D cylinder_TControlStart.gfs 2>log.txt" >> exec_from_steady_state.sh
chmod +x exec_from_steady_state.sh

