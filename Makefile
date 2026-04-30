CC ?= gcc
MPICC ?= mpicc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

.PHONY: all test report plots run-serial run-weak run-strong all-tests clean

all: pokemon_battle_demo pokemon_battle_tests

pokemon_battle_demo: pokemon_battle_demo.c pokemon_battle_core.c pokemon_battle_core.h
	$(CC) $(CFLAGS) pokemon_battle_demo.c pokemon_battle_core.c -o pokemon_battle_demo

pokemon_battle_tests: pokemon_battle_tests.c pokemon_battle_core.c pokemon_battle_core.h
	$(CC) $(CFLAGS) pokemon_battle_tests.c pokemon_battle_core.c -o pokemon_battle_tests

pokemon_battle_mpi: mpi/pokemon_battle_mpi.c pokemon_battle_core.c pokemon_battle_core.h
	$(MPICC) $(CFLAGS) mpi/pokemon_battle_mpi.c pokemon_battle_core.c -o pokemon_battle_mpi

test: pokemon_battle_tests
	./pokemon_battle_tests

plots:
	python3 plot_scaling.py

run-serial: pokemon_battle_mpi
	@mkdir -p results
	@echo "=== Running serial baseline (1 rank) ==="
	@echo "ranks,rows,cols,steps,total_seconds,comm_seconds,compute_seconds,serial_check" > results/serial_scaling.csv
	@mpirun -np 1 ./pokemon_battle_mpi "$${ROWS:-1024}" "$${COLS:-1024}" "$${STEPS:-100}" "$${SEED:-4320}" \
		| tee results/serial_1.log \
		| awk -F, '/^CSV/ {print $$2 "," $$3 "," $$4 "," $$5 "," $$6 "," $$7 "," $$8 "," $$9}' >> results/serial_scaling.csv
	@echo "Wrote results/serial_scaling.csv"

run-weak:
	@echo "=== Running weak scaling ==="
	@bash mpi/run_weak_scaling_aimos.slurm

run-strong:
	@echo "=== Running strong scaling ==="
	@bash mpi/run_strong_scaling_aimos.slurm

all-tests: run-serial run-weak run-strong
	@echo "=== Completed serial + weak + strong runs ==="

report: group-project-report.tex
	pdflatex group-project-report.tex
	pdflatex group-project-report.tex

clean:
	rm -f pokemon_battle_demo pokemon_battle_tests pokemon_battle_mpi group-project-report.aux group-project-report.log group-project-report.out group-project-report.pdf
