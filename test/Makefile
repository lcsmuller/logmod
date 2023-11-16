TOP = ..

OBJS = $(TOP)/logmod.o

CFLAGS = -Wall -std=c89 -Wpedantic -I$(TOP)

TESTS = test

all: $(TESTS)

$(TESTS): $(OBJS)

$(OBJS):
	$(MAKE) -C $(TOP)

clean:
	@ rm -f $(TESTS)

.PHONY: clean
