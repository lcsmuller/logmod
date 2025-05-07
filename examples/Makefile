TOP = ..

CC = gcc

CFLAGS  = -Wall -std=c99 -I$(TOP) -g -O0
LDFLAGS = -pthread

EXAMPLES = basic

.PHONY: all clean

all: $(EXAMPLES)

clean:
	@ rm -f $(EXAMPLES) *.log
