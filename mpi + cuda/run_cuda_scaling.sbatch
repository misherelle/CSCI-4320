#!/bin/bash
#SBATCH -J pokemon_cuda
#SBATCH -t 00:30:00
#SBATCH -N 1
#SBATCH -n 4
#SBATCH --gres=gpu:4
#SBATCH --partition=dcs-2024
#SBATCH -o pokemon_cuda_%j.out
#SBATCH -e pokemon_cuda_%j.err

cd ~/scratch/pokemon_project

./run_cuda_scaling.sh