# zero-commander (`zc`)

`zc`, short for zero-commander, is a small two-panel file manager built from scratch with a `kilo`-style bias toward low complexity.

## Goals

- Keep the codebase small and dependency-light.
- Work as a local filesystem commander.
- Build and ship a bundled `kilo` from source for editing and read-only viewing.
- Stay friendly to Cosmopolitan-style builds by avoiding heavy libraries.

## Status

`zc` is currently a usable local two-panel prototype. It is intentionally narrower than `mc`: no subshell, no VFS, no plugins, no remote filesystems, and no internal editor/viewer beyond the bundled `zc-kilo`.

## Current behavior

- Two local panels.
- Arrow keys, `Tab`, `Enter`, `Backspace`.
- `F1` shows a compact help screen.
- `Space` toggles mark on the current entry and advances the cursor.
- `*` marks or unmarks all regular entries in the active panel.
- `Ctrl-N` creates a new empty file in the active panel.
- `F3` view via bundled `zc-kilo --readonly`.
- `F4` edit via bundled `zc-kilo`.
- `F5` copy to the opposite panel.
- `F6` move to a prompted destination.
- `F7` create a directory.
- `F8` delete after confirmation.
- `F10` or `Ctrl-Q` quits `zc`.
- `n` rename a single item in place inside the active panel.
- `r` refresh, `q` quit.

When one or more entries are marked, copy, move, and delete operate on the full marked set. Without marks, they operate on the current entry only.

`zc` prefers a sibling bundled editor built from the vendored upstream `antirez/kilo` source in `third_party/kilo/`.

- native/local build: sibling `zc-kilo`
- Cosmopolitan build: sibling `zc-kilo.com`

For the native build you can still override the editor with `ZC_KILO=/path/to/editor`. In Cosmopolitan mode the runtime expects the sibling bundled editor and does not fall back to `PATH`.

## License

`zc` uses the same BSD 2-Clause license family as `kilo`.

- Project license: [LICENSE](/Users/azaia/Git/mc/zc/LICENSE)
- Vendored editor license: [third_party/kilo/LICENSE](/Users/azaia/Git/mc/zc/third_party/kilo/LICENSE)

## Keymap

- `F1`: help
- `F3`: view selected file with `zc-kilo --readonly`
- `F4`: edit selected file with `zc-kilo`
- `F5`: copy selected item or marked set
- `F6`: move selected item or marked set
- `F7`: create directory
- `F8`: delete selected item or marked set
- `F10`: quit
- `Ctrl-N`: create empty file
- `Ctrl-Q`: quit
- `Tab`: switch active panel
- `Space`: mark/unmark current entry
- `*`: mark/unmark all entries in active panel
- `n`: rename a single item
- `r`: refresh

## What Is Not Implemented

- `mc`-style top menu or user menu on `F2` / `F9`
- remote or archive filesystems
- background jobs
- integrated file creation via editor templates
- a native internal editor or viewer distinct from `zc-kilo`

## Build

```sh
cd zc
make
```

This builds:

- `./zc`
- `./zc-kilo`

To try a Cosmopolitan build:

```sh
cd zc
make cosmo COSMOCC=cosmocc
```

This is intended to build:

- `./zc.com`
- `./zc-kilo.com`

The Cosmopolitan distribution is a two-file bundle. `zc.com` expects `zc-kilo.com` in the same directory.

## Run

```sh
./zc
./zc /tmp
./zc --left /tmp --right "$HOME"
```

Set `ZC_KILO` if you want to override the bundled editor in the native build:

```sh
ZC_KILO=/path/to/kilo ./zc
```

## More Docs

- See [DOCUMENTATION.md](/Users/azaia/Git/mc/zc/DOCUMENTATION.md) for architecture, current limits, and behavior notes.
- See [AGENTS.md](/Users/azaia/Git/mc/zc/AGENTS.md) for maintenance conventions inside `zc/`.
