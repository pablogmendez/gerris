clear all
close all

mlc=MLC('MLC_gerris_cylinder_script')
mlc.insert_individual('(- S0 S8)')
mlc.generate_population
mlc.go(2)