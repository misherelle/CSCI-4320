#!/bin/bash

OUT="mpicuda_results.csv"

echo "experiment,ranks,rows,cols,steps,total_sec,comm_sec,compute_sec,check" > "$OUT"

run_test () {
    EXPERIMENT=$1
    RANKS=$2
    ROWS=$3
    COLS=$4
    STEPS=$5
    SEED=$6

    echo "Running $EXPERIMENT: ranks=$RANKS grid=${ROWS}x${COLS} steps=$STEPS"

    mpirun -np "$RANKS" ../pokemon_battle_mpi_cuda "$ROWS" "$COLS" "$STEPS" "$SEED" \
        | grep "^CSV" \
        | awk -v experiment="$EXPERIMENT" -F, \
        '{print experiment "," $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9}' \
        >> "$OUT"
}

# correctness / small tests
run_test small_correctness 1 12 24 5 4320
run_test small_correctness 2 12 24 5 4320
run_test small_correctness 4 12 24 5 4320

# strong scaling: fixed grid size, increasing ranks
run_test strong_1024 1 1024 1024 100 4320
run_test strong_1024 2 1024 1024 100 4320
run_test strong_1024 4 1024 1024 100 4320

run_test strong_2048 1 2048 2048 100 4320
run_test strong_2048 2 2048 2048 100 4320
run_test strong_2048 4 2048 2048 100 4320

# weak scaling: increase problem size with rank count
run_test weak_512_per_rank 1 512 512 100 4320
run_test weak_512_per_rank 2 1024 512 100 4320
run_test weak_512_per_rank 4 2048 512 100 4320

run_test weak_1024_per_rank 1 1024 1024 100 4320
run_test weak_1024_per_rank 2 2048 1024 100 4320
run_test weak_1024_per_rank 4 4096 1024 100 4320

# step scaling: same grid/ranks, more timesteps
run_test steps_2048_r4 4 2048 2048 50 4320
run_test steps_2048_r4 4 2048 2048 100 4320
run_test steps_2048_r4 4 2048 2048 200 4320

echo ""
echo "Done. Results saved to $OUT"
cat "$OUT"