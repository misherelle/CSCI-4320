# CSCI-4320

Parallel Programming Final Project
by Angela Imanuel,
Michael Lam,
Dillon Li,
Michelle Li


Run all commands below from the repository root (the directory that contains
`Makefile`).

## Layout

| Path | Purpose |
|------|---------|
| `pokemon_battle_core.{h,c}` | Serial rules and grid helpers |
| `pokemon_battle_demo.c`, `pokemon_battle_tests.c` | Demo and unit tests |
| `mpi/pokemon_battle_mpi.c` | MPI driver (row split, halos, timing) |
| `mpi/run_*_aimos.slurm` | AiMOS SLURM scripts (also runnable with `bash`) |
| `mpi + cuda/` | Hybrid MPI+CUDA sources and CUDA run scripts |
| `results/` | CSV output from scaling runs (created by scripts) |
| `plots/` | PNG figures from `plot_scaling.py` |
| `data/` | Data for the MPI only and MPI+CUDA runs |

## Serial core

Build and test:

```sh
make
make test
```

Run the demo:

```sh
./pokemon_battle_demo
./pokemon_battle_demo 20 40 10
./pokemon_battle_demo 20 40 10 --random 4320
```

## MPI build and run
Load Modules first
```sh
module load xl_r spectrum-mpi cuda
```


Build the MPI executable (writes `pokemon_battle_mpi` in the repo root):

```sh
make pokemon_battle_mpi
```

Run with MPI (from repo root, after loading your cluster’s MPI modules if needed):

```sh
mpirun -np 4 ./pokemon_battle_mpi 256 256 100
mpirun -np 4 ./pokemon_battle_mpi 256 256 100 4320
```

### SLURM on AiMOS

Scripts live under `mpi/`. Submit from the repo root:

```sh
sbatch mpi/run_pokemon_mpi_aimos.slurm
sbatch mpi/run_strong_scaling_aimos.slurm
sbatch mpi/run_weak_scaling_aimos.slurm
```

### Run scaling scripts without `sbatch`

On a login node or inside an interactive allocation (`salloc`, etc.):

```sh
bash mpi/run_strong_scaling_aimos.slurm
bash mpi/run_weak_scaling_aimos.slurm
```

Optional environment variables (see script headers): `ROWS`, `COLS`, `STEPS`,
`SEED`, `RANKS_LIST`, `BASE_ROWS_PER_RANK` (weak), `OUT`, `MPIRUN_TIMEOUT`.

### One-shot: serial + weak + strong

```sh
make all-tests
```

This runs `run-serial`, then `run-weak`, then `run-strong` (each invokes `bash`
on the `mpi/*.slurm` scripts). Individual targets: `make run-serial`,
`make run-weak`, `make run-strong`.


## Copying to AiMOS from your laptop

Example:

```sh
scp -r CSCI-4320 youruser@blp01.ccni.rpi.edu:~/
```

For repeated sync, `rsync -av` is often easier.

### Windows line endings

If you see `$'\r': command not found` or `set: pipefail` errors when running
`.slurm` files on Linux, convert CRLF to LF once on AiMOS (from repo root):

```sh
sed -i 's/\r$//' mpi/*.slurm Makefile
```

The repo includes `.gitattributes` so future checkouts should keep Unix line
endings for those files when using Git with appropriate settings.

## MPI + CUDA

The MPI + CUDA implementation combines row-based MPI domain decomposition with GPU acceleration for local cell updates.

### Load Modules (AiMOS)

```sh
module load xl_r spectrum-mpi cuda
```

---

### Build

From the repository root:

```sh
mpicc -c pokemon_battle_core.c -o pokemon_battle_core.o
nvcc -c "mpi + cuda/pokemon_battle_cuda.cu" -o pokemon_battle_cuda.o
mpicxx "mpi + cuda/pokemon_battle_mpi.c" pokemon_battle_core.o pokemon_battle_cuda.o \
  -L/usr/local/cuda-11.0/lib64 -lcudart \
  -o pokemon_battle_mpi_cuda
```

---

### Run (manual)

```sh
mpirun -np 4 ./pokemon_battle_mpi_cuda 256 256 100
mpirun -np 4 ./pokemon_battle_mpi_cuda 256 256 100 4320
```

---

### Run scaling tests

A helper script runs correctness, strong scaling, weak scaling, and timestep scaling:

```sh
cd "mpi + cuda"
chmod +x run_cuda_scaling.sh
./run_cuda_scaling.sh
```

Results are saved to:

```sh
mpicuda_results.csv
```

---

### SLURM on AiMOS (recommended)

Example batch script:

```sh
#!/bin/bash
#SBATCH -J pokemon_cuda
#SBATCH -N 1
#SBATCH -n 4
#SBATCH --gres=gpu:4
#SBATCH -t 00:30:00
#SBATCH --partition=dcs-2024
#SBATCH -o pokemon_cuda_%j.out
#SBATCH -e pokemon_cuda_%j.err

cd ~/scratch/pokemon_project
./run_cuda_scaling.sh
```

Submit:

```sh
sbatch run_cuda_scaling.sbatch
```

Check job:

```sh
squeue -u $USER
```

Cancel job:

```sh
scancel <jobid>
```

---

### Output

Each run prints:

CSV,ranks,rows,cols,steps,total_sec,comm_sec,compute_sec,check

All results are aggregated into:

```sh
mpicuda_results.csv
```

---

### Notes

- Requires GPU-enabled nodes on AiMOS
- All runs validated with serial correctness check (PASS)
- Scaling experiments were performed up to 4 ranks to match available GPU resources

## Clean

```sh
make clean
```

Removes serial/MPI binaries and LaTeX artifacts for `group-project-report.tex`.
