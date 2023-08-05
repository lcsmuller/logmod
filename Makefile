OBJS = logmod.o

CFLAGS = -Wall -std=c89 -Wpedantic -I.

all: $(OBJS)

clean:
	@ rm -f $(OBJS)

.PHONY: clean