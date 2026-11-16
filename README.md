# kitty-care

Interactive theme and configuration generator for the
[kitty](https://sw.kovidgoyal.net/kitty/) terminal.

`kitty-care` walks you through choosing a color theme, an ANSI palette, a
monospace font, the font size, and the background opacity, then writes
kitty's config file. It does not overwrite settings it does not manage:
`map`, `include`, `shell_integration`, comments, and any other keys you have
added are kept as they are.

## Requirements

- Python 3.8 or newer (standard library only)
- `fc-list` from fontconfig, for browsing installed monospace fonts

## Install

```sh
mkdir -p ~/.local/bin
install -m755 kitty-care ~/.local/bin/kitty-care
```

or:

```sh
make install          # installs to ~/.local/bin by default
make install PREFIX=/usr/local
```

## Usage

```sh
kitty-care            # open the interactive menu
kitty-care --help
```

The config is written to the first of:

1. `$KITTY_CONFIG_DIRECTORY/kitty.conf`
2. `${XDG_CONFIG_HOME:-~/.config}/kitty/kitty.conf`

## What gets written

- 38 color themes and 14 ANSI palettes, selectable independently
- `font_family` (kitty resolves the bold and italic faces itself)
- `font_size`, `background`, `foreground`, `color0`–`color15`,
  `background_opacity`

Before every write the previous file is copied to
`kitty.conf.<timestamp>.bak` next to the original. Backups are never pruned;
remove old ones yourself if they pile up.

Reload kitty with `ctrl+shift+f5` (or restart it) to see the changes.

## Development

```sh
make check            # byte-compile the script and tests
make test             # run the unittest suite
```

The tests use temporary `HOME` directories and never touch your real kitty
config.
