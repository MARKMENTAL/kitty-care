"""Tests for the kitty-care script."""

import contextlib
import importlib.machinery
import importlib.util
import io
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "kitty-care"


def load_kitty_care():
    loader = importlib.machinery.SourceFileLoader("kitty_care", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


kc = load_kitty_care()


def quiet(function, *args, **kwargs):
    with contextlib.redirect_stdout(io.StringIO()):
        return function(*args, **kwargs)


class EnvironmentTest(unittest.TestCase):
    """Give each test an isolated HOME and config environment."""

    def setUp(self):
        self._saved_env = dict(os.environ)
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.home = self._tmp.name
        os.environ["HOME"] = self.home
        os.environ.pop("XDG_CONFIG_HOME", None)
        os.environ.pop("KITTY_CONFIG_DIRECTORY", None)
        os.environ.pop("KITTY_CARE_THEMES", None)
        os.environ.pop("KITTY_CARE_PALETTES", None)
        kc._fonts = None

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self._saved_env)

    def make_settings(self, theme="Dracula", font="Test Mono"):
        settings = kc.Settings()
        settings.theme = kc.THEME_NAMES.index(theme)
        settings.font = font
        return settings

    def config_path(self):
        path = kc.config_path()
        os.makedirs(os.path.dirname(path), exist_ok=True)
        return Path(path)


class TableTests(unittest.TestCase):
    def test_counts(self):
        self.assertEqual(len(kc.THEMES), 38)
        self.assertEqual(len(kc.PALETTES), 14)

    def test_theme_fg_bg_pairs_are_unique(self):
        pairs = [(fg.lower(), bg.lower())
                 for fg, bg, _, _ in kc.THEMES.values()]
        self.assertEqual(len(pairs), len(set(pairs)))

    def test_color_shapes(self):
        for name, (fg, bg, normal, bright) in kc.THEMES.items():
            self.assertEqual(len(normal), 8, name)
            self.assertEqual(len(bright), 8, name)
            for color in (fg, bg) + normal + bright:
                self.assertRegex(color, r"^#[0-9a-f]{6}$", name)
        for name, (normal, bright) in kc.PALETTES.items():
            self.assertEqual(len(normal), 8, name)
            self.assertEqual(len(bright), 8, name)


VALID_THEME = """\
["Test Theme"]
foreground = "#AABBCC"
background = "#112233"
normal = ["#000000", "#AA0000", "#00AA00", "#AAAA00", "#0000AA", "#AA00AA", "#00AAAA", "#AAAAAA"]
bright = ["#555555", "#FF5555", "#55FF55", "#FFFF55", "#5555FF", "#FF55FF", "#55FFFF", "#FFFFFF"]
"""

VALID_PALETTE = """\
["Test Palette"]
normal = ["#000000", "#AA0000", "#00AA00", "#AAAA00", "#0000AA", "#AA00AA", "#00AAAA", "#AAAAAA"]
bright = ["#555555", "#FF5555", "#55FF55", "#FFFF55", "#5555FF", "#FF55FF", "#55FFFF", "#FFFFFF"]
"""


class ThemeFileTests(unittest.TestCase):
    def test_valid_theme_is_lowercased_and_returns_tuples(self):
        themes = kc.parse_themes(VALID_THEME)
        self.assertEqual(list(themes), ["Test Theme"])
        fg, bg, normal, bright = themes["Test Theme"]
        self.assertEqual((fg, bg), ("#aabbcc", "#112233"))
        self.assertEqual(normal[0], "#000000")
        self.assertEqual(normal[1], "#aa0000")
        self.assertEqual(bright[7], "#ffffff")
        self.assertIsInstance(normal, tuple)
        self.assertIsInstance(bright, tuple)

    def test_invalid_toml_reports_source(self):
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_themes("not toml =", "bad.toml")
        self.assertIn("bad.toml", str(caught.exception))
        self.assertIn("invalid TOML", str(caught.exception))

    def test_wrong_color_count_is_rejected(self):
        text = VALID_THEME.replace('"#FFFFFF"]', '"#FFFFFF", "#000000"]')
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_themes(text)
        self.assertIn("8 colors", str(caught.exception))

    def test_invalid_hex_is_rejected(self):
        text = VALID_THEME.replace('"#55FFFF"', '"#55FFF"')
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_themes(text)
        self.assertIn("invalid color", str(caught.exception))

    def test_missing_foreground_is_rejected(self):
        text = VALID_THEME.replace('foreground = "#AABBCC"\n', "")
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_themes(text)
        self.assertIn("foreground", str(caught.exception))

    def test_theme_file_must_define_a_theme(self):
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_themes("")
        self.assertIn("no themes defined", str(caught.exception))


class PaletteFileTests(unittest.TestCase):
    def test_valid_palette_returns_tuples(self):
        palettes = kc.parse_palettes(VALID_PALETTE)
        self.assertEqual(list(palettes), ["Test Palette"])
        normal, bright = palettes["Test Palette"]
        self.assertEqual(len(normal), 8)
        self.assertEqual(len(bright), 8)
        self.assertEqual(normal[1], "#aa0000")
        self.assertIsInstance(normal, tuple)
        self.assertIsInstance(bright, tuple)

    def test_empty_palette_file_is_allowed(self):
        self.assertEqual(kc.parse_palettes(""), {})

    def test_wrong_color_count_is_rejected(self):
        text = VALID_PALETTE.replace('"#FFFFFF"]', '"#FFFFFF", "#000000"]')
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_palettes(text)
        self.assertIn("8 colors", str(caught.exception))

    def test_invalid_hex_is_rejected(self):
        text = VALID_PALETTE.replace('"#55FFFF"', '"#55FFF"')
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.parse_palettes(text)
        self.assertIn("invalid color", str(caught.exception))


class DataFileTests(EnvironmentTest):
    def test_sibling_file_is_found_by_default(self):
        expected = os.path.join(str(SCRIPT.parent), kc.THEMES_FILE)
        self.assertEqual(
            kc.find_data_file(kc.THEMES_FILE, kc.THEMES_ENV), expected)

    def test_environment_override_wins(self):
        custom = Path(self.home) / "custom-themes.toml"
        custom.write_text(VALID_THEME)
        os.environ[kc.THEMES_ENV] = str(custom)
        self.assertEqual(
            kc.find_data_file(kc.THEMES_FILE, kc.THEMES_ENV), str(custom))

    def test_missing_file_lists_searched_paths(self):
        target = Path(self.home) / "missing.toml"
        os.environ[kc.THEMES_ENV] = str(target)
        with self.assertRaises(kc.ThemeFileError) as caught:
            kc.find_data_file(kc.THEMES_FILE, kc.THEMES_ENV)
        self.assertIn(str(target), str(caught.exception))

    def test_loaders_read_the_shipped_files(self):
        self.assertEqual(len(kc.load_themes()), 38)
        self.assertEqual(len(kc.load_palettes()), 14)


class PathTests(EnvironmentTest):
    def test_default_location(self):
        self.assertEqual(
            kc.config_path(), os.path.join(self.home, ".config", "kitty", "kitty.conf"))

    def test_xdg_config_home(self):
        os.environ["XDG_CONFIG_HOME"] = "/tmp/xdg"
        self.assertEqual(kc.config_path(), "/tmp/xdg/kitty/kitty.conf")

    def test_kitty_config_directory_wins(self):
        os.environ["XDG_CONFIG_HOME"] = "/tmp/xdg"
        os.environ["KITTY_CONFIG_DIRECTORY"] = "/tmp/kitty-dir"
        self.assertEqual(kc.config_path(), "/tmp/kitty-dir/kitty.conf")


class RenderTests(unittest.TestCase):
    def setUp(self):
        self.settings = kc.Settings()
        self.settings.theme = kc.THEME_NAMES.index("Dracula")
        self.settings.font = "Test Mono"
        self.settings.size = 13
        self.settings.opacity = 0.9

    def test_empty_config(self):
        text = kc.render(None, self.settings)
        self.assertTrue(text.startswith("# Generated by kitty-care\n"))
        self.assertTrue(text.endswith("\n"))

    def test_unmanaged_lines_are_preserved(self):
        existing = (
            "# my precious config\n"
            "include ~/.config/kitty/local.conf\n"
            "map ctrl+c copy_and_clear_or_interrupt\n"
            "shell_integration enabled\n"
            "font_family Old Font\n"
            "font_size 99\n"
            "background #000000\n"
            "color3 #111111\n"
        )
        text = kc.render(existing, self.settings)
        self.assertIn("# my precious config", text)
        self.assertIn("include ~/.config/kitty/local.conf", text)
        self.assertIn("map ctrl+c copy_and_clear_or_interrupt", text)
        self.assertIn("shell_integration enabled", text)
        self.assertNotIn("Old Font", text)
        self.assertNotIn("font_size 99", text)
        self.assertNotIn("color3 #111111", text)
        self.assertEqual(text.count("font_family"), 1)
        self.assertEqual(text.count("font_size"), 1)
        self.assertEqual(text.count("\nfont_size "), 1)

    def test_generated_values(self):
        fg, bg, normal, bright = kc.THEMES["Dracula"]
        text = kc.render(None, self.settings)
        self.assertIn("font_family Test Mono\n", text)
        self.assertIn("font_size 13\n", text)
        self.assertIn("background %s\n" % bg, text)
        self.assertIn("foreground %s\n" % fg, text)
        for index, color in enumerate(normal):
            self.assertIn("color%d %s\n" % (index, color), text)
        for index, color in enumerate(bright):
            self.assertIn("color%d %s\n" % (index + 8, color), text)
        self.assertIn("background_opacity 0.90\n", text)

    def test_stale_generator_headers_are_replaced(self):
        existing = (
            "# Generated by kitty-conf\n"
            "font_family Old Font\n"
            "\n"
            "# Generated by kitty-care\n"
            "font_size 10\n"
        )
        text = kc.render(existing, self.settings)
        self.assertEqual(text.count("Generated by"), 1)
        self.assertIn("# Generated by kitty-care\n", text)

    def test_palette_overrides_theme_colors(self):
        self.settings.theme = kc.THEME_NAMES.index("Dark")
        self.settings.palette = kc.PALETTE_NAMES.index("Nord")
        _, theme_bg, _, _ = kc.THEMES["Dark"]
        normal, bright = kc.PALETTES["Nord"]
        text = kc.render(None, self.settings)
        self.assertIn("background %s\n" % theme_bg, text)
        self.assertIn("color0 %s\n" % normal[0], text)
        self.assertIn("color15 %s\n" % bright[7], text)


class FontTests(unittest.TestCase):
    def test_unique_families_are_deduplicated_and_sorted(self):
        output = (
            "JetBrains Mono, JetBrains Mono NL\n"
            "JetBrains Mono\n"
            "Fira Code\n"
            "\n"
            "DejaVu Sans Mono, DejaVu Sans Mono Bold\n"
        )
        self.assertEqual(
            kc.unique_families(output),
            ["DejaVu Sans Mono", "Fira Code", "JetBrains Mono"])


class SwatchTests(unittest.TestCase):
    def setUp(self):
        self._saved = kc.COLOR_SUPPORTED

    def tearDown(self):
        kc.COLOR_SUPPORTED = self._saved

    def test_truecolor_escape(self):
        kc.COLOR_SUPPORTED = True
        rendered = kc.swatches(("#ff0000", "#00ff00"))
        self.assertIn("\x1b[48;2;255;0;0m ", rendered)
        self.assertIn("\x1b[48;2;0;255;0m ", rendered)
        self.assertTrue(rendered.endswith("\x1b[0m"))

    def test_plain_output_without_a_tty(self):
        kc.COLOR_SUPPORTED = False
        self.assertEqual(kc.swatches(("#ff0000",)), "")


class LoadTests(EnvironmentTest):
    def test_missing_config_uses_defaults(self):
        settings = quiet(kc.load_settings, kc.config_path())
        self.assertEqual(settings.theme, -1)
        self.assertEqual(settings.palette, -1)
        self.assertEqual(settings.font, "")
        self.assertEqual(settings.size, kc.DEFAULT_SIZE)
        self.assertEqual(settings.opacity, kc.DEFAULT_OPACITY)

    def test_round_trip(self):
        settings = self.make_settings(theme="Nord", font="Iosevka")
        settings.palette = kc.PALETTE_NAMES.index("Tango")
        settings.size = 15
        settings.opacity = 0.85
        self.assertTrue(quiet(kc.write_config, settings))

        loaded = quiet(kc.load_settings, kc.config_path())
        self.assertEqual(loaded.theme, kc.THEME_NAMES.index("Nord"))
        self.assertEqual(loaded.palette, kc.PALETTE_NAMES.index("Tango"))
        self.assertEqual(loaded.font, "Iosevka")
        self.assertEqual(loaded.size, 15)
        self.assertAlmostEqual(loaded.opacity, 0.85)

    def test_theme_default_ansi_round_trips(self):
        settings = self.make_settings(theme="Dark", font="Test Mono")
        self.assertTrue(quiet(kc.write_config, settings))
        loaded = quiet(kc.load_settings, kc.config_path())
        self.assertEqual(loaded.theme, kc.THEME_NAMES.index("Dark"))


class WriteTests(EnvironmentTest):
    def test_requires_theme_and_font(self):
        settings = kc.Settings()
        self.assertFalse(quiet(kc.write_config, settings))
        self.assertFalse(os.path.exists(kc.config_path()))

        settings.theme = 0
        self.assertFalse(quiet(kc.write_config, settings))
        self.assertFalse(os.path.exists(kc.config_path()))

    def test_backup_and_merge(self):
        path = self.config_path()
        original = "map ctrl+c copy\nfont_family Old Font\n"
        path.write_text(original)
        before = stat.S_IMODE(os.stat(str(path)).st_mode)

        self.assertTrue(quiet(kc.write_config, self.make_settings()))

        backups = sorted(path.parent.glob("kitty.conf.*.bak"))
        self.assertEqual(len(backups), 1)
        self.assertEqual(backups[0].read_text(), original)

        text = path.read_text()
        self.assertIn("map ctrl+c copy", text)
        self.assertNotIn("Old Font", text)
        self.assertEqual(text.count("font_family"), 1)
        self.assertEqual(stat.S_IMODE(os.stat(str(path)).st_mode), before)

    def test_backup_names_are_unique(self):
        path = self.config_path()
        path.write_text("font_family Old Font\n")
        self.assertTrue(quiet(kc.write_config, self.make_settings()))
        self.assertTrue(quiet(kc.write_config, self.make_settings()))
        backups = list(path.parent.glob("kitty.conf.*.bak"))
        self.assertEqual(len(backups), 2)
        self.assertEqual(len({backup.name for backup in backups}), 2)

    def test_symlinked_config_is_preserved(self):
        path = self.config_path()
        real = Path(self.home) / "dotfiles-kitty.conf"
        real.write_text("font_family Old Font\n")
        path.symlink_to(real)

        self.assertTrue(quiet(kc.write_config, self.make_settings()))

        self.assertTrue(path.is_symlink())
        self.assertEqual(path.resolve(), real.resolve())
        self.assertIn("font_family Test Mono", real.read_text())
        self.assertNotIn("Old Font", real.read_text())
        self.assertEqual(len(list(real.parent.glob("dotfiles-kitty.conf.*.bak"))), 1)


class CliTests(EnvironmentTest):
    def run_cli(self, *arguments, stdin=""):
        env = dict(os.environ)
        return subprocess.run(
            [sys.executable, str(SCRIPT), *arguments],
            input=stdin,
            text=True,
            capture_output=True,
            env=env,
        )

    def test_help(self):
        result = self.run_cli("--help")
        self.assertEqual(result.returncode, 0)
        self.assertIn("Usage: kitty-care", result.stdout)
        self.assertIn("38 color themes", result.stdout)
        self.assertIn("14 ANSI color palettes", result.stdout)

    def test_unknown_argument(self):
        result = self.run_cli("--bogus")
        self.assertEqual(result.returncode, 2)
        self.assertIn("unknown argument", result.stderr)

    def test_missing_theme_file_is_reported(self):
        os.environ["KITTY_CARE_THEMES"] = str(Path(self.home) / "missing.toml")
        result = self.run_cli(stdin="")
        self.assertEqual(result.returncode, 1)
        self.assertIn("could not find themes.toml", result.stderr)

    def test_missing_palette_file_is_reported(self):
        os.environ["KITTY_CARE_PALETTES"] = str(Path(self.home) / "missing.toml")
        result = self.run_cli(stdin="")
        self.assertEqual(result.returncode, 1)
        self.assertIn("could not find ansi_palettes.toml", result.stderr)

    def test_help_works_without_data_files(self):
        os.environ["KITTY_CARE_THEMES"] = str(Path(self.home) / "missing.toml")
        result = self.run_cli("--help")
        self.assertEqual(result.returncode, 0)
        self.assertIn("Usage: kitty-care", result.stdout)

    def test_exit_from_menu(self):
        result = self.run_cli(stdin="8\n")
        self.assertEqual(result.returncode, 0)
        self.assertIn("Sayonara", result.stdout)

    def test_eof_exits_cleanly(self):
        result = self.run_cli(stdin="")
        self.assertEqual(result.returncode, 0)

    def test_interactive_apply_merges_config(self):
        path = self.config_path()
        path.write_text("map ctrl+c copy\nfont_family Test Mono\n")
        result = self.run_cli(stdin="1\n2\n6\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        text = path.read_text()
        self.assertIn("map ctrl+c copy", text)
        self.assertIn("font_family Test Mono", text)
        fg, bg, _, _ = kc.THEMES["Light"]
        self.assertIn("background %s" % bg, text)
        self.assertIn("foreground %s" % fg, text)
        self.assertEqual(len(list(path.parent.glob("kitty.conf.*.bak"))), 1)


if __name__ == "__main__":
    unittest.main()
