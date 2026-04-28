/*
  CSCI-4320: Assignment #3 - 1-D Stencil using MPI
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <mpi.h>

typedef unsigned long long ticks;

// IBM POWER9 System clock with 512MHZ resolution.
static __inline__ ticks getticks(void)
{
  unsigned int tbl, tbu0, tbu1;

  do {
    __asm__ __volatile__ ("mftbu %0" : "=r"(tbu0));
    __asm__ __volatile__ ("mftb %0" : "=r"(tbl));
    __asm__ __volatile__ ("mftbu %0" : "=r"(tbu1));
  } while (tbu0 != tbu1);

  return (((unsigned long long)tbu0) << 32) | tbl;
}

// printing usage instructions and exiting if arguments are invalid or missing
static inline void die_usage(const char *prog, int rank)
{
  if (rank == 0) {
    fprintf(stderr,
            "Usage:\n"
            "  %s NumElementsPow2 HaloSize\n\n"
            "Where:\n"
            "  NumElementsPow2 = exponent p meaning N = 2^p elements\n"
            "  HaloSize        = radius of the stencil halo (ex: 64)\n",
            prog);
  }
  MPI_Finalize();
  exit(EXIT_FAILURE);
}

// checking malloc and exiting if allocation fails
static inline void *checked_malloc(size_t bytes, const char *what, int rank)
{
  void *ptr = malloc(bytes);
  if (ptr == NULL) {
    fprintf(stderr, "Rank %d: malloc failed for %s (%zu bytes)\n", rank, what, bytes);
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }
  return ptr;
}

// clamping index into valid range [0, n-1] for serial reference
static inline size_t clamp_idx_size(long long idx, size_t n)
{
  if (idx < 0) return 0;
  if ((size_t)idx >= n) return n - 1;
  return (size_t)idx;
}

int main(int argc, char **argv)
{
  int rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // checking correct number of arguments: prog + 2 params
  if (argc != 3) die_usage(argv[0], rank);

  int numPow2 = atoi(argv[1]);
  int halo = atoi(argv[2]);

  if (numPow2 <= 0 || numPow2 > 31) {
    if (rank == 0) {
      fprintf(stderr, "Error: NumElementsPow2 should be in (0, 31]. Got %d\n", numPow2);
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  if (halo <= 0) {
    if (rank == 0) {
      fprintf(stderr, "Error: HaloSize must be > 0. Got %d\n", halo);
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  // using size_t here because weak scaling can go up to 2^31 elements
  size_t N = ((size_t)1) << numPow2;

  // assuming total work divides evenly across ranks
  if (N % (size_t)size != 0) {
    if (rank == 0) {
      fprintf(stderr, "Error: N = %zu is not divisible by number of MPI ranks = %d\n", N, size);
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  size_t local_n = N / (size_t)size;

  // MPI count args are int, so local chunk used in scatter/gather also fitting in int
  if (local_n > (size_t)2147483647) {
    if (rank == 0) {
      fprintf(stderr, "Error: local chunk size %zu exceeds MPI int count limit\n", local_n);
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  // each rank needing at least halo elements in real slice
  if (local_n < (size_t)halo) {
    if (rank == 0) {
      fprintf(stderr, "Error: local chunk size %zu is smaller than halo size %d\n", local_n, halo);
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  int *global_in  = NULL;
  int *global_out = NULL;
  int *serial_out = NULL;

  // only rank 0 needing full global arrays
  if (rank == 0) {
    global_in  = (int *)checked_malloc(N * sizeof(int), "global_in", rank);
    global_out = (int *)checked_malloc(N * sizeof(int), "global_out", rank);

    // only allocating serial_out for true serial case
    if (size == 1) {
      serial_out = (int *)checked_malloc(N * sizeof(int), "serial_out", rank);
    }

    // initializing input array to 1 so expected stencil output easy to verify
    for (size_t i = 0; i < N; i++) {
      global_in[i]  = 1;
      global_out[i] = 0;
      if (size == 1) {
        serial_out[i] = 0;
      }
    }
  }

  // each rank getting own local slice
  int *local_in  = (int *)checked_malloc(local_n * sizeof(int), "local_in", rank);
  int *local_out = (int *)checked_malloc(local_n * sizeof(int), "local_out", rank);

  // extended local array including lower halo + local data + upper halo
  int *local_with_halo = (int *)checked_malloc((local_n + 2 * (size_t)halo) * sizeof(int),
                                               "local_with_halo", rank);

  for (size_t i = 0; i < local_n; i++) {
    local_in[i] = 0;
    local_out[i] = 0;
  }

  for (size_t i = 0; i < local_n + 2 * (size_t)halo; i++) {
    local_with_halo[i] = 0;
  }

  // having all ranks meet here before timed section
  MPI_Barrier(MPI_COMM_WORLD);

  // starting timing right before MPI_Scatter on rank 0
  ticks parallel_start = 0;
  if (rank == 0) parallel_start = getticks();

  MPI_Scatter(global_in, (int)local_n, MPI_INT,
              local_in,  (int)local_n, MPI_INT,
              0, MPI_COMM_WORLD);

  // copying local slice into middle of extended array
  memcpy(local_with_halo + halo, local_in, local_n * sizeof(int));

  // for first and last global blocks, using same edge scheme as before:
  // replicating edge value into missing halo slots
  if (rank == 0) {
    for (int i = 0; i < halo; i++) {
      local_with_halo[i] = local_in[0];
    }
  }

  if (rank == size - 1) {
    for (int i = 0; i < halo; i++) {
      local_with_halo[halo + local_n + (size_t)i] = local_in[local_n - 1];
    }
  }

  // exchanging halos in parallel using non-blocking MPI calls
  MPI_Request reqs[4];
  int req_count = 0;

  // lower neighbor exchange
  if (rank > 0) {
    MPI_Irecv(local_with_halo, halo, MPI_INT,
              rank - 1, 100, MPI_COMM_WORLD, &reqs[req_count++]);

    MPI_Isend(local_in, halo, MPI_INT,
              rank - 1, 200, MPI_COMM_WORLD, &reqs[req_count++]);
  }

  // upper neighbor exchange
  if (rank < size - 1) {
    MPI_Irecv(local_with_halo + halo + local_n, halo, MPI_INT,
              rank + 1, 200, MPI_COMM_WORLD, &reqs[req_count++]);

    MPI_Isend(local_in + (local_n - (size_t)halo), halo, MPI_INT,
              rank + 1, 100, MPI_COMM_WORLD, &reqs[req_count++]);
  }

  if (req_count > 0) {
    MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);
  }

  // computing stencil on this rank's local data
  // using signed center index so negative k values behave correctly
  for (size_t i = 0; i < local_n; i++) {
    int sum = 0;
    long long center = (long long)halo + (long long)i;

    for (int k = -halo; k <= halo; k++) {
      sum += local_with_halo[center + k];
    }

    local_out[i] = sum;
  }

  MPI_Gather(local_out,  (int)local_n, MPI_INT,
             global_out, (int)local_n, MPI_INT,
             0, MPI_COMM_WORLD);

  // stopping timing once gather done
  ticks parallel_finish = 0;
  if (rank == 0) parallel_finish = getticks();

  // rank 0 does reporting and correctness checking
  if (rank == 0) {
    double parallel_seconds = (double)(parallel_finish - parallel_start) / 512000000.0;
    int expected = 2 * halo + 1;

    printf("1D Stencil using MPI (CPU only)\n");
    printf("MPI ranks = %d, N = 2^%d = %zu elements, HaloSize = %d\n", size, numPow2, N, halo);
    printf("Parallel time (Scatter + halo exchange + compute + Gather) = %.6lf seconds\n",
           parallel_seconds);

    // checking expected value correctness
    int errors_expected = 0;
    for (size_t i = 0; i < N; i++) {
      if (global_out[i] != expected) {
        errors_expected++;
        if (errors_expected <= 10) {
          printf("EXPECTED CHECK ERROR at i=%zu: got=%d expected=%d\n",
                 i, global_out[i], expected);
        }
      }
    }

    // doing serial timing/comparison for only 1 rank case because expensive
    if (size == 1) {
      ticks serial_start = getticks();

      for (size_t i = 0; i < N; i++) {
        int sum = 0;
        for (int k = -halo; k <= halo; k++) {
          size_t idx = clamp_idx_size((long long)i + (long long)k, N);
          sum += global_in[idx];
        }
        serial_out[i] = sum;
      }

      ticks serial_finish = getticks();
      double serial_seconds = (double)(serial_finish - serial_start) / 512000000.0;

      // comparing MPI result against serial result after timing done
      int errors_serial = 0;
      for (size_t i = 0; i < N; i++) {
        if (global_out[i] != serial_out[i]) {
          errors_serial++;
          if (errors_serial <= 10) {
            printf("SERIAL CHECK ERROR at i=%zu: mpi=%d serial=%d\n",
                   i, global_out[i], serial_out[i]);
          }
        }
      }

      printf("Serial time (rank 0 serial stencil) = %.6lf seconds\n", serial_seconds);

      if (errors_expected == 0 && errors_serial == 0) {
        printf("SUCCESS (all elements match expected %d and serial result)\n", expected);
      } else {
        printf("ERROR (expected mismatches = %d, serial mismatches = %d)\n",
               errors_expected, errors_serial);
      }
    } else {
      if (errors_expected == 0) {
        printf("SUCCESS (all elements match expected %d)\n", expected);
      } else {
        printf("ERROR (expected mismatches = %d)\n", errors_expected);
      }
    }
  }

  free(local_in);
  free(local_out);
  free(local_with_halo);

  if (rank == 0) {
    free(global_in);
    free(global_out);
    if (serial_out != NULL) {
      free(serial_out);
    }
  }

  MPI_Finalize();
  return 0;
}