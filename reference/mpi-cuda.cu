/*
  CSCI-4320: Assignment #4 - 1-D Stencil using MPI & CUDA (CUDA Code)
*/

#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <cuda_runtime.h>

#define NUM_THREADS_PER_BLOCK 1024
#define GRID_SIZE 16384

extern "C"
{
void runCudaLand(const int *host_in_with_halo,
                 int *host_out,
                 size_t local_n,
                 int halo,
                 int myrank);
}

// checking CUDA calls and exiting on error
static inline void cuda_check(cudaError_t err, const char *what)
{
  if (err != cudaSuccess) {
    fprintf(stderr, "CUDA error during %s: %s\n", what, cudaGetErrorString(err));
    exit(EXIT_FAILURE);
  }
}

// having fixed grid size
__global__ void stencil_kernel(const int *in_with_halo, int *out, size_t local_n, int halo)
{
  // computing global thread id and stride for grid-strided loop
  size_t tid = (size_t)blockIdx.x * (size_t)blockDim.x + (size_t)threadIdx.x;
  size_t stride = (size_t)blockDim.x * (size_t)gridDim.x;

  // having each thread process multiple elements if needed
  for (size_t i = tid; i < local_n; i += stride) {
    int sum = 0;

    // shifting by halo to align with center of local data
    long long center = (long long)halo + (long long)i;

    // summing across stencil window
    for (int k = -halo; k <= halo; k++) {
      sum += in_with_halo[center + k];
    }

    // writing computed stencil result
    out[i] = sum;
  }
}

void runCudaLand(const int *host_in_with_halo,
                 int *host_out,
                 size_t local_n,
                 int halo,
                 int myrank)
{
  int device_count = 0;
  cuda_check(cudaGetDeviceCount(&device_count), "cudaGetDeviceCount");

  if (device_count <= 0) {
    fprintf(stderr, "Rank %d: no CUDA devices found\n", myrank);
    exit(EXIT_FAILURE);
  }

  // having one GPU per rank
  int device = myrank % device_count;
  cuda_check(cudaSetDevice(device), "cudaSetDevice");

  // computing input and output buffer sizes
  size_t in_count = local_n + 2 * (size_t)halo;
  size_t in_bytes = in_count * sizeof(int);
  size_t out_bytes = local_n * sizeof(int);

  int *managed_in = NULL;
  int *managed_out = NULL;

  // allocating unified memory for GPU compute
  cuda_check(cudaMallocManaged(&managed_in, in_bytes), "cudaMallocManaged managed_in");
  cuda_check(cudaMallocManaged(&managed_out, out_bytes), "cudaMallocManaged managed_out");

  // copying host input with halo into managed memory
  for (size_t i = 0; i < in_count; i++) {
    managed_in[i] = host_in_with_halo[i];
  }

  // initializing output buffer
  for (size_t i = 0; i < local_n; i++) {
    managed_out[i] = 0;
  }

  // turning prefetching on
  cuda_check(cudaMemPrefetchAsync(managed_in, in_bytes, device), "cudaMemPrefetchAsync managed_in");
  cuda_check(cudaMemPrefetchAsync(managed_out, out_bytes, device), "cudaMemPrefetchAsync managed_out");
  cuda_check(cudaDeviceSynchronize(), "cudaDeviceSynchronize before kernel");

  // launching stencil kernel on GPU
  stencil_kernel<<<GRID_SIZE, NUM_THREADS_PER_BLOCK>>>(managed_in, managed_out, local_n, halo);
  cuda_check(cudaGetLastError(), "kernel launch");
  cuda_check(cudaDeviceSynchronize(), "cudaDeviceSynchronize after kernel");

  // bringing back to CPU
  cuda_check(cudaMemPrefetchAsync(managed_out, out_bytes, cudaCpuDeviceId),
             "cudaMemPrefetchAsync managed_out to cpu");
  cuda_check(cudaDeviceSynchronize(), "cudaDeviceSynchronize after cpu prefetch");

  // copying final results back into host output buffer
  for (size_t i = 0; i < local_n; i++) {
    host_out[i] = managed_out[i];
  }

  // freeing unified memory
  cuda_check(cudaFree(managed_in), "cudaFree managed_in");
  cuda_check(cudaFree(managed_out), "cudaFree managed_out");
}