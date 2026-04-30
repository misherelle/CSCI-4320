import pandas as pd
import matplotlib.pyplot as plt

mpi = pd.read_csv("data/mpi/mpi_results.csv")
cuda = pd.read_csv("data/mpicuda/mpicuda_results.csv")

# strong scaling graph
mpi_strong = mpi[(mpi["experiment"] == "strong_1024") & (mpi["ranks"].isin([1, 2, 4]))]
cuda_strong = cuda[cuda["experiment"] == "strong_1024"]

plt.figure(figsize=(8, 5))
plt.plot(mpi_strong["ranks"], mpi_strong["total_sec"], marker="o", linewidth=2, markersize=7, label="MPI-only")
plt.plot(cuda_strong["ranks"], cuda_strong["total_sec"], marker="o", linewidth=2, markersize=7, label="MPI + CUDA")

plt.xlabel("Number of Ranks")
plt.ylabel("Runtime (seconds, log scale)")
plt.title("Strong Scaling: MPI-only vs MPI + CUDA\n1024 x 1024 Grid, 100 Steps")
plt.yscale("log")
plt.xticks([1, 2, 4])
plt.grid(True, which="both")
plt.legend()
plt.tight_layout()
plt.savefig("strong_scaling_mpi_vs_cuda.png", dpi=300)
plt.show()

# weak scaling graph
mpi_weak = mpi[(mpi["experiment"] == "weak_256_per_rank") & (mpi["ranks"].isin([1, 2, 4]))]
cuda_weak = cuda[cuda["experiment"] == "weak_512_per_rank"]

plt.figure(figsize=(8, 5))
plt.plot(mpi_weak["ranks"], mpi_weak["total_sec"], marker="o", linewidth=2, markersize=7, label="MPI-only")
plt.plot(cuda_weak["ranks"], cuda_weak["total_sec"], marker="o", linewidth=2, markersize=7, label="MPI + CUDA")

plt.xlabel("Number of Ranks")
plt.ylabel("Runtime (seconds)")
plt.title("Weak Scaling: MPI-only vs MPI + CUDA\n100 Steps")
plt.xticks([1, 2, 4])
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("weak_scaling_mpi_vs_cuda.png", dpi=300)
plt.show()