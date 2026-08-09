# Harness inventory — meshmonk

Per-repo inventory of verification mechanisms and fit profiles. Seeded 2026-08-09 by the Sunday generation session (dotpi-aut6.8).

## Mechanisms available

- CI (`.github/workflows/ci.yml`): pre-commit (ruff), pyright typecheck on `meshmonk/`, `pytest tests/ -q --ignore=tests/test_golden.py` (OS/compiler/py matrix), CLI integration smoke against real demo meshes, cibuildwheel dry-run.
- Nightly golden characterization (`.github/workflows/nightly.yml`): `tests/test_golden.py` — Tier-3 gating (`-m "not advisory"`, deterministic RMSE tolerance vs committed `tests/golden/*.npz`), Tier-3.5 legacy-baseline advisory (`continue-on-error`, cross-platform FP drift). Excluded from PR CI because SLOW (>30s/test, real ICP on a 10.6MB face scan), not flaky.

## Refactoring fit profile (gate manifest — dotpi-aut6.8, 2026-08-09)

Per the ratified agent-first refactoring principles (dotpi-2114 notes; charter amendment V2 on dotpi-aut6.5). The files named below are GATE FILES — a refactor diff must not touch them; a needed gate edit raises to the granting brain.

- **Gate command (proposed, timing UNVERIFIED locally — dev-dep install needs network; verification bead filed 2026-08-09):** `uv run pytest tests/ -q --ignore=tests/test_golden.py && uv run pytest tests/test_golden.py -v -m "not advisory"` (budget ~2-5min). Fallback until verified: `uv run pytest tests/ -q --ignore=tests/test_golden.py` (CI's own command) + flag `golden-not-verified-this-dispatch` on the dispatched bead.
- **Gate files (fail-closed):** ALL of `tests/` (incl. `tests/golden/*.npz`, `tests/golden/legacy_baseline/`, `tests/conftest.py`), `.github/workflows/*.yml`, `pyproject.toml` `[tool.pytest.ini_options]`.
- **D3 conformance:** the golden layer is exactly characterization-at-the-stable-seam (D3.4) — deterministic tolerance, hermetic committed fixtures; the PR-CI suite alone omits it, hence the proposed dispatch-time inclusion.
- **Green-at-dispatch:** checked by the dispatching brain; result recorded on the dispatched bead.
