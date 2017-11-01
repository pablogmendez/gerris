N_threads ?= 4
TControlStart ?= 60
TFinal ?= 130

.PHONY: clean init ready

ready: init results/cylinder_130.000.gfs

clean:
	@echo "Cleaning simulation directory"
	-@rm -f log*.txt error*.txt results/*

init: clean
	@echo "Initializing simulation directory"
	-@rm -rf results* vorticity* tracer* exec*
	-@rm -rf cylinder_T*.gfs* cylinder_run.gfs
	-@rm -rf python/user/controller.py python/user/controller.pyc python/user/controller.py.noact

cylinder_run.gfs: python/user/controller.py.noact
	@echo "Preparing simulation for mpirun"
	@sed -e 's:  Time { i = 0 end =.*:  Time { i = 0 end = $(TControlStart) dtmax = DT }:' < cylinder_control.gfs > temp && mv temp cylinder_control.gfs
	@cp python/user/controller.py.noact python/user/controller.py
	-@ mkdir results
	@./split_mpi.sh $(N_threads)

results/cylinder_TFinal.gfs: cylinder_TControlStart.gfs.unchanged python/user/controller.py.noact
	@echo "Running full non actuated simulation"
	@cp python/user/controller.py.noact python/user/controller.py
	sed -e 's:  GfsTime { i = \([0-9][0-9]*\) t =.*:  GfsTime { i = \1 t = $(TControlStart) end = $(TFinal) dtmax = 0.025 }:' < cylinder_TControlStart.gfs.unchanged > cylinder_TControlStart.gfs
	@./exec_from_steady_state.sh

results/cylinder_TControlStart.gfs: cylinder_run.gfs python/user/controller.py.noact
	@echo "Running non actuated simulation up to steady state"
	@cp python/user/controller.py.noact python/user/controller.py
	@./exec_from_scratch.sh
	@cd results && cp `ls -t cylinder*000.gfs | head -n 1` cylinder_TControlStart.gfs

cylinder_TControlStart.gfs.unchanged: results/cylinder_TControlStart.gfs
	@echo "Copying steady state for hot restart"
	@cp results/cylinder_TControlStart.gfs cylinder_TControlStart.gfs.unchanged

python/user/controller.py.noact:
	@echo "Preparing actuator for null control law"
	@echo "from math import *" > $@
	@echo "import logging" >> $@
	@echo "import ConfigParser" >> $@
	@echo "import numpy as np" >> $@
	@echo "import math" >> $@
	@echo "import csv" >> $@
	@echo "import os, os.path" >> $@
	@echo "import time" >> $@
	@echo "" >> $@
	@echo "def init(proc_index):" >> $@
	@echo "    pass" >> $@
	@echo "" >> $@
	@echo "def destroy(proc_index):" >> $@
	@echo "    pass" >> $@
	@echo "" >> $@
	@echo "def actuation(time, step, samples):" >> $@
	@echo "    act = 0" >> $@
	@echo "    return act" >> $@


