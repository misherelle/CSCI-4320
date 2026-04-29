#!/bin/bash
#SBATCH --job-name=pokemon-strong
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=32
#SBATCH --time=00:20:00
#SBATCH --output=pokemon_strong_%j.out
#SBATCH --error=pokemon_strong_%j.err

# Adjust these module names to match the course AiMOS environment.
# module load gcc
# module load spectrum-mpi

set -euo pipefail

timestamp() {
    date +"%Y-%m-%d %H:%M:%S"
}

ROWS=${ROWS:-1024}
COLS=${COLS:-1024}
STEPS=${STEPS:-100}
SEED=${SEED:-4320}
RANKS_LIST=${RANKS_LIST:-"1 2 4 8 16 32"}
OUT=${OUT:-results/strong_scaling.csv}
MPIRUN_TIMEOUT=${MPIRUN_TIMEOUT:-600}

echo "[$(timestamp)] Starting strong scaling job ${SLURM_JOB_ID:-local}"
echo "[$(timestamp)] Parameters: ROWS=${ROWS} COLS=${COLS} STEPS=${STEPS} SEED=${SEED}"
echo "[$(timestamp)] RANKS_LIST=${RANKS_LIST}"
echo "[$(timestamp)] MPIRUN_TIMEOUT=${MPIRUN_TIMEOUT}s"

mkdir -p results
echo "[$(timestamp)] Building pokemon_battle_mpi"
make pokemon_battle_mpi
echo "[$(timestamp)] Build complete"

echo "ranks,rows,cols,steps,total_seconds,comm_seconds,compute_seconds,serial_check" > "${OUT}"
echo "[$(timestamp)] Initialized CSV at ${OUT}"

for ranks in ${RANKS_LIST}; do
    run_start=$(date +%s)
    echo "[$(timestamp)] >>> START ranks=${ranks}, grid=${ROWS}x${COLS}, steps=${STEPS}"

    (
        while true; do
            sleep 30
            echo "[$(timestamp)] ... still running ranks=${ranks}" >&2
        done
    ) &
    heartbeat_pid=$!

    set +e
    if command -v timeout >/dev/null 2>&1; then
        timeout "${MPIRUN_TIMEOUT}" \
            mpirun -np "${ranks}" ./pokemon_battle_mpi "${ROWS}" "${COLS}" "${STEPS}" "${SEED}" \
            | tee "results/strong_${ranks}.log" \
            | awk -F, '/^CSV/ {print $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9}' >> "${OUT}"
        run_status=$?
    else
        mpirun -np "${ranks}" ./pokemon_battle_mpi "${ROWS}" "${COLS}" "${STEPS}" "${SEED}" \
            | tee "results/strong_${ranks}.log" \
            | awk -F, '/^CSV/ {print $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9}' >> "${OUT}"
        run_status=$?
    fi
    set -e

    kill "${heartbeat_pid}" 2>/dev/null || true

    if [ "${run_status}" -ne 0 ]; then
        if [ "${run_status}" -eq 124 ]; then
            echo "[$(timestamp)] ERROR: ranks=${ranks} timed out after ${MPIRUN_TIMEOUT}s"
        else
            echo "[$(timestamp)] ERROR: ranks=${ranks} failed with exit code ${run_status}"
        fi
        exit "${run_status}"
    fi

    run_end=$(date +%s)
    echo "[$(timestamp)] <<< END ranks=${ranks} (elapsed=$((run_end - run_start))s)"
done

echo "[$(timestamp)] Wrote ${OUT}"
