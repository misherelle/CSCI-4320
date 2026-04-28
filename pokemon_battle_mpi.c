/*
  CSCI-4320 Group Project - Pokemon Battle Territory MPI Layer

  Person 2 starter implementation:
  - row-based 2D grid decomposition
  - top/bottom ghost-row halo exchange
  - communication and computation timing
  - correctness check against the serial core on rank 0 for small/medium runs
*/

#include "pokemon_battle_core.h"

#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long ticks;

#define CLOCK_RATE_HZ 512000000.0

static inline ticks aimos_clock_read(void)
{
#if defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
    unsigned int tbl, tbu0, tbu1;
    do {
        __asm__ __volatile__("mftbu %0" : "=r"(tbu0));
        __asm__ __volatile__("mftb %0"  : "=r"(tbl));
        __asm__ __volatile__("mftbu %0" : "=r"(tbu1));
    } while (tbu0 != tbu1);
    return (((unsigned long long)tbu0) << 32) | tbl;
#else
    return (ticks)(MPI_Wtime() * CLOCK_RATE_HZ);
#endif
}

static inline double ticks_to_seconds(ticks dt)
{
    return (double)dt / CLOCK_RATE_HZ;
}

static void mpi_fail(int rc, const char *what, int rank)
{
    if (rc != MPI_SUCCESS) {
        char err[MPI_MAX_ERROR_STRING];
        int len = 0;
        MPI_Error_string(rc, err, &len);
        fprintf(stderr, "Rank %d: MPI error during %s: %s\n", rank, what, err);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
}

static void *checked_malloc(size_t bytes, const char *label, int rank)
{
    void *ptr = malloc(bytes);
    if (ptr == NULL) {
        fprintf(stderr, "Rank %d: malloc failed for %s (%zu bytes)\n", rank, label, bytes);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    return ptr;
}

static void usage_and_exit(const char *prog, int rank)
{
    if (rank == 0) {
        fprintf(stderr,
                "Usage:\n"
                "  %s <rows> <cols> <steps> [random_seed]\n\n"
                "Examples:\n"
                "  mpirun -np 4 %s 256 256 100\n"
                "  mpirun -np 4 %s 256 256 100 4320\n",
                prog, prog, prog);
    }
    MPI_Finalize();
    exit(EXIT_FAILURE);
}

static void compute_row_counts(size_t rows,
                               size_t cols,
                               int nprocs,
                               int *sendcounts,
                               int *displs,
                               size_t *local_rows_out,
                               int rank)
{
    size_t offset = 0;

    for (int r = 0; r < nprocs; r++) {
        size_t base = rows / (size_t)nprocs;
        size_t extra = ((size_t)r < rows % (size_t)nprocs) ? 1 : 0;
        size_t local_rows = base + extra;
        size_t count = local_rows * cols;

        if (count > (size_t)INT32_MAX || offset > (size_t)INT32_MAX) {
            if (rank == 0) {
                fprintf(stderr, "Grid is too large for MPI_Scatterv/Gatherv int counts.\n");
            }
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        sendcounts[r] = (int)count;
        displs[r] = (int)offset;
        offset += count;

        if (r == rank) {
            *local_rows_out = local_rows;
        }
    }
}

static void post_halo_exchange(const PokemonGrid *local_with_halo,
                               size_t local_rows,
                               int rank,
                               int nprocs,
                               MPI_Request reqs[4],
                               int *req_count)
{
    const int up = rank - 1;
    const int down = rank + 1;
    int count = 0;
    int rc;

    if (rank > 0) {
        rc = MPI_Irecv((void *)local_with_halo->cells,
                       (int)local_with_halo->cols,
                       MPI_UNSIGNED_CHAR,
                       up,
                       200,
                       MPI_COMM_WORLD,
                       &reqs[count++]);
        mpi_fail(rc, "MPI_Irecv upper halo", rank);

        rc = MPI_Isend((void *)(local_with_halo->cells + local_with_halo->cols),
                       (int)local_with_halo->cols,
                       MPI_UNSIGNED_CHAR,
                       up,
                       100,
                       MPI_COMM_WORLD,
                       &reqs[count++]);
        mpi_fail(rc, "MPI_Isend upper interior row", rank);
    }

    if (rank < nprocs - 1) {
        rc = MPI_Irecv((void *)(local_with_halo->cells + (local_rows + 1) * local_with_halo->cols),
                       (int)local_with_halo->cols,
                       MPI_UNSIGNED_CHAR,
                       down,
                       100,
                       MPI_COMM_WORLD,
                       &reqs[count++]);
        mpi_fail(rc, "MPI_Irecv lower halo", rank);

        rc = MPI_Isend((void *)(local_with_halo->cells + local_rows * local_with_halo->cols),
                       (int)local_with_halo->cols,
                       MPI_UNSIGNED_CHAR,
                       down,
                       200,
                       MPI_COMM_WORLD,
                       &reqs[count++]);
        mpi_fail(rc, "MPI_Isend lower interior row", rank);
    }

    *req_count = count;
}

static void update_row_range(const PokemonGrid *current,
                             PokemonGrid *next,
                             size_t row_start,
                             size_t row_end,
                             size_t min_row,
                             size_t max_row)
{
    if (row_start > row_end) {
        return;
    }

    for (size_t r = row_start; r <= row_end; r++) {
        for (size_t c = 0; c < current->cols; c++) {
            pokemon_grid_set(next, r, c,
                             pokemon_next_cell_bounded(current, r, c, min_row, max_row));
        }
    }
}

static void update_local_rows_with_overlap(const PokemonGrid *current,
                                           PokemonGrid *next,
                                           size_t local_rows,
                                           int rank,
                                           int nprocs,
                                           double *comm_seconds,
                                           double *compute_seconds)
{
    const size_t min_row = (rank == 0) ? 1 : 0;
    const size_t max_row = (rank == nprocs - 1) ? local_rows + 1 : local_rows + 2;
    MPI_Request reqs[4];
    int req_count = 0;

    ticks comm_start = aimos_clock_read();
    post_halo_exchange(current, local_rows, rank, nprocs, reqs, &req_count);
    *comm_seconds += ticks_to_seconds(aimos_clock_read() - comm_start);

    ticks compute_start = aimos_clock_read();
    if (local_rows > 2) {
        update_row_range(current, next, 2, local_rows - 1, min_row, max_row);
    }
    *compute_seconds += ticks_to_seconds(aimos_clock_read() - compute_start);

    comm_start = aimos_clock_read();
    if (req_count > 0) {
        int rc = MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);
        mpi_fail(rc, "MPI_Waitall halo exchange", rank);
    }
    *comm_seconds += ticks_to_seconds(aimos_clock_read() - comm_start);

    compute_start = aimos_clock_read();
    if (local_rows >= 1) {
        update_row_range(current, next, 1, 1, min_row, max_row);
        if (local_rows > 1) {
            update_row_range(current, next, local_rows, local_rows, min_row, max_row);
        }
    }
    *compute_seconds += ticks_to_seconds(aimos_clock_read() - compute_start);
}

static int compare_grids(const PokemonGrid *a, const PokemonGrid *b)
{
    if (a->rows != b->rows || a->cols != b->cols) {
        return 0;
    }

    for (size_t i = 0; i < a->rows * a->cols; i++) {
        if (a->cells[i] != b->cells[i]) {
            return 0;
        }
    }

    return 1;
}

int main(int argc, char **argv)
{
    int rank, nprocs;
    int rc;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 4 && argc != 5) {
        usage_and_exit(argv[0], rank);
    }

    size_t rows = (size_t)strtoull(argv[1], NULL, 10);
    size_t cols = (size_t)strtoull(argv[2], NULL, 10);
    int steps = atoi(argv[3]);
    int use_random = (argc == 5);
    unsigned int seed = use_random ? (unsigned int)strtoul(argv[4], NULL, 10) : 4320U;

    if (rows == 0 || cols == 0 || steps < 0 || rows < (size_t)nprocs) {
        if (rank == 0) {
            fprintf(stderr, "Error: require rows > 0, cols > 0, steps >= 0, and rows >= MPI ranks.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (cols > (size_t)INT32_MAX) {
        if (rank == 0) {
            fprintf(stderr, "Error: cols is too large for MPI row messages.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int *sendcounts = (int *)checked_malloc((size_t)nprocs * sizeof(int), "sendcounts", rank);
    int *displs = (int *)checked_malloc((size_t)nprocs * sizeof(int), "displs", rank);
    size_t local_rows = 0;
    compute_row_counts(rows, cols, nprocs, sendcounts, displs, &local_rows, rank);

    PokemonGrid global = {0, 0, NULL};
    PokemonGrid serial = {0, 0, NULL};
    PokemonGrid serial_scratch = {0, 0, NULL};

    if (rank == 0) {
        global = pokemon_grid_create(rows, cols);
        serial = pokemon_grid_create(rows, cols);
        serial_scratch = pokemon_grid_create(rows, cols);

        if (use_random) {
            pokemon_grid_seed_random(&global, seed);
        } else {
            pokemon_grid_seed_pattern(&global);
        }

        memcpy(serial.cells, global.cells, rows * cols * sizeof(uint8_t));
    }

    PokemonGrid current = pokemon_grid_create(local_rows + 2, cols);
    PokemonGrid next = pokemon_grid_create(local_rows + 2, cols);

    rc = MPI_Scatterv(rank == 0 ? global.cells : NULL,
                      sendcounts,
                      displs,
                      MPI_UNSIGNED_CHAR,
                      current.cells + cols,
                      sendcounts[rank],
                      MPI_UNSIGNED_CHAR,
                      0,
                      MPI_COMM_WORLD);
    mpi_fail(rc, "MPI_Scatterv initial grid", rank);

    MPI_Barrier(MPI_COMM_WORLD);
    ticks total_start = aimos_clock_read();
    double comm_seconds = 0.0;
    double compute_seconds = 0.0;

    for (int step = 0; step < steps; step++) {
        update_local_rows_with_overlap(&current, &next, local_rows, rank, nprocs,
                                       &comm_seconds, &compute_seconds);

        uint8_t *tmp = current.cells;
        current.cells = next.cells;
        next.cells = tmp;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double total_local = ticks_to_seconds(aimos_clock_read() - total_start);

    rc = MPI_Gatherv(current.cells + cols,
                     sendcounts[rank],
                     MPI_UNSIGNED_CHAR,
                     rank == 0 ? global.cells : NULL,
                     sendcounts,
                     displs,
                     MPI_UNSIGNED_CHAR,
                     0,
                     MPI_COMM_WORLD);
    mpi_fail(rc, "MPI_Gatherv final grid", rank);

    double total_seconds = 0.0;
    double max_comm_seconds = 0.0;
    double max_compute_seconds = 0.0;
    MPI_Reduce(&total_local, &total_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&comm_seconds, &max_comm_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&compute_seconds, &max_compute_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        pokemon_run(&serial, &serial_scratch, steps, 0);
        int ok = compare_grids(&global, &serial);

        printf("Pokemon Battle Territory MPI row-decomposition run\n");
        printf("MPI ranks = %d, grid = %zu x %zu, steps = %d, seed mode = %s\n",
               nprocs, rows, cols, steps, use_random ? "random" : "deterministic pattern");
        printf("Total time      = %.6f seconds\n", total_seconds);
        printf("Communication   = %.6f seconds max per rank\n", max_comm_seconds);
        printf("Computation     = %.6f seconds max per rank\n", max_compute_seconds);
        printf("Serial check    = %s\n", ok ? "PASS" : "FAIL");
        printf("CSV,%d,%zu,%zu,%d,%.9f,%.9f,%.9f,%s\n",
               nprocs,
               rows,
               cols,
               steps,
               total_seconds,
               max_comm_seconds,
               max_compute_seconds,
               ok ? "PASS" : "FAIL");

        if (rows <= 16 && cols <= 32) {
            printf("\nFinal grid:\n");
            pokemon_grid_print(&global);
        }
    }

    pokemon_grid_free(&next);
    pokemon_grid_free(&current);
    if (rank == 0) {
        pokemon_grid_free(&serial_scratch);
        pokemon_grid_free(&serial);
        pokemon_grid_free(&global);
    }
    free(displs);
    free(sendcounts);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
