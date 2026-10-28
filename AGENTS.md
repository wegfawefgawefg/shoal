# shoal

Stockfish for Advance Wars: AW2 rules (AWBW standard, no luck, no COs, no fog), a search
engine, and a native replay viewer. Read `docs/plan.md` first (goals, status, queue), then
`docs/rules.md`, `docs/formats.md`, `docs/engine.md`.

- Build: `cmake --preset release && cmake --build --preset release` (Ninja, ccache, lld;
  `dev` and `asan` presets too). Tests: `./build/shoal_tests`. Keep the build fast.
- C++20 in the splonks-cpp "C+-" style: `namespace shoal`, PascalCase functions, snake_case
  fields and locals, `kConstants`, plain structs, no exceptions. Format with `.clang-format`.
  Small files (300-500 lines) grouped by domain: `src/rules`, `src/game`, `src/io`,
  `src/search`, `src/tools`, `src/viewer`.
- One `shoal` binary with subcommands in `src/main.cpp`; everything else is `shoal_core`
  so the tool, viewer and tests share it.
- Tests: add a `RunXTests(TestResults&)` per area and one call in `tests/test_main.cpp`.
- Heavy runs (solves, matches, tuning, fuzzing) go to stevejobs (`ssh mini`, 14-core M4 Pro;
  leave 2 cores free; `scripts/mini_sync.sh` pushes HEAD and builds in `~/shoal`). Small
  runs locally are fine.
- Never open a viewer window on the owner's display: use `--screenshot` with the offscreen
  driver (see `docs/viewer.md`).
- No Nintendo assets. The viewer art is our own.

## File Size

- Target file size is roughly 300-500 lines; split by responsibility when clearly bigger.

## Style

- Direct, explicit, readable, low-magic, modest abstraction, easy to trace in a debugger.
- Early returns over deep nesting; small helpers over abstraction stacks; concrete names.
