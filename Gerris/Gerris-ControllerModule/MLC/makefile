### MakeFile for setting up the running MLC with GerrisControl
N_threads ?= 4
TControlStart ?= 60
TFinal ?= 130

.PHONY: info clean init ready

info: 
	@echo "Makefile to prepare MLC x GerrisControl"
	@echo ""
	@echo "Target         | Effect"
	@echo "-----------------------------------------------------"
	@echo "info (or none) | This text"
	@echo "clean          | Puts MLC directory in zippable state"
	@echo "               | except for MLC results"
	@echo "init           | Initialize archives"
	@echo "ready          | Prepares MLC and simulation folders"
	@echo "               | for MLC. Option N_thread fixes the"
	@echo "               | number of processors for the sims."
	@echo "               | Ex: make ready M_threads=2 (default 4)"
	@echo ""
	@echo "Thomas Duriez 2017 (thomas.duriez@gmail.com)"


ready: init
	@echo "Preparing uncontrolled results"
	@cd cylinder_control && $(MAKE) -s results/cylinder_TFinal.gfs N_threads=$(N_threads) TFinal=$(TFinal) TControlStart=$(TControlStart)
	@mkdir uncontrolled_logs
	@cp cylinder_control/results/*.txt uncontrolled_logs/
	@cd cylinder_control &&  $(MAKE) -s clean


clean: 
	@echo "Cleaning MLC dir"
	-@rm -rf uncontrolled_logs  error_logs *.txt

init: clean
	@cd cylinder_control &&  $(MAKE) -s init


