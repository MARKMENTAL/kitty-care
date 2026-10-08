/*
 * kitty-conf.c — Kitty terminal theme & configuration generator
 *
 * Compiles with TCC in strict C89 mode:
 *   tcc -std=c89 -Wall -Wextra kitty-conf.c -o kitty-conf
 *   Or with a typical compiler like GCC
 *
 * Generates a config file for the kitty terminal emulator at
 * ~/.config/kitty/kitty.conf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */

#define MAX_FONTS       500
#define MAX_FONT_LEN    65
#define MAX_LINE_LEN    2048
#define MAX_THEMES      34
#define MAX_PALETTES    14
#define MAX_INPUT_LEN   256
#define TEMP_FILE       "/tmp/kitty_conf_fonts.txt"
#define CONFIG_PATH     ".config/kitty/kitty.conf"

#define MIN_FONT_SIZE   6
#define MAX_FONT_SIZE   72
#define DEFAULT_SIZE    12
#define DEFAULT_OPACITY 1

/* ------------------------------------------------------------------ */
/* Data structures                                                     */
/* ------------------------------------------------------------------ */

struct Theme {
    const char *name;
    const char *foreground;
    const char *background;
    const char *normal[8];
    const char *bright[8];
};

struct ANSIPalette {
    const char *name;
    const char *normal[8];
    const char *bright[8];
};

/* ------------------------------------------------------------------ */
/* Predefined themes                                                   */
/* ------------------------------------------------------------------ */

static struct Theme themes[MAX_THEMES] = {
    {
        "Dark",
        "#d0d0d0", "#1c1c1c",
        { "#1c1c1c", "#c50f1f", "#23a523", "#b58900", "#268bd2", "#d33682", "#2aa198", "#d0d0d0" },
        { "#666666", "#ff6666", "#66ff66", "#ffff66", "#6666ff", "#ff66ff", "#66ffff", "#ffffff" }
    },
    {
        "Light",
        "#1c1c1c", "#d0d0d0",
        { "#1c1c1c", "#c50f1f", "#23a523", "#b58900", "#268bd2", "#d33682", "#2aa198", "#d0d0d0" },
        { "#666666", "#ff6666", "#66ff66", "#ffff66", "#6666ff", "#ff66ff", "#66ffff", "#ffffff" }
    },
    {
        "Solarized Dark",
        "#839496", "#002b36",
        { "#073642", "#dc322f", "#859900", "#b58900", "#268bd2", "#d33682", "#2aa198", "#eee8d5" },
        { "#002b36", "#cb4b16", "#586e75", "#657b83", "#839496", "#6c71c4", "#93a1a1", "#fdf6e3" }
    },
    {
        "Solarized Light",
        "#657b83", "#fdf6e3",
        { "#073642", "#dc322f", "#859900", "#b58900", "#268bd2", "#d33682", "#2aa198", "#eee8d5" },
        { "#002b36", "#cb4b16", "#586e75", "#657b83", "#839496", "#6c71c4", "#93a1a1", "#fdf6e3" }
    },
    {
        "Dracula",
        "#f8f8f2", "#282a36",
        { "#21222c", "#ff5555", "#50fa7b", "#f1fa8c", "#bd93f9", "#ff79c6", "#8be9fd", "#f8f8f2" },
        { "#6272a4", "#ff6e6e", "#69ff94", "#ffffa5", "#d6acff", "#ff92df", "#a4ffff", "#ffffff" }
    },
    {
        "Gruvbox Dark",
        "#ebdbb2", "#282828",
        { "#282828", "#cc241d", "#98971a", "#d79921", "#458588", "#b16286", "#689d6a", "#a89984" },
        { "#928374", "#fb4934", "#b8bb26", "#fabd2f", "#83a598", "#d3869b", "#8ec07c", "#ebdbb2" }
    },
    {
        "Gruvbox Light",
        "#3c3836", "#fbf1c7",
        { "#fbf1c7", "#cc241d", "#98971a", "#d79921", "#458588", "#b16286", "#689d6a", "#7c6f64" },
        { "#928374", "#9d0006", "#79740e", "#b57614", "#076678", "#8f3f71", "#427b58", "#3c3836" }
    },
    {
        "Nord",
        "#d8dee9", "#2e3440",
        { "#3b4252", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#88c0d0", "#e5e9f0" },
        { "#4c566a", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#8fbcbb", "#eceff4" }
    },
    {
        "Monokai",
        "#f8f8f2", "#272822",
        { "#272822", "#f92672", "#a6e22e", "#f4bf75", "#66d9ef", "#ae81ff", "#a1efe4", "#f8f8f2" },
        { "#75715e", "#f92672", "#a6e22e", "#e6db74", "#66d9ef", "#ae81ff", "#a1efe4", "#f9f8f5" }
    },
    {
        "Tokyo Night",
        "#c0caf5", "#1a1b26",
        { "#15161e", "#f7768e", "#9ece6a", "#e0af68", "#7aa2f7", "#bb9af7", "#7dcfff", "#a9b1d6" },
        { "#414868", "#f7768e", "#9ece6a", "#e0af68", "#7aa2f7", "#bb9af7", "#7dcfff", "#c0caf5" }
    },
    {
        "One Dark",
        "#abb2bf", "#282c34",
        { "#282c34", "#e06c75", "#98c379", "#e5c07b", "#61afef", "#c678dd", "#56b6c2", "#abb2bf" },
        { "#5c6370", "#e06c75", "#98c379", "#e5c07b", "#61afef", "#c678dd", "#56b6c2", "#ffffff" }
    },
    {
        "Cyberpunk 2077",
        "#f9f227", "#0d0d0d",
        { "#0d0d0d", "#ff0055", "#00ff41", "#f9f227", "#00b8ff", "#ff00cc", "#00ffff", "#f9f227" },
        { "#333333", "#ff3377", "#33ff66", "#ffff33", "#33ccff", "#ff33ff", "#33ffff", "#ffff66" }
    },
    {
        "Cyan on Black",
        "#00ffff", "#000000",
        { "#000000", "#cc0000", "#00cc00", "#cccc00", "#0000cc", "#cc00cc", "#00cccc", "#cccccc" },
        { "#666666", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff" }
    },
    {
        "Red on Black",
        "#ff0000", "#000000",
        { "#000000", "#cc0000", "#00cc00", "#cccc00", "#0000cc", "#cc00cc", "#00cccc", "#cccccc" },
        { "#666666", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff" }
    },
    {
        "Lime Green on Black",
        "#00ff00", "#000000",
        { "#000000", "#cc0000", "#00cc00", "#cccc00", "#0000cc", "#cc00cc", "#00cccc", "#cccccc" },
        { "#666666", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff" }
    },
    {
        "Chartreuse on Black",
        "#7fff00", "#000000",
        { "#000000", "#cc0000", "#00cc00", "#cccc00", "#0000cc", "#cc00cc", "#00cccc", "#cccccc" },
        { "#666666", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff" }
    },
    {
        "Amber on Black",
        "#ffbf00", "#000000",
        { "#000000", "#cc0000", "#00cc00", "#cccc00", "#0000cc", "#cc00cc", "#00cccc", "#cccccc" },
        { "#666666", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff" }
    },
    {
        "Blueberry on Black",
        "#8ec5fc", "#000000",
        { "#000000", "#0d47a1", "#1565c0", "#1976d2", "#1e88e5", "#42a5f5", "#64b5f6", "#90caf9" },
        { "#37474f", "#1e88e5", "#42a5f5", "#64b5f6", "#90caf9", "#bbdefb", "#e3f2fd", "#ffffff" }
    },
    {
        "Spring Blossom",
        "#ffb7c5", "#2d5016",
        { "#2d5016", "#c2185b", "#7cb342", "#fbc02d", "#00897b", "#d81b60", "#aed581", "#f8bbd0" },
        { "#558b2f", "#e91e63", "#8bc34a", "#ffeb3b", "#009688", "#ec407a", "#c5e1a5", "#fce4ec" }
    },
    {
        "Summer Sunset",
        "#ff9a5a", "#1a1a2e",
        { "#1a1a2e", "#e64a19", "#ff7043", "#ffab40", "#d81b60", "#8e24aa", "#5c6bc0", "#ffb74d" },
        { "#424242", "#ff5722", "#ff8a65", "#ffc107", "#e91e63", "#9c27b0", "#3f51b5", "#ffe0b2" }
    },
    {
        "Autumn Forest",
        "#d4a373", "#3e2723",
        { "#3e2723", "#bf360c", "#e65100", "#8d6e63", "#5d4037", "#33691e", "#9e9d24", "#a1887f" },
        { "#6d4c41", "#e64a19", "#f57c00", "#a1887f", "#795548", "#558b2f", "#c0ca33", "#d7ccc8" }
    },
    {
        "Winter Frost",
        "#e0f7fa", "#1a237e",
        { "#1a237e", "#006064", "#00838f", "#0097a7", "#00acc1", "#26c6da", "#4dd0e1", "#80deea" },
        { "#283593", "#00897b", "#00acc1", "#26c6da", "#4dd0e1", "#80deea", "#b2ebf2", "#ffffff" }
    },
    {
        "Stormy Night",
        "#b0bec5", "#263238",
        { "#263238", "#37474f", "#455a64", "#546e7a", "#607d8b", "#78909c", "#90a4ae", "#b0bec5" },
        { "#455a64", "#607d8b", "#78909c", "#90a4ae", "#b0bec5", "#cfd8dc", "#eceff1", "#ffffff" }
    },
    {
        "Sunny Day",
        "#fff59d", "#33691e",
        { "#33691e", "#f57f17", "#c0ca33", "#00acc1", "#00897b", "#5c6bc0", "#7cb342", "#fff59d" },
        { "#558b2f", "#fbc02d", "#afb42b", "#26c6da", "#0097a7", "#3f51b5", "#9ccc65", "#fff9c4" }
    },
    {
        "Foggy Morning",
        "#cfd8dc", "#455a64",
        { "#455a64", "#546e7a", "#607d8b", "#78909c", "#90a4ae", "#b0bec5", "#cfd8dc", "#eceff1" },
        { "#607d8b", "#78909c", "#90a4ae", "#b0bec5", "#cfd8dc", "#eceff1", "#f5f5f5", "#ffffff" }
    },
    {
        "Lavender Purple on Black",
        "#c8a2c8", "#000000",
        { "#000000", "#8b008b", "#9370db", "#ba55d3", "#9932cc", "#8a2be2", "#da70d6", "#dda0dd" },
        { "#4b0082", "#9400d3", "#a020f0", "#b030b0", "#c040c0", "#d050d0", "#e060e0", "#f070f0" }
    },
    {
        "#d70a53 on Black",
        "#d70a53", "#000000",
        { "#000000", "#8b0045", "#a01050", "#b52060", "#ca3070", "#df4080", "#e85090", "#f060a0" },
        { "#550022", "#6a0033", "#7f0044", "#940055", "#a90066", "#be0077", "#d30088", "#e80099" }
    },
    {
        "Gold on Black",
        "#ffd700", "#000000",
        { "#000000", "#8b6914", "#a07d1a", "#b59120", "#caa526", "#dfb92c", "#f0cd32", "#ffe138" },
        { "#554400", "#6a5500", "#7f6600", "#947700", "#a98800", "#be9900", "#d3aa00", "#e8bb00" }
    },
    {
        "Rose Gold on Black",
        "#e0bfb8", "#000000",
        { "#000000", "#8b4557", "#a05060", "#b55b69", "#ca6672", "#df717b", "#e87c84", "#f0878d" },
        { "#552233", "#6a2d3e", "#7f3849", "#944354", "#a94e5f", "#be596a", "#d36475", "#e86f80" }
    },
    {
        "IBM VGA",
        "#aaaaaa", "#000000",
        { "#000000", "#0000aa", "#00aa00", "#00aaaa", "#aa0000", "#aa00aa", "#aa5500", "#aaaaaa" },
        { "#555555", "#5555ff", "#55ff55", "#55ffff", "#ff5555", "#ff55ff", "#ffff55", "#ffffff" }
    },
    {
        "Miku NT",
        "#338296", "#1a1a1a",
        { "#1a1a1a", "#e43d82", "#89cdc6", "#338296", "#338296", "#e43d82", "#89cdc6", "#fef5f7" },
        { "#3a3a3a", "#f060a0", "#a0e0d8", "#4a9aad", "#4a9aad", "#f060a0", "#a0e0d8", "#ffffff" }
    },
    {
        "Strange Red",
        "#ca6a6a", "#090404",
        { "#090404", "#ca6a6a", "#aa5050", "#7b3a3a", "#7b3a3a", "#ca6a6a", "#aa5050", "#ca6a6a" },
        { "#2a1515", "#e08080", "#c06060", "#904848", "#904848", "#e08080", "#c06060", "#f0a0a0" }
    },
    {
        "Burgundy on Black",
        "#900020", "#000000",
        { "#000000", "#900020", "#600018", "#700018", "#500010", "#a00028", "#800020", "#c00030" },
        { "#300008", "#b00028", "#800020", "#900020", "#700018", "#c00030", "#a00028", "#e00038" }
    },
    {
        "Win95 Teal",
        "#008080", "#000000",
        { "#000000", "#aa0000", "#00aa00", "#aa5500", "#0000aa", "#aa00aa", "#00aaaa", "#aaaaaa" },
        { "#555555", "#5555ff", "#55ff55", "#55ffff", "#ff5555", "#ff55ff", "#ffff55", "#ffffff" }
    }
};

/* ------------------------------------------------------------------ */
/* Predefined ANSI palettes                                            */
/* ------------------------------------------------------------------ */

static struct ANSIPalette palettes[MAX_PALETTES] = {
    {
        "Nord",
        { "#3b4252", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#88c0d0", "#e5e9f0" },
        { "#4c566a", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#8fbcbb", "#eceff4" }
    },
    {
        "Gruvbox Dark",
        { "#282828", "#cc241d", "#98971a", "#d79921", "#458588", "#b16286", "#689d6a", "#a89984" },
        { "#928374", "#fb4934", "#b8bb26", "#fabd2f", "#83a598", "#d3869b", "#8ec07c", "#ebdbb2" }
    },
    {
        "Gruvbox Light",
        { "#fbf1c7", "#cc241d", "#98971a", "#d79921", "#458588", "#b16286", "#689d6a", "#7c6f64" },
        { "#928374", "#9d0006", "#79740e", "#b57614", "#076678", "#8f3f71", "#427b58", "#3c3836" }
    },
    {
        "Catppuccin Latte",
        { "#5c5f77", "#d20f39", "#40a02b", "#df8e1d", "#1e66f5", "#ea76cb", "#179299", "#acb0be" },
        { "#6c6f85", "#d20f39", "#40a02b", "#df8e1d", "#1e66f5", "#ea76cb", "#179299", "#bcc0cc" }
    },
    {
        "Catppuccin Frappé",
        { "#626880", "#e78284", "#a6d189", "#e5c890", "#8caaee", "#f4b8e4", "#81c8be", "#a5adce" },
        { "#737994", "#e78284", "#a6d189", "#e5c890", "#8caaee", "#f4b8e4", "#81c8be", "#b5bfe2" }
    },
    {
        "Catppuccin Macchiato",
        { "#5b6078", "#ed8796", "#a6da95", "#eed49f", "#8aadf4", "#f5bde6", "#8bd5ca", "#a5adcb" },
        { "#6e738d", "#ed8796", "#a6da95", "#eed49f", "#8aadf4", "#f5bde6", "#8bd5ca", "#b8c0e0" }
    },
    {
        "Catppuccin Mocha",
        { "#585b70", "#eba0ac", "#a6e3a1", "#f9e2af", "#89b4fa", "#f5c2e7", "#94e2d5", "#a6adc8" },
        { "#7f849c", "#eba0ac", "#a6e3a1", "#f9e2af", "#89b4fa", "#f5c2e7", "#94e2d5", "#bac2de" }
    },
    {
        "Dracula",
        { "#21222c", "#ff5555", "#50fa7b", "#f1fa8c", "#bd93f9", "#ff79c6", "#8be9fd", "#f8f8f2" },
        { "#6272a4", "#ff6e6e", "#69ff94", "#ffffa5", "#d6acff", "#ff92df", "#a4ffff", "#ffffff" }
    },
    {
        "Tokyo Night",
        { "#15161e", "#f7768e", "#9ece6a", "#e0af68", "#7aa2f7", "#bb9af7", "#7dcfff", "#a9b1d6" },
        { "#414868", "#f7768e", "#9ece6a", "#e0af68", "#7aa2f7", "#bb9af7", "#7dcfff", "#c0caf5" }
    },
    {
        "Syntwave '84",
        { "#261f33", "#ff3838", "#3eff7a", "#ffe43e", "#3e8bff", "#ff3ed4", "#3effe0", "#f8f8f2" },
        { "#4a3f5c", "#ff5c5c", "#5cff9c", "#ffff5c", "#5c9cff", "#ff5cf0", "#5cffff", "#ffffff" }
    },
    {
        "Tango",
        { "#2e3436", "#cc0000", "#4e9a06", "#c4a000", "#3465a4", "#75507b", "#06989a", "#d3d7cf" },
        { "#555753", "#ef2929", "#8ae234", "#fce94f", "#729fcf", "#ad7fa8", "#34e2e2", "#eeeeec" }
    },
    {
        "Solarized Dark",
        { "#073642", "#dc322f", "#859900", "#b58900", "#268bd2", "#d33682", "#2aa198", "#eee8d5" },
        { "#002b36", "#cb4b16", "#586e75", "#657b83", "#839496", "#6c71c4", "#93a1a1", "#fdf6e3" }
    },
    {
        "Solarized Light",
        { "#073642", "#dc322f", "#859900", "#b58900", "#268bd2", "#d33682", "#2aa198", "#eee8d5" },
        { "#002b36", "#cb4b16", "#586e75", "#657b83", "#839496", "#6c71c4", "#93a1a1", "#fdf6e3" }
    },
    {
        "Monokai",
        { "#272822", "#f92672", "#a6e22e", "#f4bf75", "#66d9ef", "#ae81ff", "#a1efe4", "#f8f8f2" },
        { "#75715e", "#f92672", "#a6e22e", "#e6db74", "#66d9ef", "#ae81ff", "#a1efe4", "#f9f8f5" }
    }
};

/* ------------------------------------------------------------------ */
/* Global state                                                        */
/* ------------------------------------------------------------------ */

static char font_list[MAX_FONTS][MAX_FONT_LEN];
static int font_count = 0;

static int selected_theme = -1;
static int selected_palette = -1;
static char selected_font[MAX_FONT_LEN] = "";
static int font_size = DEFAULT_SIZE;
static double opacity = DEFAULT_OPACITY;

/* ------------------------------------------------------------------ */
/* C89-compatible case-insensitive substring search                    */
/* ------------------------------------------------------------------ */

static char *stristr(const char *haystack, const char *needle)
{
    size_t haystack_len;
    size_t needle_len;
    size_t i;
    size_t j;

    if (needle[0] == '\0') {
        return (char *)haystack;
    }

    haystack_len = strlen(haystack);
    needle_len = strlen(needle);

    if (needle_len > haystack_len) {
        return NULL;
    }

    for (i = 0; i <= haystack_len - needle_len; i++) {
        for (j = 0; j < needle_len; j++) {
            char h = haystack[i + j];
            char n = needle[j];
            if (h >= 'A' && h <= 'Z') {
                h = (char)(h + 32);
            }
            if (n >= 'A' && n <= 'Z') {
                n = (char)(n + 32);
            }
            if (h != n) {
                break;
            }
        }
        if (j == needle_len) {
            return (char *)(haystack + i);
        }
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/* Extract first font family (truncate at first comma)                */
/* ------------------------------------------------------------------ */

static void extract_first_family(char *name)
{
    size_t i;
    for (i = 0; name[i] != '\0'; i++) {
        if (name[i] == ',') {
            name[i] = '\0';
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Font name truncation with ellipsis                                  */
/* ------------------------------------------------------------------ */

static void truncate_font_name(char *name, size_t max_len)
{
    size_t len;

    if (max_len < 4) {
        return;
    }

    len = strlen(name);

    if (len > max_len) {
        name[max_len - 4] = '.';
        name[max_len - 3] = '.';
        name[max_len - 2] = '.';
        name[max_len - 1] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* Font loading from fc-list                                           */
/* ------------------------------------------------------------------ */

static int load_fonts(void)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    char cmd[MAX_LINE_LEN + 64];
    int i;
    int found;

    /* Use system() to write fc-list output to temp file — C89 safe */
    sprintf(cmd, "fc-list : family > %s 2>/dev/null", TEMP_FILE);
    if (system(cmd) != 0) {
        fprintf(stderr, "Error: fc-list command failed. Is fontconfig installed?\n");
        return 0;
    }

    fp = fopen(TEMP_FILE, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open temp file %s\n", TEMP_FILE);
        return 0;
    }

    font_count = 0;

    while (fgets(line, sizeof(line), fp) != NULL && font_count < MAX_FONTS) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        /* Skip empty lines */
        if (line[0] == '\0') {
            continue;
        }

        /* Extract first font family (truncate at first comma) */
        extract_first_family(line);

        /* Skip duplicate fonts */
        found = 0;
        for (i = 0; i < font_count; i++) {
            if (strcmp(font_list[i], line) == 0) {
                found = 1;
                break;
            }
        }
        if (found) {
            continue;
        }

        /* Truncate font name to MAX_FONT_LEN with ellipsis */
        truncate_font_name(line, MAX_FONT_LEN);

        /* Safe copy with explicit null-termination */
        strncpy(font_list[font_count], line, MAX_FONT_LEN - 1);
        font_list[font_count][MAX_FONT_LEN - 1] = '\0';
        font_count++;
    }

    fclose(fp);
    return font_count;
}

/* ------------------------------------------------------------------ */
/* Load current config                                                 */
/* ------------------------------------------------------------------ */

static void load_current_config(void)
{
    FILE *fp;
    const char *home;
    char path[1024];
    char line[MAX_LINE_LEN];
    char bg[32] = "", fg[32] = "";
    char colors[16][32];
    int color_found[16] = {0};
    char font[256] = "";
    int fsize = -1;
    double op = -1.0;
    int theme_match = -1;
    int palette_match = -1;
    int i, j;

    /* Build config path */
    home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return;
    }

    sprintf(path, "%s/%s", home, CONFIG_PATH);

    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("No existing config found at %s\n", path);
        return;
    }

    /* Parse config file */
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *key, *val;
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        /* Split on first space */
        key = line;
        val = strchr(line, ' ');
        if (val == NULL) {
            continue;
        }
        *val = '\0';
        val++;

        if (strcmp(key, "background") == 0) {
            strncpy(bg, val, sizeof(bg) - 1);
            bg[sizeof(bg) - 1] = '\0';
        } else if (strcmp(key, "foreground") == 0) {
            strncpy(fg, val, sizeof(fg) - 1);
            fg[sizeof(fg) - 1] = '\0';
        } else if (strncmp(key, "color", 5) == 0) {
            int idx = atoi(key + 5);
            if (idx >= 0 && idx < 16) {
                strncpy(colors[idx], val, 31);
                colors[idx][31] = '\0';
                color_found[idx] = 1;
            }
        } else if (strcmp(key, "font_family") == 0) {
            strncpy(font, val, sizeof(font) - 1);
            font[sizeof(font) - 1] = '\0';
        } else if (strcmp(key, "font_size") == 0) {
            fsize = atoi(val);
        } else if (strcmp(key, "background_opacity") == 0) {
            op = atof(val);
        }
    }

    fclose(fp);

    /* Match theme by background + foreground */
    for (i = 0; i < MAX_THEMES; i++) {
        if (strcmp(themes[i].background, bg) == 0 &&
            strcmp(themes[i].foreground, fg) == 0) {
            theme_match = i;
            break;
        }
    }

    /* Match palette by all 16 colors */
    {
        int all_found = 1;
        for (i = 0; i < 16; i++) {
            if (!color_found[i]) {
                all_found = 0;
                break;
            }
        }
        if (all_found) {
            for (i = 0; i < MAX_PALETTES; i++) {
                int match = 1;
                for (j = 0; j < 8; j++) {
                    if (strcmp(palettes[i].normal[j], colors[j]) != 0 ||
                        strcmp(palettes[i].bright[j], colors[j + 8]) != 0) {
                        match = 0;
                        break;
                    }
                }
                if (match) {
                    palette_match = i;
                    break;
                }
            }
        }
    }

    /* Apply loaded settings */
    printf("\n--- Loaded Config ---\n");

    if (theme_match >= 0) {
        selected_theme = theme_match;
        printf("  Theme:     %s\n", themes[selected_theme].name);
    } else {
        printf("  Theme:     <no match found>\n");
    }

    if (palette_match >= 0) {
        selected_palette = palette_match;
        printf("  ANSI:      %s\n", palettes[selected_palette].name);
    } else {
        printf("  ANSI:      <no match found>\n");
    }

    if (font[0] != '\0') {
        strncpy(selected_font, font, MAX_FONT_LEN - 1);
        selected_font[MAX_FONT_LEN - 1] = '\0';
        printf("  Font:      %s\n", selected_font);
    } else {
        printf("  Font:      <not set>\n");
    }

    if (fsize >= MIN_FONT_SIZE && fsize <= MAX_FONT_SIZE) {
        font_size = fsize;
    }
    printf("  Size:      %d\n", font_size);

    if (op >= 0.0 && op <= 1.0) {
        opacity = op;
    }
    printf("  Opacity:   %.2f\n", opacity);
}

/* ------------------------------------------------------------------ */
/* Menu display                                                        */
/* ------------------------------------------------------------------ */

static void display_menu(void)
{
    printf("\n");
    printf("=== kitty-care — Kitty Configuration Generator ===\n");
    printf("\n");
    printf("Current settings:\n");
    if (selected_theme >= 0) {
        printf("  Theme:     %s\n", themes[selected_theme].name);
    } else {
        printf("  Theme:     <not set>\n");
    }
    if (selected_palette >= 0) {
        printf("  ANSI:      %s\n", palettes[selected_palette].name);
    } else {
        printf("  ANSI:      <not set>\n");
    }
    if (selected_font[0] != '\0') {
        printf("  Font:      %s\n", selected_font);
    } else {
        printf("  Font:      <not set>\n");
    }
    printf("  Size:      %d\n", font_size);
    printf("  Opacity:   %.2f\n", opacity);
    printf("\n");
    printf("[1] Select Theme\n");
    printf("[2] Select ANSI Palette\n");
    printf("[3] Select Font\n");
    printf("[4] Set Font Size\n");
    printf("[5] Set Opacity\n");
    printf("[6] Apply Config\n");
    printf("[7] Load Current Config\n");
    printf("[8] Exit\n");
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* Theme selection                                                     */
/* ------------------------------------------------------------------ */

static void select_theme(void)
{
    int i;
    int choice;

    printf("\n--- Select Theme ---\n");
    for (i = 0; i < MAX_THEMES; i++) {
        printf("[%d] %s\n", i + 1, themes[i].name);
    }
    printf("\nChoice: ");

    if (scanf("%d", &choice) != 1) {
        /* Clear input buffer */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("Invalid input.\n");
        return;
    }
    /* Clear trailing newline */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    if (choice < 1 || choice > MAX_THEMES) {
        printf("Invalid choice. Please select 1-%d.\n", MAX_THEMES);
        return;
    }

    selected_theme = choice - 1;
    printf("Theme set to: %s\n", themes[selected_theme].name);
}

/* ------------------------------------------------------------------ */
/* ANSI palette selection                                              */
/* ------------------------------------------------------------------ */

static void select_palette(void)
{
    int i;
    int choice;

    printf("\n--- Select ANSI Palette ---\n");
    for (i = 0; i < MAX_PALETTES; i++) {
        printf("[%d] %s\n", i + 1, palettes[i].name);
    }
    printf("[%d] Reset to Theme Default\n", MAX_PALETTES + 1);
    printf("\nChoice: ");

    if (scanf("%d", &choice) != 1) {
        /* Clear input buffer */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("Invalid input.\n");
        return;
    }
    /* Clear trailing newline */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    if (choice < 1 || choice > MAX_PALETTES + 1) {
        printf("Invalid choice. Please select 1-%d.\n", MAX_PALETTES + 1);
        return;
    }

    if (choice == MAX_PALETTES + 1) {
        selected_palette = -1;
        printf("ANSI palette reset to theme default.\n");
        return;
    }

    selected_palette = choice - 1;
    printf("ANSI palette set to: %s\n", palettes[selected_palette].name);
}

/* ------------------------------------------------------------------ */
/* Font selection with search                                          */
/* ------------------------------------------------------------------ */

static void select_font(void)
{
    char search[MAX_INPUT_LEN];
    int matches[MAX_FONTS];
    int match_count = 0;
    int i;
    int choice;

    if (font_count == 0) {
        printf("\nNo fonts loaded. Attempting to load fonts...\n");
        if (load_fonts() == 0) {
            printf("No fonts found. Is fontconfig installed?\n");
            return;
        }
        printf("Loaded %d fonts.\n", font_count);
    }

    printf("\n--- Select Font ---\n");
    printf("Enter search term (or press Enter for all): ");

    /* Read search term */
    if (fgets(search, sizeof(search), stdin) == NULL) {
        return;
    }
    /* Remove trailing newline */
    {
        size_t len = strlen(search);
        if (len > 0 && search[len - 1] == '\n') {
            search[len - 1] = '\0';
        }
    }

    /* Find matching fonts */
    for (i = 0; i < font_count; i++) {
        if (search[0] == '\0' || stristr(font_list[i], search) != NULL) {
            matches[match_count] = i;
            match_count++;
        }
    }

    if (match_count == 0) {
        printf("No fonts found matching '%s'.\n", search);
        return;
    }

    printf("\nFound %d matching fonts:\n", match_count);
    for (i = 0; i < match_count; i++) {
        printf("[%d] %s\n", i + 1, font_list[matches[i]]);
    }
    printf("\nChoice: ");

    if (scanf("%d", &choice) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("Invalid input.\n");
        return;
    }
    /* Clear trailing newline */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    if (choice < 1 || choice > match_count) {
        printf("Invalid choice. Please select 1-%d.\n", match_count);
        return;
    }

    /* Safe copy with explicit null-termination */
    strncpy(selected_font, font_list[matches[choice - 1]], MAX_FONT_LEN - 1);
    selected_font[MAX_FONT_LEN - 1] = '\0';

    printf("Font set to: %s\n", selected_font);
    printf("  font_family:      %s\n", selected_font);
    printf("  bold_font:        %s\n", selected_font);
    printf("  italic_font:      %s\n", selected_font);
    printf("  bold_italic_font: %s\n", selected_font);
}

/* ------------------------------------------------------------------ */
/* Font size input                                                     */
/* ------------------------------------------------------------------ */

static void set_font_size(void)
{
    int size;

    printf("\n--- Set Font Size ---\n");
    printf("Enter font size (%d-%d): ", MIN_FONT_SIZE, MAX_FONT_SIZE);

    if (scanf("%d", &size) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("Invalid input.\n");
        return;
    }
    /* Clear trailing newline */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    if (size < MIN_FONT_SIZE) {
        size = MIN_FONT_SIZE;
        printf("Clamped to minimum: %d\n", MIN_FONT_SIZE);
    } else if (size > MAX_FONT_SIZE) {
        size = MAX_FONT_SIZE;
        printf("Clamped to maximum: %d\n", MAX_FONT_SIZE);
    }

    font_size = size;
    printf("Font size set to: %d\n", font_size);
}

/* ------------------------------------------------------------------ */
/* Opacity input                                                       */
/* ------------------------------------------------------------------ */

static void set_opacity(void)
{
    double op;

    printf("\n--- Set Opacity ---\n");
    printf("Enter opacity (0.0 - 1.0): ");

    if (scanf("%lf", &op) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("Invalid input.\n");
        return;
    }
    /* Clear trailing newline */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    if (op < 0.0) {
        op = 0.0;
        printf("Clamped to minimum: 0.0\n");
    } else if (op > 1.0) {
        op = 1.0;
        printf("Clamped to maximum: 1.0\n");
    }

    opacity = op;
    printf("Opacity set to: %.2f\n", opacity);
}

/* ------------------------------------------------------------------ */
/* Config generation                                                   */
/* ------------------------------------------------------------------ */

static void generate_config(void)
{
    FILE *fp;
    const char *home;
    char path[1024];
    struct Theme *t;
    const char **normal_colors;
    const char **bright_colors;
    int i;

    /* Validate required settings */
    if (selected_theme < 0) {
        printf("\nError: No theme selected. Please select a theme first.\n");
        return;
    }
    if (selected_font[0] == '\0') {
        printf("\nError: No font selected. Please select a font first.\n");
        return;
    }

    /* Build config path */
    home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return;
    }

    sprintf(path, "%s/%s", home, CONFIG_PATH);

    /* Create directory if needed */
    {
        char mkdir_cmd[1024 + 32];
        sprintf(mkdir_cmd, "mkdir -p %s/.config/kitty", home);
        system(mkdir_cmd);
    }

    /* Open config file */
    fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open %s for writing.\n", path);
        return;
    }

    t = &themes[selected_theme];

    /* Determine which ANSI colors to use */
    if (selected_palette >= 0) {
        normal_colors = (const char **)palettes[selected_palette].normal;
        bright_colors = (const char **)palettes[selected_palette].bright;
    } else {
        normal_colors = (const char **)t->normal;
        bright_colors = (const char **)t->bright;
    }

    /* Write header */
    fprintf(fp, "# Generated by kitty-conf\n\n");

    /* Write font */
    fprintf(fp, "font_family %s\n", selected_font);
    fprintf(fp, "bold_font %s\n", selected_font);
    fprintf(fp, "italic_font %s\n", selected_font);
    fprintf(fp, "bold_italic_font %s\n", selected_font);
    fprintf(fp, "font_size %d.0\n\n", font_size);

    /* Write colors */
    fprintf(fp, "background %s\n", t->background);
    fprintf(fp, "foreground %s\n", t->foreground);

    /* Write normal colors (color0-color7) */
    for (i = 0; i < 8; i++) {
        fprintf(fp, "color%d %s\n", i, normal_colors[i]);
    }

    /* Write bright colors (color8-color15) */
    for (i = 0; i < 8; i++) {
        fprintf(fp, "color%d %s\n", i + 8, bright_colors[i]);
    }

    fprintf(fp, "\n");

    /* Write opacity */
    fprintf(fp, "background_opacity %.2f\n", opacity);

    fclose(fp);

    printf("\nConfig generated successfully!\n");
    printf("Written to: %s\n", path);
    printf("\nTo use this config, restart kitty or reload with:\n");
    printf("  ctrl+shift+f5\n");
}

/* ------------------------------------------------------------------ */
/* Help message                                                        */
/* ------------------------------------------------------------------ */

static void print_help(void)
{
    printf("Usage: kitty-conf\n");
    printf("\n");
    printf("Generates a config file for the kitty terminal emulator at:\n");
    printf("  ~/.config/kitty/kitty.conf\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help    Show this help message\n");
    printf("\n");
    printf("Interactive menu allows you to:\n");
    printf("  - Select from 17 predefined color themes\n");
    printf("  - Select from 14 predefined ANSI color palettes\n");
    printf("  - Search and select a system font (via fc-list)\n");
    printf("  - Set font size (6-72)\n");
    printf("  - Set background opacity (0.0-1.0)\n");
    printf("\n");
    printf("Generated config includes font_family, bold_font, italic_font,\n");
    printf("and bold_italic_font settings.\n");
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    int choice;
    int running = 1;

    /* Check for help flag */
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_help();
        return 0;
    }

    printf("Kitty Configuration Generator\n");
    printf("=============================\n");

    /* Pre-load fonts */
    printf("Loading fonts...\n");
    if (load_fonts() > 0) {
        printf("Loaded %d fonts.\n", font_count);
    } else {
        printf("Warning: No fonts loaded. You may need to install fontconfig.\n");
    }

    while (running) {
        display_menu();
        printf("Choice: ");

        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        /* Clear trailing newline */
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
        }

        switch (choice) {
            case 1:
                select_theme();
                break;
            case 2:
                select_palette();
                break;
            case 3:
                select_font();
                break;
            case 4:
                set_font_size();
                break;
            case 5:
                set_opacity();
                break;
            case 6:
                generate_config();
                running = 0;
                break;
            case 7:
                load_current_config();
                break;
            case 8:
                running = 0;
                printf("\nSayonara, Goodbye!\n");
                break;
            default:
                printf("Invalid choice. Please select 1-7.\n");
                break;
        }
    }

    return 0;
}
