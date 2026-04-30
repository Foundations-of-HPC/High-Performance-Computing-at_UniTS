"""
analyse.py — a small scientific pipeline: read observations, compute
statistics, write results.

Inputs:
    /data/in/weather_observations.csv       hourly observations (read-only)

Outputs (all under /data/out/, created if missing):
    summary.json                            overall statistics
    daily_means.csv                         per-day aggregates
    correlations.txt                        pairwise correlations

This deliberately produces three outputs of different shapes so that
Chapter 7's volume demos have something interesting to persist and inspect.

The directory structure (/data/in, /data/out) is the Docker-idiomatic
choice: inputs and outputs are separate mount points, with different
access patterns (read-only vs read-write). Real pipelines on HPC and
cloud platforms follow the same split.
"""

import json
import os
import sys
from pathlib import Path

import numpy as np
import pandas as pd


IN_DIR  = Path(os.environ.get("IN_DIR",  "/data/in"))
OUT_DIR = Path(os.environ.get("OUT_DIR", "/data/out"))


def main():
    in_file = IN_DIR / "weather_observations.csv"
    if not in_file.exists():
        print(f"ERROR: input file not found: {in_file}", file=sys.stderr)
        sys.exit(1)

    print(f"reading  {in_file}")
    df = pd.read_csv(in_file)
    print(f"  {len(df)} observations, columns = {list(df.columns)}")

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # ---- 1. overall summary --------------------------------------------
    summary = {
        "n_observations": int(len(df)),
        "temperature_c": {
            "mean": float(df.temperature_c.mean()),
            "std":  float(df.temperature_c.std()),
            "min":  float(df.temperature_c.min()),
            "max":  float(df.temperature_c.max()),
        },
        "pressure_hpa": {
            "mean": float(df.pressure_hpa.mean()),
            "std":  float(df.pressure_hpa.std()),
            "min":  float(df.pressure_hpa.min()),
            "max":  float(df.pressure_hpa.max()),
        },
        "humidity_pct": {
            "mean": float(df.humidity_pct.mean()),
            "std":  float(df.humidity_pct.std()),
            "min":  float(df.humidity_pct.min()),
            "max":  float(df.humidity_pct.max()),
        },
    }
    with open(OUT_DIR / "summary.json", "w") as f:
        json.dump(summary, f, indent=2)
    print(f"wrote    {OUT_DIR / 'summary.json'}")

    # ---- 2. daily aggregates -------------------------------------------
    df["day"] = df.hour // 24
    daily = df.groupby("day").agg({
        "temperature_c": ["mean", "min", "max"],
        "pressure_hpa":  ["mean"],
        "humidity_pct":  ["mean"],
    })
    daily.columns = [f"{c[0]}_{c[1]}" for c in daily.columns]
    daily.to_csv(OUT_DIR / "daily_means.csv")
    print(f"wrote    {OUT_DIR / 'daily_means.csv'} ({len(daily)} days)")

    # ---- 3. pairwise correlations --------------------------------------
    corr = df[["temperature_c", "pressure_hpa", "humidity_pct"]].corr()
    with open(OUT_DIR / "correlations.txt", "w") as f:
        f.write("Pairwise Pearson correlations\n")
        f.write("=" * 40 + "\n")
        f.write(corr.to_string())
        f.write("\n")
    print(f"wrote    {OUT_DIR / 'correlations.txt'}")

    # ---- summary on stdout ---------------------------------------------
    print()
    print("analysis complete.")
    print(f"  mean temperature   = {summary['temperature_c']['mean']:.2f} °C")
    print(f"  mean pressure      = {summary['pressure_hpa']['mean']:.2f} hPa")
    print(f"  temperature–humidity correlation = {corr.loc['temperature_c','humidity_pct']:.3f}")


if __name__ == "__main__":
    main()

