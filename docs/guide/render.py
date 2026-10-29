#!/usr/bin/env python3
"""Render the guide's pictures with the viewer, headless.

Each shot is a game or map file, a ply, a window size and extra viewer flags. The viewer
draws with a software renderer and no window (never on the screen); the BMP it writes is
turned into a PNG next to the guide and the BMP is deleted.

usage (from the repo root): python3 docs/guide/render.py
"""
import os
import subprocess
import tempfile

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VIEWER = os.path.join(ROOT, "build", "shoal_viewer")
POS = os.path.join(ROOT, "docs", "guide", "positions")
IMG = os.path.join(ROOT, "docs", "guide", "img")

# name: (file, ply or None for the last, size, extra flags)
SHOTS = {
    "step-in": (f"{POS}/step-in.sexp", None, "1100x440", []),
    "stay-out": (f"{POS}/stay-out.sexp", None, "1100x440", ["--play", "--select", "h2"]),
    "mountain": (f"{POS}/mountain.sexp", None, "1100x440", []),
    "art-ring": (f"{POS}/art-ring.sexp", None, "1100x440", []),
    "art-pinned": (f"{POS}/art-pinned.sexp", None, "1100x440", ["--play", "--select", "g2"]),
    "rockets": (f"{POS}/rockets.sexp", None, "1100x440", []),
    "snowball-1": (f"{POS}/snowball.sexp", 3, "1100x440", []),
    "snowball-2": (f"{POS}/snowball.sexp", 5, "1100x440", []),
    "snowball-3": (f"{POS}/snowball.sexp", None, "1100x440", []),
    "day-limit-loss": (f"{ROOT}/games/gen16-water-21-analysed.sexp", -70, "1300x840", []),
    "solved-city5": (f"{ROOT}/games/solved/city5.sexp", 3, "1100x620", []),
}


def render(name, path, ply, size, extra):
    env = dict(os.environ)
    env.pop("DISPLAY", None)
    env.pop("WAYLAND_DISPLAY", None)
    with tempfile.TemporaryDirectory() as tmp:
        bmp = os.path.join(tmp, name + ".bmp")
        args = [VIEWER, path, "--screenshot", bmp, "--size", size] + extra
        if ply is not None:
            args += ["--ply", str(ply)]
        subprocess.run(args, env=env, check=True, capture_output=True)
        Image.open(bmp).save(os.path.join(IMG, name + ".png"), optimize=True)


def main():
    os.makedirs(IMG, exist_ok=True)
    for name, (path, ply, size, extra) in SHOTS.items():
        render(name, path, ply, size, extra)
        print("rendered", name)


if __name__ == "__main__":
    main()
