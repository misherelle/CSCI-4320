#include "pokemon_battle_cuda.h"

#include <cuda_runtime.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

__constant__ int d_effectiveness[POKEMON_TYPE_COUNT][POKEMON_TYPE_COUNT];

static const int h_effectiveness[POKEMON_TYPE_COUNT][POKEMON_TYPE_COUNT] = {
    {100,100,100,100,100,100,100,100,100,100,100,100, 50,  0,100,100, 50,100},
    {100, 50, 50,100,200,200,100,100,100,100,100,200, 50,100, 50,100,200,100},
    {100,200, 50,100, 50,100,100,100,200,100,100,100,200,100, 50,100,100,100},
    {100,100,200, 50, 50,100,100,100,  0,200,100,100,100,100, 50,100,100,100},
    {100, 50,200,100, 50,100,100, 50,200, 50,100, 50,200,100, 50,100, 50,100},
    {100, 50, 50,100,200, 50,100,100,200,200,100,100,100,100,200,100, 50,100},
    {200,100,100,100,100,200,100, 50,100, 50, 50, 50,200,  0,100,200,200, 50},
    {100,100,100,100,200,100,100, 50, 50,100,100,100, 50, 50,100,100,  0,200},
    {100,200,100,200, 50,100,100,200,100,  0,100, 50,200,100,100,100,200,100},
    {100,100,100, 50,200,100,200,100,100,100,100,200, 50,100,100,100, 50,100},
    {100,100,100,100,100,100,200,200,100,100, 50,100,100,100,100,  0, 50,100},
    {100, 50,100,100,200,100, 50, 50,100, 50,200,100,100, 50,100,200, 50, 50},
    {100,200,100,100,100,200, 50,100, 50,200,100,200,100,100,100,100, 50,100},
    {  0,100,100,100,100,100,100,100,100,100,200,100,100,200,100, 50,100,100},
    {100,100,100,100,100,100,100,100,100,100,100,100,100,100,200,100, 50,  0},
    {100,100,100,100,100,100, 50,100,100,100,200,100,100,200,100, 50,100, 50},
    {100, 50, 50, 50,100,200,100,100,100,100,100,100,200,100,100,100, 50,200},
    {100, 50,100,100,100,100,200, 50,100,100,100,100,100,100,200,200, 50,100}
};

static void cuda_check(cudaError_t err, const char *msg)
{
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA error during %s: %s\n", msg, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
}

void pokemon_cuda_init(void)
{
    cuda_check(cudaMemcpyToSymbol( d_effectiveness, h_effectiveness, sizeof(h_effectiveness)), "copy effectiveness table to constant memory");
}

__device__ uint8_t grid_get(const uint8_t *grid, size_t row, size_t col, size_t cols)
{
    return grid[row * cols + col];
}

__device__ int count_neighbor_type(const uint8_t *grid, size_t row, size_t col, uint8_t type, size_t cols, size_t min_row, size_t max_row)
{
    int count = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;

            long rr = (long)row + dr;
            long cc = (long)col + dc;

            if (rr < (long)min_row || rr >= (long)max_row || cc < 0 || cc >= (long)cols) {
                continue;
            }

            if (grid_get(grid, (size_t)rr, (size_t)cc, cols) == type) {
                count++;
            }
        }
    }

    return count;
}

__device__ int has_cardinal_type(const uint8_t *grid, size_t row, size_t col, uint8_t type, size_t cols, size_t min_row, size_t max_row)
{
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {
        long rr = (long)row + dr[i];
        long cc = (long)col + dc[i];

        if (rr < (long)min_row || rr >= (long)max_row || cc < 0 || cc >= (long)cols) {
            continue;
        }

        if (grid_get(grid, (size_t)rr, (size_t)cc, cols) == type) {
            return 1;
        }
    }

    return 0;
}

__device__ uint8_t cuda_next_cell(const uint8_t *grid, size_t row, size_t col, size_t cols, size_t min_row, size_t max_row)
{
    uint8_t center = grid_get(grid, row, col, cols);

    // rare evolution event 1: Normal -> Dragon
    if (center == PTYPE_NORMAL &&
        has_cardinal_type(grid, row, col, PTYPE_FIRE, cols, min_row, max_row) &&
        has_cardinal_type(grid, row, col, PTYPE_WATER, cols, min_row, max_row) &&
        has_cardinal_type(grid, row, col, PTYPE_GRASS, cols, min_row, max_row) &&
        count_neighbor_type(grid, row, col, PTYPE_ELECTRIC, cols, min_row, max_row) > 0) {
        return PTYPE_DRAGON;
    }

    // rare evolution event 2: Ice -> Steel
    if (center == PTYPE_ICE &&
        count_neighbor_type(grid, row, col, PTYPE_WATER, cols, min_row, max_row) >= 3 &&
        count_neighbor_type(grid, row, col, PTYPE_ELECTRIC, cols, min_row, max_row) >= 2) {
        return PTYPE_STEEL;
    }

    int pressure[POKEMON_TYPE_COUNT] = {0};
    int same_support = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;

            long rr = (long)row + dr;
            long cc = (long)col + dc;

            if (rr < (long)min_row || rr >= (long)max_row || cc < 0 || cc >= (long)cols) {
                continue;
            }

            uint8_t neighbor = grid_get(grid, (size_t)rr, (size_t)cc, cols);

            if (neighbor == center) {
                same_support++;
                pressure[center] += 120;
            } else {
                pressure[neighbor] += d_effectiveness[neighbor][center];
            }
        }
    }

    uint8_t best_type = center;
    int best_pressure = pressure[center];

    for (uint8_t type = 0; type < POKEMON_TYPE_COUNT; type++) {
        if (pressure[type] > best_pressure) {
            best_pressure = pressure[type];
            best_type = type;
        }
    }

    if (same_support >= 3 && best_pressure < pressure[center] + 220) {
        return center;
    }

    if (best_type != center && best_pressure >= pressure[center] + 150) {
        return best_type;
    }

    return center;
}

__global__ void pokemon_update_kernel(const uint8_t *current, uint8_t *next, size_t local_rows, size_t cols, size_t min_row, size_t max_row)
{
    size_t c = blockIdx.x * blockDim.x + threadIdx.x;
    size_t local_r = blockIdx.y * blockDim.y + threadIdx.y;

    // actual update rows are 1 through local_rows
    size_t r = local_r + 1;

    if (local_r >= local_rows || c >= cols) {
        return;
    }

    next[r * cols + c] = cuda_next_cell(current, r, c, cols, min_row, max_row);
}

void pokemon_cuda_update_local_rows(const PokemonGrid *current, PokemonGrid *next, size_t local_rows, int rank, int nprocs)
{
    size_t total_cells = current->rows * current->cols;
    size_t bytes = total_cells * sizeof(uint8_t);

    uint8_t *d_current = NULL;
    uint8_t *d_next = NULL;

    cuda_check(cudaMalloc((void **)&d_current, bytes), "cudaMalloc d_current");
    cuda_check(cudaMalloc((void **)&d_next, bytes), "cudaMalloc d_next");

    cuda_check(cudaMemcpy(d_current, current->cells, bytes, cudaMemcpyHostToDevice), "copy current grid to GPU");

    const size_t min_row = (rank == 0) ? 1 : 0;
    const size_t max_row = (rank == nprocs - 1) ? local_rows + 1 : local_rows + 2;

    dim3 block(16, 16);
    dim3 grid((unsigned int)((current->cols + block.x - 1) / block.x), (unsigned int)((local_rows + block.y - 1) / block.y)
    );

    pokemon_update_kernel<<<grid, block>>>(d_current, d_next, local_rows, current->cols, min_row, max_row);

    cuda_check(cudaGetLastError(), "launch pokemon_update_kernel");
    cuda_check(cudaDeviceSynchronize(), "synchronize pokemon_update_kernel");

    cuda_check(cudaMemcpy(next->cells, d_next, bytes, cudaMemcpyDeviceToHost), "copy next grid back to CPU");

    cuda_check(cudaFree(d_current), "cudaFree d_current");
    cuda_check(cudaFree(d_next), "cudaFree d_next");
}