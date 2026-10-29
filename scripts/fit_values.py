#!/usr/bin/env python3
"""Fit a logistic model of game results to turn-start features from `shoal dataset`.

Each row is a turn start seen by the side to move; the model predicts that side's result
(1 win, 0.5 draw, 0 loss) from its material per unit type, property, funds and capture
differences. The material coefficients say how much a thousand funds of each unit type moves
the odds of winning; dividing by the average gives each type's worth relative to the rest.

usage: scripts/fit_values.py rows.csv [--min-days-left N]
"""
import sys

import numpy as np


def load(path):
    with open(path) as f:
        header = f.readline().strip().split(",")
        rows = [list(map(float, line.strip().split(","))) for line in f if line.strip()]
    return header, np.array(rows)


def fit(x, y, l2=1e-3, steps=60):
    # newton's method on the regularised log loss; y may be 0.5 for draws
    w = np.zeros(x.shape[1])
    for _ in range(steps):
        p = 1.0 / (1.0 + np.exp(-x @ w))
        grad = x.T @ (p - y) + l2 * w
        hess = (x * (p * (1 - p))[:, None]).T @ x + l2 * np.eye(len(w))
        w -= np.linalg.solve(hess, grad)
    return w


def main():
    path = sys.argv[1]
    header, data = load(path)
    col = {name: i for i, name in enumerate(header)}
    min_left = 0
    if "--min-days-left" in sys.argv:
        min_left = int(sys.argv[sys.argv.index("--min-days-left") + 1])
    data = data[data[:, col["days_left"]] >= min_left]
    mats = [name for name in header if name.startswith("mat_")]
    # drop unit types that never appear
    mats = [m for m in mats if np.abs(data[:, col[m]]).sum() > 0]
    feats = mats + ["props", "funds", "capturing"]
    x = np.column_stack([data[:, col[f]] for f in feats])
    y = data[:, col["result"]]
    w = fit(x, y)
    p = 1.0 / (1.0 + np.exp(-x @ w))
    accuracy = np.mean((p > 0.5) == (y > 0.5))
    games = len(np.unique(data[:, col["game"]]))
    print(f"{games} games, {len(y)} rows, accuracy {accuracy:.3f}")
    print("\nlogit per 1000 funds of material (and per property / per 1000 cash / per capture):")
    for name, coef in zip(feats, w):
        print(f"  {name:14s} {coef:+.4f}")
    mat_w = np.array([w[feats.index(m)] for m in mats])
    mean = mat_w.mean()
    print("\nworth per fund relative to the average unit type:")
    for m, coef in sorted(zip(mats, mat_w), key=lambda t: -t[1]):
        print(f"  {m[4:]:8s} {100 * coef / mean:5.0f}%")
    funds_coef = w[feats.index("funds")]
    props_coef = w[feats.index("props")]
    print(f"\na property is worth about {1000 * props_coef / mean:.0f} funds of average army;"
          f" 1000 cash is worth {1000 * funds_coef / mean:.0f}")


if __name__ == "__main__":
    main()
