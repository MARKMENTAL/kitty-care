# kitty-care — interactive kitty configuration generator

PYTHON ?= python3
PREFIX ?= $(HOME)/.local
BIN    := kitty-care
DEST   := $(DESTDIR)$(PREFIX)/bin
SHARE  := $(DESTDIR)$(PREFIX)/share/kitty-care
THEMES := themes.toml ansi_palettes.toml

all: check

# Byte-compile the script and the tests without leaving a binary behind
check:
	$(PYTHON) -m py_compile $(BIN) tests/test_kitty_care.py

test:
	$(PYTHON) -m unittest discover -s tests -v

install:
	mkdir -p $(DEST) $(SHARE)
	install -m755 $(BIN) $(DEST)/$(BIN)
	install -m644 $(THEMES) $(SHARE)

uninstall:
	rm -f $(DEST)/$(BIN)
	rm -f $(addprefix $(SHARE)/,$(THEMES))

clean:
	rm -rf __pycache__ tests/__pycache__

.PHONY: all check test install uninstall clean
