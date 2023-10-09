CC=gcc
CFLAGS=-Iinclude
LDFLAGS=-lelf -lyaml
SRC_DIR=src
BIN_DIR=.

SOURCES := $(wildcard $(SRC_DIR)/*.c)
TARGETS := $(SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%)

all: $(TARGETS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(TARGETS)