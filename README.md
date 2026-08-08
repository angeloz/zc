# zero-commander (`zc`)

`zc`, short for zero-commander, is a USB-first portable two-panel file commander. The intended distribution is a small bundle that can live on a removable drive, launch from that directory, and operate on the host machine's local filesystem without installation.

## Product Manifesto

`zc` is not positioned as "just another Norton Commander or `mc` clone".

It exists for a more operational use case:

- portable bundle first, install-less use second
- host-filesystem work on foreign machines
- direct handling of native plain or encrypted `.zcc` containers
- predictable archive, file, and backup workflows without depending on host crypto libraries

The goal is a small tool you can keep with you, launch almost anywhere, and use immediately on the machine in front of you.

That means:

- no install step as the primary model
- no assumption of a private runtime environment
- no dependence on host crypto stacks for secure container workflows
- no pressure to become a large compatibility clone full of legacy subsystems

In practice, `zc` is meant to be useful both as a local file commander and as a portable utility you can carry on a USB drive for file work, ad-hoc archives, and passphrase-protected backups.

## Why `zc`

- portable by design: move the whole bundle and run it again
- bundle-relative behavior: editor and optional helpers resolve from the bundle layout first
- native encrypted containers: `.zcc` can protect contents, names, paths, and metadata
- modern crypto baseline: encrypted `.zcc` uses vendored Monocypher with passphrase-based authenticated encryption
- focused scope: file work, text editing, packing/unpacking, and extract-first secure containers

## Typical Use Cases

- carry `zc` on a USB drive, plug into another machine, and work on that host filesystem without install steps
- collect files from a host machine into one portable `.zcc` container
- prepare an encrypted `.zcc` backup on removable storage protected by a passphrase
- move a small self-contained toolbox with `zc` + `zc-kilo` for browsing, editing, copying, and recovery work
- use `zc` locally as a compact two-panel commander without dragging in a larger runtime stack

## What Makes It Different

- classic two-panel interaction, but with a portable-bundle-first product model
- native `.zcc` containers, including encrypted containers, instead of only helper-backed archives
- modern passphrase-based protection for portable backups and archives
- deterministic sibling-relative behavior for editor handoff and optional helper lookup
- deliberate focus on field use, removable media, and host-machine operations

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
- `F9` create a native `zc` container (`.zcc`)
- `Ctrl-N` create empty file
- `n` rename a single item
- `Space` toggle mark and advance
- `*` mark or unmark all regular entries
- `F10` or `Ctrl-Q` quit

Optional archive actions remain command-oriented only:

- `F2` / `Ctrl-P` pack the current item or marked set
- `Ctrl-U` unpack a selected archive into a chosen directory
- archive browsing is out of scope

`zc` also has a native extract-first container flow:

- `F9` creates a `.zcc` container from the current item or marked selection
- `F9` lets you choose between a plain or encrypted `.zcc` container
- opening a `.zcc` file extracts it into a chosen host directory
- `.zcc` creation/extraction is implemented directly in `zc`, without external archive helpers
- encrypted `.zcc` containers require a passphrase on each extract/open operation
- encrypted `.zcc` containers hide file contents, names, paths, and metadata
- encrypted `.zcc` uses the vendored Monocypher backend instead of host crypto libraries
- extraction preserves the selected top-level names under the destination directory you choose

## Positioning

Compared with a classic file commander, the emphasis here is different:

- not a large compatibility clone with many legacy subsystems
- not archive browsing as virtual directories
- not plugin-first extensibility
- yes to portable bundle behavior, deterministic helper lookup, and secure container workflows
- yes to a small operational tool that can live on removable media and still work directly on the host machine

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
make
make bundle
```

This produces:

- local binaries: `./zc`, `./zc-kilo`
- portable bundle layout: `./dist/zc-bundle/`

Cosmopolitan-oriented build:

```sh
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
- in-place browsing inside `.zcc` containers
- validated Windows console support

## License

`zc` uses the same BSD 2-Clause license family as `kilo`.

- Project license: [LICENSE](/Users/azaia/Git/mc/LICENSE)
- Vendored editor license: [third_party/kilo/LICENSE](/Users/azaia/Git/mc/third_party/kilo/LICENSE)

## More Docs

- [DOCUMENTATION.md](/Users/azaia/Git/mc/DOCUMENTATION.md) covers the runtime contract, bundle layout, host expectations, and validation matrix.
- [AGENTS.md](/Users/azaia/Git/mc/AGENTS.md) covers maintenance conventions for this repository.
