# kitty-care

Interactive theme and configuration generator for the
[kitty](https://sw.kovidgoyal.net/kitty/) terminal.

`kitty-care` walks you through choosing a color theme, an ANSI palette, a
monospace font, the font size, and the background opacity, then writes
kitty's config file. It does not overwrite settings it does not manage:
`map`, `include`, `shell_integration`, comments, and any other keys you have
added are kept as they are.

## Requirements

- Python 3.11 or newer (`tomllib` is used to read the color files)
- `fc-list` from fontconfig, for browsing installed monospace fonts

## Install

```sh
mkdir -p ~/.local/bin ~/.local/share/kitty-care
install -m755 kitty-care ~/.local/bin/kitty-care
install -m644 themes.toml ansi_palettes.toml ~/.local/share/kitty-care/
```

or:

```sh
make install          # installs to ~/.local by default
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

## Themes and palettes

Theme and ANSI palette definitions live outside the script:

- `themes.toml` — one table per theme, with `foreground`, `background`,
  `normal`, and `bright`
- `ansi_palettes.toml` — one table per palette, with `normal` and `bright`

`normal` and `bright` each hold exactly eight `#rrggbb` strings (ANSI
colors 0–7 and 8–15). The order of tables in each file is the order shown
in the menus.

Each file is looked up in:

1. `$KITTY_CARE_THEMES` / `$KITTY_CARE_PALETTES`, if set
2. the directory containing the `kitty-care` script
3. `${XDG_DATA_HOME:-~/.local/share}/kitty-care/`
4. `/usr/local/share/kitty-care/`
5. `/usr/share/kitty-care/`

```toml
["My Theme"]
foreground = "#c0caf5"
background = "#1a1b26"
normal = ["#15161e", "#f7768e", "#9ece6a", "#e0af68",
          "#7aa2f7", "#bb9af7", "#7dcfff", "#a9b1d6"]
bright = ["#414868", "#f7768e", "#9ece6a", "#e0af68",
          "#7aa2f7", "#bb9af7", "#7dcfff", "#c0caf5"]
```

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
