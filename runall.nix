#!/bin/bash

cd results/work_c3a1_36/

for trial_num in {148}
do
  nice -10 ../../build/main.o "basic.txt" "trial${trial_num}_LOD.txt" "trial${trial_num}_GEN.txt" "trial${trial_num}" ${trial_num} 0 2>&1 | tee $trial_num"_output.log" &
done
