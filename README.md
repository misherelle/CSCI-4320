# CSCI-4320

Parallel Programming and Computing coursework plus the group project serial core.

## Pokemon Battle Territory Serial Core

Person 1 scope is implemented in:

- `pokemon_battle_core.h`
- `pokemon_battle_core.c`
- `pokemon_battle_demo.c`
- `pokemon_battle_tests.c`

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

The serial core files provide the 18 Pokemon type IDs, the 18 x 18
type-effectiveness matrix, local neighbor attack/support update rules, rare
evolution-style pattern events, and small correctness tests. Those files do not
include MPI, CUDA, or scaling experiments.

## MPI Row-Decomposition Starter

Person 2 starter code is implemented in:

- `pokemon_battle_mpi.c`

Build:

```sh
make pokemon_battle_mpi
```

Run locally or on AiMOS through an MPI launch command:

```sh
mpirun -np 4 ./pokemon_battle_mpi 256 256 100
mpirun -np 4 ./pokemon_battle_mpi 256 256 100 4320
```

This version splits the 2D grid by rows, exchanges one top and one bottom ghost
row each iteration with `MPI_Sendrecv`, times communication separately from
computation, gathers the final grid on rank 0, and checks it against the serial
core for correctness.

An AiMOS SLURM starter script is included:

```sh
sbatch run_pokemon_mpi_aimos.slurm
```

Before submitting, uncomment or adjust the `module load` lines in the script to
match the MPI/compiler modules provided in the course environment.

## Rubric-Facing Report and Experiments

Report draft:

```sh
make report
```

Strong scaling on AiMOS:

```sh
sbatch run_strong_scaling_aimos.slurm
```

Weak scaling on AiMOS:

```sh
sbatch run_weak_scaling_aimos.slurm
```

After the CSV files exist in `results/`, generate plots:

```sh
python3 plot_scaling.py
```

The report template expects:

- `plots/strong_scaling.png`
- `plots/weak_scaling.png`

If those plots do not exist yet, the PDF will show placeholders instead.
