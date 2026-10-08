# Compiler and flags
CC      := gcc
CFLAGS  := -std=c89 -Wall -Wextra

SRC     := kitty-conf.c
BIN     := kitty-conf

# Default target — builds the binary
all: $(BIN)

# Compile the source into the binary
$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

# Force a full rebuild
rebuild: clean all

# Build and run
run: $(BIN)
	./$(BIN)

# Syntax-check only, no binary output
check: $(SRC)
	$(CC) $(CFLAGS) -fsyntax-only $(SRC)

# Remove build artifacts
clean:
	rm -f $(BIN)

.PHONY: all rebuild run check clean
