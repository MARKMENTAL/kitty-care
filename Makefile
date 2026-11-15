# kitty-care — interactive kitty configuration generator

PYTHON ?= python3
PREFIX ?= $(HOME)/.local
BIN    := kitty-care
DEST   := $(DESTDIR)$(PREFIX)/bin

all: check

# Byte-compile the script and the tests without leaving a binary behind
check:
	$(PYTHON) -m py_compile $(BIN) tests/test_kitty_care.py

test:
	$(PYTHON) -m unittest discover -s tests -v

install:
	mkdir -p $(DEST)
	install -m755 $(BIN) $(DEST)/$(BIN)

uninstall:
	rm -f $(DEST)/$(BIN)

clean:
	rm -rf __pycache__ tests/__pycache__

.PHONY: all check test install uninstall clean
