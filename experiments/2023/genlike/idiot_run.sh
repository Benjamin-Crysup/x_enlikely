./builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error 0.00144 --flat-efficiency --efficiency 0.9 --load 2 --allele 4 --ploidies 2/2 --maxreact 1000000 --cycles 0 --seed 1234 --fan 32 --gpu 0 --chunk 2097152 --out waiting/eval2_naive.load
./builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error 0.00144 --flat-efficiency --efficiency 0.9 --load 2 --allele 4 --ploidies 2/2 --maxreact 1000000 --cycles 20 --seed 1234 --fan 32 --gpu 0 --chunk 2097152 --out waiting/eval2_sim.load
./builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error 0.00144 --flat-efficiency --efficiency 0.9 --load 2 --allele 4 --ploidies 2/2 --maxreact 1000000 --cycles 20 --seed 5678 --fan 64 --gpu 0 --chunk 2097152 --out waiting/eval2_draw.load
./builds/bin_x64_linux/experiments/2023/exp_genlike drawread --flat-error --error 0.00001 --rcb 4 --gpu 0 --chunk 8192 --seed 8675309 --drawL waiting/eval2_draw.load --compL waiting/eval2_naive.load --out waiting/eval2_naive_comp.pros --mapout waiting/eval2_naive_comp.glmap
./builds/bin_x64_linux/experiments/2023/exp_genlike drawread --flat-error --error 0.00001 --rcb 4 --gpu 0 --chunk 8192 --seed 8675309 --drawL waiting/eval2_draw.load --compL waiting/eval2_sim.load --out waiting/eval2_sim_comp.pros --mapout waiting/eval2_sim_comp.glmap
./builds/bin_x64_linux/experiments/2023/exp_genlike prototsv --in waiting/eval2_naive_comp.pros --map waiting/eval2_naive_comp.glmap --out waiting/eval2_naive.tsv.gz
./builds/bin_x64_linux/experiments/2023/exp_genlike prototsv --in waiting/eval2_sim_comp.pros --map waiting/eval2_sim_comp.glmap --out waiting/eval2_sim.tsv.gz

./builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error 0.00144 --flat-efficiency --efficiency 0.9 --load 4 --allele 4 --ploidies 2/2 --maxreact 1000000 --cycles 0 --seed 1234 --fan 32 --gpu 0 --chunk 2097152 --out waiting/eval4_naive.load
./builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error 0.00144 --flat-efficiency --efficiency 0.9 --load 4 --allele 4 --ploidies 2/2 --maxreact 1000000 --cycles 20 --seed 1234 --fan 32 --gpu 0 --chunk 2097152 --out waiting/eval4_sim.load
./builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error 0.00144 --flat-efficiency --efficiency 0.9 --load 4 --allele 4 --ploidies 2/2 --maxreact 1000000 --cycles 20 --seed 5678 --fan 64 --gpu 0 --chunk 2097152 --out waiting/eval4_draw.load
./builds/bin_x64_linux/experiments/2023/exp_genlike drawread --flat-error --error 0.00001 --rcb 10 --gpu 0 --chunk 64 --seed 8675309 --drawL waiting/eval4_draw.load --compL waiting/eval4_naive.load --out waiting/eval4_naive_comp.pros --mapout waiting/eval4_naive_comp.glmap
./builds/bin_x64_linux/experiments/2023/exp_genlike drawread --flat-error --error 0.00001 --rcb 10 --gpu 0 --chunk 64 --seed 8675309 --drawL waiting/eval4_draw.load --compL waiting/eval4_sim.load --out waiting/eval4_sim_comp.pros --mapout waiting/eval4_sim_comp.glmap
./builds/bin_x64_linux/experiments/2023/exp_genlike prototsv --in waiting/eval4_naive_comp.pros --map waiting/eval4_naive_comp.glmap --out waiting/eval4_naive.tsv.gz
./builds/bin_x64_linux/experiments/2023/exp_genlike prototsv --in waiting/eval4_sim_comp.pros --map waiting/eval4_sim_comp.glmap --out waiting/eval4_sim.tsv.gz



#!/bin/bash

NUMCYC=20
ERRORV=0.0014400
ERRORI=0
EFFIC=0.9
MAXREACT=1000000
EVALFAN=512
TESTFAN=2048
SIMCHUNK=2097152
SEEDVAL=689814
READERR=0.00001
PROBCHUNK=8192

for InitLoad in 2 4 8 16 32
do
	echo Loading $InitLoad
	
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error $ERRORV --flat-efficiency --efficiency $EFFIC --load $InitLoad --allele 7 --ploidies 2/2 --maxreact $MAXREACT --cycles 0 --seed $SEEDVAL --fan $EVALFAN --gpu 0 --chunk $SIMCHUNK --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.load
	SEEDVAL=$(expr $SEEDVAL + 1)
	
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error $ERRORV --flat-efficiency --efficiency $EFFIC --load $InitLoad --allele 7 --ploidies 2/2 --maxreact $MAXREACT --cycles $NUMCYC --seed $SEEDVAL --fan $EVALFAN --gpu 0 --chunk $SIMCHUNK --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.load
	SEEDVAL=$(expr $SEEDVAL + 1)
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike sampload --flat-error --error $ERRORV --flat-efficiency --efficiency $EFFIC --load $InitLoad --allele 7 --ploidies 2/2 --maxreact $MAXREACT --cycles $NUMCYC --seed $SEEDVAL --fan $TESTFAN --gpu 0 --chunk $SIMCHUNK --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_draw.load
	SEEDVAL=$(expr $SEEDVAL + 1)
	
	# SAME SEED for each condition
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike drawread --flat-error --error $READERR --rcb 10 --gpu 0 --chunk $PROBCHUNK --seed $SEEDVAL --drawL snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_draw.load --compL snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.load   --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.pros   --mapout snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.glmap
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike drawread --flat-error --error $READERR --rcb 10 --gpu 0 --chunk $PROBCHUNK --seed $SEEDVAL --drawL snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_draw.load --compL snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.load --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.pros --mapout snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.glmap
	SEEDVAL=$(expr $SEEDVAL + 1)
	
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike prototsv --in snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.pros   --map snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.glmap   --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.tsv.gz
	../software/builds/bin_x64_linux/experiments/2023/exp_genlike prototsv --in snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.pros --map snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.glmap --out snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.tsv.gz
	
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.load
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.load
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_draw.load
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.pros
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_sim.glmap
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.pros
	gzip snp_mix_dip/err"$ERRORI"_cyc"$NUMCYC"_load"$InitLoad"_eval_naive.glmap
done


