# zero-commander (`zc`)

`zc`, short for zero-commander, is a USB-first portable two-panel file commander. The intended distribution is a small bundle that can live on a removable drive, launch from that directory, and operate on the host machine's local filesystem without installation.

## Status

As of Saturday, August 8, 2026, `zc` is a usable local prototype with the USB-first runtime model documented and partially enforced in code:

- core filesystem work is local-host only
- bundled `zc-kilo` remains the default editor/viewer handoff
- optional pack/unpack helpers now resolve from the bundle `tools/` directory before `PATH`

Current implementation status is still narrower than the product direction:

- supported today: POSIX-style terminals on macOS/Linux
- target direction: macOS, Linux, and Windows portable bundles
- Windows console support and validation are not complete yet

## Runtime Contract

- `zc` always acts on the host filesystem.
- `zc` does not create a private environment or sandbox.
- `zc` uses the host terminal and the host machine's filesystem permissions.
- Core file work must function when the whole bundle is moved to another directory.
- Core behavior does not depend on host `PATH`.
- Optional archive helpers degrade with explicit status messages when unavailable.

## Core Features

- Two local panels
- Arrow keys, `Tab`, `Enter`, `Backspace`
- `F1` help
- `F3` view via bundled `zc-kilo --readonly`
- `F4` edit via bundled `zc-kilo`
- `F5` copy
- `F6` move
- `F7` create directory
- `F8` delete after confirmation
- `Ctrl-N` create empty file
- `n` rename a single item
- `Space` toggle mark and advance
- `*` mark or unmark all regular entries
- `F10` or `Ctrl-Q` quit

Optional archive actions remain command-oriented only:

- `F2` / `Ctrl-P` pack the current item or marked set
- `Ctrl-U` unpack a selected archive into a chosen directory
- archive browsing is out of scope

## Stable Bundle Layout

```text
zc-bundle/
  zc
  zc-kilo
  tools/
```

Cosmopolitan layout:

```text
zc-cosmo-bundle/
  zc.com
  zc-kilo.com
  tools/
```

Rules:

- `zc` and `zc-kilo` are sibling executables in the bundle root.
- Optional helper executables such as `bsdtar`, `zip`, `gzip`, `bzip2`, `xz`, or `zstd` belong in `tools/`.
- `zc` checks `tools/` before host `PATH` for optional helper resolution.
- Native builds may still use `ZC_KILO=/path/to/editor` to override the bundled editor.
- Cosmopolitan builds keep deterministic sibling lookup and do not fall back to `PATH` for the editor.

## Build

```sh
cd zc
make
make bundle
```

This produces:

- local binaries: `./zc`, `./zc-kilo`
- portable bundle layout: `./dist/zc-bundle/`

Cosmopolitan-oriented build:

```sh
cd zc
make cosmo COSMOCC=cosmocc
make bundle-cosmo COSMOCC=cosmocc
```

This is intended to produce:

- `./zc.com`
- `./zc-kilo.com`
- `./dist/zc-cosmo-bundle/`

## Run

```sh
./zc
./zc /tmp
./zc --left /tmp --right "$HOME"
```

Portable-bundle usage is expected to look like:

```sh
cd /Volumes/MyUSB/zc-bundle
./zc
```

## What Is Not Implemented

- archive browsing or archive-as-directory navigation
- subshells or remote filesystems
- background jobs
- plugin-style extension mechanisms
- a separate internal editor/viewer distinct from `zc-kilo`
- validated Windows console support

## License

`zc` uses the same BSD 2-Clause license family as `kilo`.

- Project license: [LICENSE](/Users/azaia/Git/mc/zc/LICENSE)
- Vendored editor license: [third_party/kilo/LICENSE](/Users/azaia/Git/mc/zc/third_party/kilo/LICENSE)

## More Docs

- [DOCUMENTATION.md](/Users/azaia/Git/mc/zc/DOCUMENTATION.md) covers the runtime contract, bundle layout, host expectations, and validation matrix.
- [AGENTS.md](/Users/azaia/Git/mc/zc/AGENTS.md) covers maintenance conventions inside `zc/`.
