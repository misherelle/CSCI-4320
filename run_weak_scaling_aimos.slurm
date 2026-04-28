#!/bin/bash
#SBATCH --job-name=pokemon-weak
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=32
#SBATCH --time=00:20:00
#SBATCH --output=pokemon_weak_%j.out
#SBATCH --error=pokemon_weak_%j.err

# Adjust these module names to match the course AiMOS environment.
# module load gcc
# module load spectrum-mpi

set -euo pipefail

BASE_ROWS_PER_RANK=${BASE_ROWS_PER_RANK:-256}
COLS=${COLS:-1024}
STEPS=${STEPS:-100}
SEED=${SEED:-4320}
RANKS_LIST=${RANKS_LIST:-"1 2 4 8 16 32"}
OUT=${OUT:-results/weak_scaling.csv}

mkdir -p results
make pokemon_battle_mpi

echo "ranks,rows,cols,steps,total_seconds,comm_seconds,compute_seconds,serial_check" > "${OUT}"

for ranks in ${RANKS_LIST}; do
    rows=$((BASE_ROWS_PER_RANK * ranks))
    echo "Running weak scaling: ranks=${ranks}, grid=${rows}x${COLS}, steps=${STEPS}"
    mpirun -np "${ranks}" ./pokemon_battle_mpi "${rows}" "${COLS}" "${STEPS}" "${SEED}" \
        | tee "results/weak_${ranks}.log" \
        | awk -F, '/^CSV/ {print $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9}' >> "${OUT}"
done

echo "Wrote ${OUT}"
