# CSCI-4320

Parallel Programming Final Project
by Angela Immanuel
Michael Lam
Dillon Li
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
| `mpidata/` | Example or archived CSV data (optional) |

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

See `mpi + cuda/` for the hybrid sources and batch/shell helpers. Build steps
depend on your module stack; follow course notes for `nvcc` and MPI wrappers.

## Clean

```sh
make clean
```

Removes serial/MPI binaries and LaTeX artifacts for `group-project-report.tex`.
