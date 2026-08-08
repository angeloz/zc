# zero-commander (`zc`) Documentation

## Overview

`zc`, short for zero-commander, is a compact two-panel file manager developed inside this repository as a new project, not as a port of `mc`. Its goal is to stay small, local, and portable while reusing a bundled `kilo`-derived editor binary for edit and view actions.

As of Saturday, August 8, 2026, the implemented scope is intentionally narrow:

- local filesystem only
- ANSI/VT-style terminal UI
- two panels
- copy, move, delete, rename, mkdir, new empty file
- bundled `zc-kilo` for edit and read-only view

## Runtime model

`zc` is currently built around a single C translation unit:

- [src/zc.c](/Users/azaia/Git/mc/zc/src/zc.c)

The file contains:

- terminal raw-mode handling
- keyboard decoding for arrows and function keys
- panel state and entry lists
- ANSI rendering
- prompt and confirmation flows
- local filesystem operations
- process spawning for `zc-kilo`

This is deliberate. The code should remain easy to trace without needing a large internal framework.

## Bundled editor integration

`zc` builds a sibling executable named `zc-kilo` from vendored upstream source in:

- [third_party/kilo](/Users/azaia/Git/mc/zc/third_party/kilo)

Current local integration changes are minimal:

- support `--readonly`
- minor build cleanup for vendored use

At runtime, `zc` uses a sibling bundled editor by default.

- native/local build: sibling `zc-kilo`, with optional `ZC_KILO` override
- Cosmopolitan build: sibling `zc-kilo.com`, with no fallback to `PATH`

If the required sibling bundled editor is missing in Cosmopolitan mode, `zc` should fail clearly instead of silently choosing a different editor.

## License

`zc` is intended to use the same BSD 2-Clause license family as `kilo`.

- local project license: [LICENSE](/Users/azaia/Git/mc/zc/LICENSE)
- vendored upstream license: [third_party/kilo/LICENSE](/Users/azaia/Git/mc/zc/third_party/kilo/LICENSE)

This matters for future development: copying GPL-only implementation code from `mc` into `zc` would change the licensing constraints.

## Keymap

### File-manager oriented bindings

- `F1`: help
- `F3`: view
- `F4`: edit
- `F5`: copy
- `F6`: move
- `F7`: mkdir
- `F8`: delete
- `F10`: quit

### Additional bindings

- `Ctrl-N`: create empty file
- `Ctrl-Q`: quit
- `Tab`: switch panel
- `Space`: toggle mark on current entry
- `*`: toggle mark-all in active panel
- `n`: rename one item
- `r`: refresh
- `Enter`: open directory or file
- `Backspace` / `-`: go to parent directory

### Compatibility note

The project is trying to balance:

- `mc`-style function keys for commander actions
- `kilo`-style control bindings where they are ergonomic and low-conflict

## Multi-selection behavior

Selections are mark-based, not range-based.

- If one or more entries are marked, bulk operations use the marked set.
- If nothing is marked, operations act on the current entry.
- Rename remains a single-item action.

Current bulk-aware actions:

- copy
- move
- delete

## Current limitations

- no `mc`-style menu bar or user menu
- no `F2` action beyond reserved future compatibility
- no `F9` menu implementation
- no remote filesystems
- no archive browsing
- no background jobs
- no native internal viewer/editor separate from `zc-kilo`
- Cosmopolitan validation still depends on having `cosmocc` available in the build environment

## Build and distribution

Local build:

```sh
cd zc
make
```

This produces:

- `zc`
- `zc-kilo`

Experimental Cosmopolitan-oriented build:

```sh
cd zc
make cosmo COSMOCC=cosmocc
```

This build path is intended to produce:

- `zc.com`
- `zc-kilo.com`

The expected deployment model is a two-file bundle in one directory, with `zc.com` launching the sibling `zc-kilo.com`.

## Suggested next steps

- decide what `F2` should become, if anything
- decide whether `F9` should remain unimplemented or gain a minimal menu
- validate the two-file Cosmopolitan bundle on actual target systems once `cosmocc` is available
- consider a clearer status line for marked-count and current operation results
