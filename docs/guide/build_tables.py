#!/usr/bin/env python3
"""Fill the guide's computed tables from `shoal duel`.

The guide marks each generated table with <!-- TABLE name --> ... <!-- /TABLE name -->; this
script runs the engine and replaces what is between the markers, so the tables always match
the rules code.

usage (from the repo root, after a release build): python3 docs/guide/build_tables.py
"""
import os
import re
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SHOAL = os.path.join(ROOT, "build", "shoal")
GUIDE = os.path.join(ROOT, "docs", "guide", "index.html")

NAMES = {"inf": "Infantry", "mech": "Mech", "recon": "Recon", "tank": "Tank", "md": "Md.Tank",
         "neo": "Neotank", "aa": "Anti-Air", "bcopter": "B-Copter"}


def run(*args):
    return subprocess.run([SHOAL, *args], check=True, capture_output=True, text=True).stdout


def rows_of(block):
    rows = []
    for line in block.strip().splitlines():
        if "|" in line:
            rows.append([cell.strip() for cell in line.split("|")])
    return rows


def duel_table(stepper, striker):
    """footing x footing: does the unit that steps in win, and with what left"""
    rows = rows_of(run("duel", stepper, striker))
    # footings neither unit can use (mountains for vehicles) are left out
    rows = [rows[0]] + [r for r in rows[1:] if any(c != "-" for c in r[1:])]
    keep = [0] + [i for i in range(1, len(rows[0]))
                  if any(r[i] != "-" for r in rows[1:])]
    rows = [[r[i] for i in keep] for r in rows]
    header = rows[0]
    out = ['<table class="duel">', "<tr><th>steps in on &darr; / strikes from &rarr;</th>"]
    out += [f"<th>{name}</th>" for name in header[1:]]
    out.append("</tr>")
    for row in rows[1:]:
        out.append(f"<tr><th>{row[0]}</th>")
        for cell in row[1:]:
            if cell == "-":
                out.append('<td class="na">-</td>')
                continue
            first = re.search(r"\((\d+)/(\d+)\)", cell)
            after = f"{first.group(1)} v {first.group(2)}" if first else ""
            if cell.startswith("WIN"):
                left = cell.split()[1]
                out.append(f'<td class="win">win, {left} left<br><small>{after}</small></td>')
            elif cell.startswith("loss"):
                out.append(f'<td class="loss">loss<br><small>{after}</small></td>')
            else:
                out.append(f'<td class="na">hold<br><small>{after}</small></td>')
        out.append("</tr>")
    out.append("</table>")
    return "\n".join(out)


def matrix_table(striker_footing):
    text = run("duel", "--matrix")
    blocks = re.split(r"\n(?=least footing)", text.strip())
    block = next(b for b in blocks if f"striker on {striker_footing} " in b)
    rows = rows_of(block)
    header = rows[0]
    out = ['<table class="matrix">',
           "<tr><th>steps in &darr; / strikes first &rarr;</th>"]
    out += [f"<th>{NAMES.get(name, name)}</th>" for name in header[1:]]
    out.append("</tr>")
    rank = {"road": "a", "plain": "a", "wood": "b", "city": "c", "own city": "d",
            "never": "e", "-": "f"}
    for row in rows[1:]:
        out.append(f"<tr><th>{NAMES.get(row[0], row[0])}</th>")
        for cell in row[1:]:
            shown = "anywhere" if cell == "road" else cell
            out.append(f'<td class="m{rank.get(cell, "f")}">{shown}</td>')
        out.append("</tr>")
    out.append("</table>")
    return "\n".join(out)


TABLES = {
    "same-type": lambda: duel_table("tank", "tank"),
    "recon": lambda: duel_table("recon", "recon"),
    "aa": lambda: duel_table("aa", "aa"),
    "matrix-plain": lambda: matrix_table("plain"),
    "matrix-city": lambda: matrix_table("city"),
}


def main():
    with open(GUIDE) as f:
        html = f.read()
    for name, make in TABLES.items():
        pattern = re.compile(rf"(<!-- TABLE {name} -->).*?(<!-- /TABLE {name} -->)", re.S)
        if not pattern.search(html):
            print("no marker for", name)
            continue
        html = pattern.sub(lambda m: m.group(1) + "\n" + make() + "\n" + m.group(2), html)
        print("filled", name)
    with open(GUIDE, "w") as f:
        f.write(html)


if __name__ == "__main__":
    main()
