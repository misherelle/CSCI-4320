/*
  CSCI-4320/6360: Assignment #5 - MPI Parallel I/O Benchmark
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef unsigned long long ticks;

#define CLOCK_RATE_HZ 512000000.0
#define DEFAULT_NUM_BLOCKS 32

/* POWER9 / AiMOS hardware clock */
static inline ticks aimos_clock_read(void)
{
    unsigned int tbl, tbu0, tbu1;
    do {
        __asm__ __volatile__("mftbu %0" : "=r"(tbu0));
        __asm__ __volatile__("mftb %0"  : "=r"(tbl));
        __asm__ __volatile__("mftbu %0" : "=r"(tbu1));
    } while (tbu0 != tbu1);
    return (((unsigned long long)tbu0) << 32) | tbl;
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
                "  %s <file_path> <block_size_mb> <storage_label> <csv_file>\n\n"
                "Example:\n"
                "  %s /gpfs/u/scratch/.../test.bin 4 scratch results.csv\n"
                "  %s /mnt/nvme/uid_${SLURM_JOB_UID}/job_${SLURM_JOB_ID}/test.bin 4 nvme results.csv\n",
                prog, prog, prog);
    }
    MPI_Finalize();
    exit(EXIT_FAILURE);
}

static int valid_block_size_mb(int mb)
{
    return (mb == 1 || mb == 2 || mb == 4 || mb == 8 || mb == 16);
}

static MPI_Offset compute_offset(int rank, int nprocs, int block_idx, MPI_Offset block_bytes)
{
    return ((MPI_Offset)block_idx * (MPI_Offset)nprocs + (MPI_Offset)rank) * block_bytes;
}

static int verify_ones(const unsigned char *buf, size_t nbytes)
{
    for (size_t i = 0; i < nbytes; i++) {
        if (buf[i] != 1) {
            return 0;
        }
    }
    return 1;
}

static void delete_file_if_present(const char *path, int rank)
{
    int rc;
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        rc = MPI_File_delete((char *)path, MPI_INFO_NULL);
        if (rc != MPI_SUCCESS) {
            char err[MPI_MAX_ERROR_STRING];
            int len = 0;
            MPI_Error_string(rc, err, &len);
            /* Ignore “file does not exist” style failures, abort on others. */
            if (strstr(err, "no such") == NULL && strstr(err, "No such") == NULL &&
                strstr(err, "does not exist") == NULL) {
                fprintf(stderr, "Rank 0: MPI_File_delete failed for %s: %s\n", path, err);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

static void append_csv(const char *csv_file,
                       const char *storage,
                       const char *op,
                       int ranks,
                       int block_size_mb,
                       int num_blocks,
                       double seconds,
                       double mib_per_sec,
                       double gib_per_sec,
                       int verify_ok)
{
    FILE *fp = fopen(csv_file, "a+");
    if (fp == NULL) {
        fprintf(stderr, "Could not open CSV file %s\n", csv_file);
        return;
    }

    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0) {
        fprintf(fp,
                "storage,operation,ranks,block_size_mb,num_blocks,seconds,MiB_per_sec,GiB_per_sec,verify\n");
    }

    fprintf(fp, "%s,%s,%d,%d,%d,%.9f,%.6f,%.6f,%s\n",
            storage, op, ranks, block_size_mb, num_blocks,
            seconds, mib_per_sec, gib_per_sec, verify_ok ? "PASS" : "FAIL");
    fclose(fp);
}

int main(int argc, char **argv)
{
    int rank, nprocs;
    const int num_blocks = DEFAULT_NUM_BLOCKS;
    MPI_File fh;
    int rc;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 5) {
        usage_and_exit(argv[0], rank);
    }

    const char *file_path = argv[1];
    int block_size_mb = atoi(argv[2]);
    const char *storage_label = argv[3];
    const char *csv_file = argv[4];

    if (!valid_block_size_mb(block_size_mb)) {
        if (rank == 0) {
            fprintf(stderr, "Error: block_size_mb must be one of 1, 2, 4, 8, or 16.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    MPI_Offset block_bytes = (MPI_Offset)block_size_mb * 1024LL * 1024LL;
    size_t block_nbytes = (size_t)block_bytes;

    unsigned char *write_buf = (unsigned char *)checked_malloc(block_nbytes, "write_buf", rank);
    unsigned char *read_buf  = (unsigned char *)checked_malloc(block_nbytes, "read_buf", rank);
    memset(write_buf, 1, block_nbytes);
    memset(read_buf, 0, block_nbytes);

    /* Delete the benchmark file before this specific test. */
    delete_file_if_present(file_path, rank);

    /* ---------------- WRITE TEST ---------------- */
    MPI_Barrier(MPI_COMM_WORLD);
    ticks write_start = aimos_clock_read();

    rc = MPI_File_open(MPI_COMM_WORLD,
                       (char *)file_path,
                       MPI_MODE_CREATE | MPI_MODE_WRONLY,
                       MPI_INFO_NULL,
                       &fh);
    mpi_fail(rc, "MPI_File_open for write", rank);

    for (int b = 0; b < num_blocks; b++) {
        MPI_Offset offset = compute_offset(rank, nprocs, b, block_bytes);
        rc = MPI_File_write_at(fh, offset, write_buf, (int)block_nbytes, MPI_BYTE, MPI_STATUS_IGNORE);
        mpi_fail(rc, "MPI_File_write_at", rank);
    }

    rc = MPI_File_sync(fh);
    mpi_fail(rc, "MPI_File_sync", rank);

    rc = MPI_File_close(&fh);
    mpi_fail(rc, "MPI_File_close after write", rank);

    MPI_Barrier(MPI_COMM_WORLD);
    ticks write_end = aimos_clock_read();

    double write_local_sec = ticks_to_seconds(write_end - write_start);
    double write_sec = 0.0;
    MPI_Reduce(&write_local_sec, &write_sec, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    /* ---------------- READ TEST ---------------- */
    MPI_Barrier(MPI_COMM_WORLD);
    ticks read_start = aimos_clock_read();

    rc = MPI_File_open(MPI_COMM_WORLD,
                       (char *)file_path,
                       MPI_MODE_RDONLY,
                       MPI_INFO_NULL,
                       &fh);
    mpi_fail(rc, "MPI_File_open for read", rank);

    int local_verify_ok = 1;

    for (int b = 0; b < num_blocks; b++) {
        MPI_Offset offset = compute_offset(rank, nprocs, b, block_bytes);
        memset(read_buf, 0, block_nbytes);
        rc = MPI_File_read_at(fh, offset, read_buf, (int)block_nbytes, MPI_BYTE, MPI_STATUS_IGNORE);
        mpi_fail(rc, "MPI_File_read_at", rank);
        if (!verify_ones(read_buf, block_nbytes)) {
            local_verify_ok = 0;
        }
    }

    rc = MPI_File_close(&fh);
    mpi_fail(rc, "MPI_File_close after read", rank);

    MPI_Barrier(MPI_COMM_WORLD);
    ticks read_end = aimos_clock_read();

    double read_local_sec = ticks_to_seconds(read_end - read_start);
    double read_sec = 0.0;
    MPI_Reduce(&read_local_sec, &read_sec, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    int global_verify_ok = 0;
    MPI_Reduce(&local_verify_ok, &global_verify_ok, 1, MPI_INT, MPI_LAND, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double total_bytes = (double)nprocs * (double)num_blocks * (double)block_bytes;
        double total_mib = total_bytes / (1024.0 * 1024.0);
        double total_gib = total_bytes / (1024.0 * 1024.0 * 1024.0);

        double write_mib_per_sec = total_mib / write_sec;
        double write_gib_per_sec = total_gib / write_sec;
        double read_mib_per_sec  = total_mib / read_sec;
        double read_gib_per_sec  = total_gib / read_sec;

        printf("============================================================\n");
        printf("MPI Parallel I/O Benchmark\n");
        printf("storage         = %s\n", storage_label);
        printf("file            = %s\n", file_path);
        printf("ranks           = %d\n", nprocs);
        printf("block_size_mb   = %d\n", block_size_mb);
        printf("num_blocks      = %d\n", num_blocks);
        printf("total_data_mib  = %.3f\n", total_mib);
        printf("write_seconds   = %.9f\n", write_sec);
        printf("write_MiB_per_s = %.6f\n", write_mib_per_sec);
        printf("write_GiB_per_s = %.6f\n", write_gib_per_sec);
        printf("read_seconds    = %.9f\n", read_sec);
        printf("read_MiB_per_s  = %.6f\n", read_mib_per_sec);
        printf("read_GiB_per_s  = %.6f\n", read_gib_per_sec);
        printf("verify          = %s\n", global_verify_ok ? "PASS" : "FAIL");
        printf("============================================================\n");

        append_csv(csv_file, storage_label, "write", nprocs, block_size_mb, num_blocks,
                   write_sec, write_mib_per_sec, write_gib_per_sec, global_verify_ok);
        append_csv(csv_file, storage_label, "read", nprocs, block_size_mb, num_blocks,
                   read_sec, read_mib_per_sec, read_gib_per_sec, global_verify_ok);
    }

    /* Delete benchmark data file after this test, per assignment. */
    delete_file_if_present(file_path, rank);

    free(write_buf);
    free(read_buf);
    MPI_Finalize();
    return 0;
}