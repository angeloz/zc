# AGENTS

This file describes how to work on this repository safely and consistently.

## Project Intent

- `zc`, short for zero-commander, is a USB-first portable two-panel file commander.
- The portable distribution unit is a small bundle, not just a bare binary.
- The product value is host-filesystem work from a removable or arbitrary directory, without installation.
- The design target is still closer to `kilo` than to full `mc`.

## Current Platform Status

- current implementation target: POSIX-style terminals on macOS and Linux
- future product target: macOS, Linux, and Windows portable bundles
- do not describe Windows support as complete until the runtime and validation work actually exist

## Source Layout

- [src/zc.c](/Users/azaia/Git/mc/src/zc.c): main application, TUI, filesystem actions, launcher logic, bundle-relative helper discovery.
- [Makefile](/Users/azaia/Git/mc/Makefile): local build, bundle targets, and vendored `zc-kilo` build.
- [third_party/kilo](/Users/azaia/Git/mc/third_party/kilo): vendored upstream `kilo` source with minimal local patches.

## Build Expectations

- `make` must build both `zc` and `zc-kilo`.
- `make bundle` must produce the stable native bundle layout under `dist/`.
- `make cosmo` is intended to build both `zc.com` and `zc-kilo.com`.
- `make bundle-cosmo` is intended to produce the stable Cosmopolitan bundle layout under `dist/`.
- Avoid introducing dependencies on `glib`, `ncurses`, `slang`, or other heavy runtime libraries.
- Keep native `.zcc` container handling independent from external archive helpers.
- Keep encrypted `.zcc` metadata hidden; names, paths, and metadata should not leak in cleartext.
- Keep encrypted `.zcc` on a vendored cross-platform crypto backend, not host-specific libraries.

## Bundle Policy

- Treat the distributed unit as a portable bundle:
  - `zc` or `zc.com`
  - `zc-kilo` or `zc-kilo.com`
  - optional helper tools in `tools/`
- Keep sibling-relative lookup deterministic for the bundled editor.
- Optional helper-backed features should prefer bundle-relative lookup before `PATH`.
- Core filesystem behavior must not depend on host `PATH`.
- Do not add silent `PATH` fallback for the Cosmopolitan editor path.

## Scope And Priorities

Primary scope:

- browse directories
- copy and move files
- create, rename, delete
- inspect and edit text files through `zc-kilo`
- marked multi-selection workflows

Lower-priority or constrained scope:

- pack/unpack commands may exist, but archive browsing stays out of scope
- native `.zcc` containers are acceptable only as extract-first flows, not as browsable VFS in v1
- encrypted `.zcc` should stay passphrase-per-operation in v1; no session key cache
- no subshell
- no remote VFS
- no menu bar or user menu; `F9` is available for native `zc` container creation
- no background jobs
- no separate internal editor/viewer beyond launching `zc-kilo`

## Editing Rules

- Keep the main code path in `src/zc.c` simple and readable.
- Prefer small helper functions over framework-like layers.
- Preserve the host-filesystem-first runtime contract unless a change explicitly expands it.
- Do not edit vendored `kilo` broadly.
  Only make minimal, clearly justified changes such as:
  - read-only mode support
  - small portability fixes
  - integration points needed by `zc`

## License Guardrail

- `zc` is intended to stay under BSD 2-Clause, matching the license family used by `kilo`.
- Do not copy GPL-only code from `mc` into `zc` unless you are intentionally changing the licensing story for `zc`.
- Keep the local project license in [LICENSE](/Users/azaia/Git/mc/LICENSE) and the vendored upstream license in [third_party/kilo/LICENSE](/Users/azaia/Git/mc/third_party/kilo/LICENSE).

## Keymap Policy

- File-manager actions should prefer `mc`-style function keys where there is a clear equivalent.
- Text-oriented convenience shortcuts may follow `kilo` conventions when they do not conflict badly.
- Keep help text, README, and runtime footer consistent with the implemented keymap.
- Keep archive support command-oriented: pack/unpack actions are fine, archive browsing is out of scope.
- Keep native `.zcc` container behavior distinct from helper-backed generic archives.
- Keep encrypted `.zcc` behavior aligned with the same extract-first model as plain `.zcc`.

## Validation

After changes, prefer validating at least:

- `make`
- `make bundle`
- `./zc --version`
- terminal-environment failure messaging if you changed startup behavior
- a short PTY smoke test for any changed keybinding or prompt flow

If you change bundled editor behavior, also test `./zc-kilo` directly.
