# zero-commander (`zc`) Documentation

## Overview

`zc`, short for zero-commander, is a compact two-panel file manager developed inside this repository as a new project, not as a port of `mc`. The product direction is now USB-first: the distribution unit is a portable bundle that can live on a removable drive, launch on the host machine, and work directly on the host filesystem without installation.

As of Saturday, August 8, 2026, the runtime model is clearer than the portability matrix:

- implemented today: local-host filesystem operations, bundled editor handoff, bundle-relative optional helper lookup
- intended direction: portable bundles for macOS, Linux, and Windows
- not complete yet: Windows console/runtime support and cross-host validation

## Runtime Contract

The contract for `zc` is now:

- `zc` always acts on the host filesystem.
- `zc` never creates an isolated runtime environment.
- `zc` uses the host terminal and host filesystem permissions.
- `zc` is expected to launch from an arbitrary directory, including a removable-drive path.
- Core filesystem actions must remain usable even when the whole bundle is moved.
- Core behavior must not rely on host `PATH`.
- Optional helper-backed features must fail clearly instead of failing silently.

This keeps the product focused on predictable filesystem work on a foreign machine instead of trying to ship a private environment.

## Current Source Layout

`zc` is still intentionally small and centered on one main translation unit:

- [src/zc.c](/Users/azaia/Git/mc/src/zc.c)

That file currently contains:

- terminal raw-mode handling
- keyboard decoding
- panel state and entry lists
- ANSI rendering
- prompt and confirmation flows
- local filesystem operations
- bundle-relative editor/helper discovery
- process spawning for `zc-kilo` and optional archive tools

The code should remain easy to trace without a large internal framework.

## Stable Bundle Layout

Native bundle:

```text
zc-bundle/
  zc
  zc-kilo
  tools/
```

Cosmopolitan-oriented bundle:

```text
zc-cosmo-bundle/
  zc.com
  zc-kilo.com
  tools/
```

Bundle rules:

- `zc` and `zc-kilo` live in the bundle root as siblings.
- Optional helper executables live in `tools/`.
- `tools/` is part of the stable public layout even when empty.
- `zc` resolves optional helpers from `tools/` before consulting host `PATH`.
- Editor lookup is sibling-relative first.
- Cosmopolitan editor lookup stays deterministic and does not fall back to `PATH`.

At the moment, no archive helpers are bundled by default; the layout exists so a portable release can add them without changing runtime lookup rules.

## Bundled Editor Integration

`zc` builds a sibling executable named `zc-kilo` from vendored upstream source in:

- [third_party/kilo](/Users/azaia/Git/mc/third_party/kilo)

Runtime behavior:

- native/local build: sibling `zc-kilo`, with optional `ZC_KILO` override
- Cosmopolitan build: sibling `zc-kilo.com`, with no editor fallback to `PATH`

If the required sibling editor is missing in forced-bundle mode, `zc` reports that clearly instead of silently choosing another editor.

## Optional Helper Strategy

Archive support is deliberately secondary under the USB-first model.

Current policy:

- archive browsing remains out of scope
- pack/unpack may exist as explicit commands
- helper resolution is deterministic: bundle `tools/` first, host `PATH` second
- missing helpers produce explicit status messages that name the missing tool

This means archive support does not define the architecture. Core filesystem work does.

Helper names currently used by pack/unpack code paths:

- `zip`
- `bsdtar`
- `gzip`
- `bzip2`
- `xz`
- `zstd`

## Host Terminal Expectations

`zc` treats terminal capability as a real runtime dependency.

Current behavior:

- requires an interactive terminal on stdin/stdout
- rejects `TERM=dumb`
- uses raw keyboard input and ANSI/VT-style screen control
- reports a minimum terminal size requirement at runtime

Host matrix status as of August 8, 2026:

- macOS terminal environments: current implementation target
- Linux terminal environments: current implementation target
- Windows terminal/console environments: product target, but not yet validated or port-complete

The code still uses POSIX APIs such as `fork()`, `termios`, `isatty()`, and `exec*()`, so Windows support should not be described as done yet.

## Feature Priority Under USB-First Constraints

Primary workflows:

- browse directories
- copy and move files
- create, rename, and delete
- inspect and edit text files through bundled `zc-kilo`
- marked multi-selection workflows

Secondary workflows:

- explicit pack/unpack commands

Lower-priority or out-of-scope areas:

- archive browsing
- menu systems
- background jobs
- plugin-like extensibility
- features that introduce broad host runtime dependencies without a packaging plan

## Keymap

- `F1`: help
- `F2` / `Ctrl-P`: pack current item or marked set into an archive
- `F3`: view
- `F4`: edit
- `F5`: copy
- `F6`: move
- `F7`: mkdir
- `F8`: delete
- `F10`: quit
- `Ctrl-N`: create empty file
- `Ctrl-Q`: quit
- `Tab`: switch panel
- `Space`: toggle mark on current entry
- `*`: toggle mark-all in active panel
- `n`: rename one item
- `Ctrl-U`: unpack one archive into a chosen directory
- `r`: refresh
- `Enter`: open directory or file
- `Backspace` / `-`: go to parent directory

## Build And Packaging

Local build:

```sh
make
```

Portable native bundle:

```sh
make bundle
```

Cosmopolitan-oriented build:

```sh
make cosmo COSMOCC=cosmocc
make bundle-cosmo COSMOCC=cosmocc
```

The bundle targets currently create stable directories under `dist/` with an empty `tools/` directory ready for optional helper packaging.

## Validation Matrix

Bundle-level validation:

- build `dist/zc-bundle/` and run `zc` from that directory
- move the whole bundle directory and run it again
- verify sibling discovery of `zc-kilo`
- verify the bundle does not depend on the current working directory

Host filesystem validation:

- copy, move, delete, create, rename on host paths
- edit and view through bundled `zc-kilo`
- marked multi-selection flows

Dependency-isolation validation:

- verify core file operations with no reliance on host `PATH`
- verify archive helpers resolve from bundle `tools/` first
- verify missing optional helpers fail with explicit status messaging

Host-matrix validation:

- macOS terminal
- Linux terminal
- Windows terminal/console once a Windows-capable runtime exists

Cosmopolitan validation:

- `zc.com` and `zc-kilo.com` from the same bundle directory
- editor handoff through sibling lookup
- host filesystem operations from the `.com` build

## Current Limitations

- no archive browsing
- no remote filesystems
- no background jobs
- no menu bar or `F9` menu
- no native internal editor/viewer separate from `zc-kilo`
- no completed Windows support path yet
- Cosmopolitan validation still depends on having `cosmocc` available in the build environment

## License

`zc` is intended to use the same BSD 2-Clause license family as `kilo`.

- local project license: [LICENSE](/Users/azaia/Git/mc/LICENSE)
- vendored upstream license: [third_party/kilo/LICENSE](/Users/azaia/Git/mc/third_party/kilo/LICENSE)

This remains important: copying GPL-only implementation code from `mc` into `zc` would change the licensing constraints for `zc`.
