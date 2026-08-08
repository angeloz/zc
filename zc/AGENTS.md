# AGENTS

This file describes how to work on `zc/` safely and consistently.

## Project intent

- `zc` is a lightweight, local-only two-panel file manager.
- The design target is closer to `kilo` than to full `mc`.
- Prefer small code, direct control flow, and limited features over broad abstraction.

## Source layout

- [src/zc.c](/Users/azaia/Git/mc/zc/src/zc.c): main application, TUI, filesystem actions, launcher logic.
- [Makefile](/Users/azaia/Git/mc/zc/Makefile): local build plus vendored `zc-kilo` build.
- [third_party/kilo](/Users/azaia/Git/mc/zc/third_party/kilo): vendored upstream `kilo` source with minimal local patches.

## Build expectations

- `make` must build both `zc` and `zc-kilo`.
- `make cosmo` is intended to build both `zc.com` and `zc-kilo.com`.
- Avoid introducing dependencies on `glib`, `ncurses`, `slang`, or other heavy runtime libraries.

## License guardrail

- `zc` is intended to stay under BSD 2-Clause, matching the license family used by `kilo`.
- Do not copy GPL-only code from `mc` into `zc` unless you are intentionally changing the licensing story for `zc`.
- Keep the local project license in [LICENSE](/Users/azaia/Git/mc/zc/LICENSE) and the vendored upstream license in [third_party/kilo/LICENSE](/Users/azaia/Git/mc/zc/third_party/kilo/LICENSE).

## Editing rules

- Keep the main code path in `src/zc.c` simple and readable.
- Prefer adding small helper functions over introducing framework-like layers.
- Preserve the current local-filesystem-only scope unless a change explicitly expands it.
- Do not edit vendored `kilo` broadly.
  Only make minimal, clearly justified changes such as:
  - read-only mode support
  - small portability fixes
  - integration points needed by `zc`

## Keymap policy

- File-manager actions should prefer `mc`-style function keys where there is a clear equivalent.
- Text-oriented convenience shortcuts may follow `kilo` conventions when they do not conflict badly.
- Keep help text, README, and runtime footer consistent with the implemented keymap.

## Current scope limits

- No subshell.
- No remote VFS.
- No archive browsing.
- No menu bar or user menu on `F2` / `F9`.
- No separate internal editor/viewer beyond launching `zc-kilo`.

## Cosmopolitan bundle policy

- Treat the Cosmopolitan distribution as a two-file bundle:
  - `zc.com`
  - `zc-kilo.com`
- In Cosmopolitan mode, keep editor lookup deterministic: use the sibling `zc-kilo.com`.
- Do not add a silent PATH fallback for the Cosmopolitan runtime path.

## Validation

After changes, prefer validating at least:

- `make`
- `./zc --version`
- a short PTY smoke test for any changed keybinding or prompt flow

If you change bundled editor behavior, also test `./zc-kilo` directly.
