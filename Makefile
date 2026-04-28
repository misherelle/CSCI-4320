CC ?= gcc
MPICC ?= mpicc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

.PHONY: all test report plots clean

all: pokemon_battle_demo pokemon_battle_tests

pokemon_battle_demo: pokemon_battle_demo.c pokemon_battle_core.c pokemon_battle_core.h
	$(CC) $(CFLAGS) pokemon_battle_demo.c pokemon_battle_core.c -o pokemon_battle_demo

pokemon_battle_tests: pokemon_battle_tests.c pokemon_battle_core.c pokemon_battle_core.h
	$(CC) $(CFLAGS) pokemon_battle_tests.c pokemon_battle_core.c -o pokemon_battle_tests

pokemon_battle_mpi: pokemon_battle_mpi.c pokemon_battle_core.c pokemon_battle_core.h
	$(MPICC) $(CFLAGS) pokemon_battle_mpi.c pokemon_battle_core.c -o pokemon_battle_mpi

test: pokemon_battle_tests
	./pokemon_battle_tests

plots:
	python3 plot_scaling.py

report: report.tex
	pdflatex report.tex
	pdflatex report.tex

clean:
	rm -f pokemon_battle_demo pokemon_battle_tests pokemon_battle_mpi report.aux report.log report.out report.pdf
