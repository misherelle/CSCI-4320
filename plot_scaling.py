#!/usr/bin/env python3
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_rows(path):
    with open(path, newline="") as fp:
        return list(csv.DictReader(fp))


def plot_csv(csv_path, output_path, title):
    rows = read_rows(csv_path)
    if not rows:
        raise SystemExit(f"{csv_path} has no data rows")

    ranks = [int(row["ranks"]) for row in rows]
    total = [float(row["total_seconds"]) for row in rows]
    comm = [float(row["comm_seconds"]) for row in rows]
    compute = [float(row["compute_seconds"]) for row in rows]

    plt.figure(figsize=(7, 4.4))
    plt.plot(ranks, total, marker="o", label="Total")
    plt.plot(ranks, comm, marker="s", label="Communication")
    plt.plot(ranks, compute, marker="^", label="Computation")
    plt.xlabel("MPI ranks")
    plt.ylabel("Seconds")
    plt.title(title)
    plt.grid(True, alpha=0.35)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_path, dpi=200)
    plt.close()


def main():
    results = Path("results")
    plots = Path("plots")
    plots.mkdir(exist_ok=True)

    strong = results / "strong_scaling.csv"
    weak = results / "weak_scaling.csv"

    if strong.exists():
        plot_csv(strong, plots / "strong_scaling.png", "Strong Scaling")
    if weak.exists():
        plot_csv(weak, plots / "weak_scaling.png", "Weak Scaling")

    if not strong.exists() and not weak.exists():
        raise SystemExit("No results CSV files found. Run the scaling scripts first.")


if __name__ == "__main__":
    main()
