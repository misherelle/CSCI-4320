#!/bin/bash
#SBATCH --job-name=pokemon-mpi
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --time=00:10:00
#SBATCH --output=pokemon_mpi_%j.out
#SBATCH --error=pokemon_mpi_%j.err

# Load the MPI/compiler environment used for the class on AiMOS before building.
# Example module names vary by semester/account, so run `module avail` if needed.
# module load gcc
# module load spectrum-mpi

set -euo pipefail

make pokemon_battle_mpi

mpirun -np "${SLURM_NTASKS}" ./pokemon_battle_mpi 256 256 100
