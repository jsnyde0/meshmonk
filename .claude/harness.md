# Harness inventory — meshmonk

Per-repo inventory of verification mechanisms and fit profiles. Seeded 2026-08-09 by the Sunday generation session (dotpi-aut6.8).

## Mechanisms available

- CI (`.github/workflows/ci.yml`): pre-commit (ruff), pyright typecheck on `meshmonk/`, `pytest tests/ -q --ignore=tests/test_golden.py` (OS/compiler/py matrix), CLI integration smoke against real demo meshes, cibuildwheel dry-run.
- Nightly golden characterization (`.github/workflows/nightly.yml`): `tests/test_golden.py` — Tier-3 gating (`-m "not advisory"`, deterministic RMSE tolerance vs committed `tests/golden/*.npz`), Tier-3.5 legacy-baseline advisory (`continue-on-error`, cross-platform FP drift). Excluded from PR CI because SLOW (>30s/test, real ICP on a 10.6MB face scan), not flaky.

## Refactoring fit profile (gate manifest — dotpi-aut6.8, 2026-08-09)

Per the ratified agent-first refactoring principles (dotpi-2114 notes; charter amendment V2 on dotpi-aut6.5). The files named below are GATE FILES — a refactor diff must not touch them; a needed gate edit raises to the granting brain.

- **Gate command (VERIFIED 2026-08-09, dotpi-g0uu — the combined command is ADOPTED):** `uv run pytest tests/ -q --ignore=tests/test_golden.py && uv run pytest tests/test_golden.py -v -m "not advisory"`. Measured on this machine after `uv sync --extra dev --extra io`: leg 1 exit 0 in **24s** (372 collected, 17 skipped, 0 failed); leg 2 exit 0 in **24s** (3 passed, 1 deselected). **Total ~48s**, comfortably inside the ~2–5min budget — so the golden Tier-3 layer costs far less at dispatch time than its exclusion from PR CI suggests, and there is no reason to fall back to CI's command. The `golden-not-verified-this-dispatch` flag is retired.
  - Note when reading leg 1's output: `[tool.pytest.ini_options]` already sets `addopts = "-q ..."`, so the `-q` above makes it `-qq`, which suppresses the trailing `N passed` summary line. Exit code is the signal; add `-v` if you want the count on screen.
- **Gate files (fail-closed):** ALL of `tests/` (incl. `tests/golden/*.npz`, `tests/golden/legacy_baseline/`, `tests/conftest.py`), `.github/workflows/*.yml`, `pyproject.toml` `[tool.pytest.ini_options]`.
- **D3 conformance:** the golden layer is exactly characterization-at-the-stable-seam (D3.4) — deterministic tolerance, hermetic committed fixtures; the PR-CI suite alone omits it, which is why the dispatch-time gate includes it (verified cheap, above).
- **Prerequisite:** the gate needs dev deps present — `uv sync --extra dev --extra io` (~8s warm, needs network ONCE). A sandbox without network cannot run this gate at all; that is what left the command unverified until 2026-08-09.
- **Green-at-dispatch:** checked by the dispatching brain; result recorded on the dispatched bead.
