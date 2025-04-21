TOP = ..

OBJS = $(TOP)/logmod.o

CFLAGS += -Wall -std=c89 -Wpedantic -I$(TOP) -g

TESTS = test

all: $(TESTS)

$(TESTS): $(OBJS)

$(OBJS):
	CFLAGS="-g" $(MAKE) -C $(TOP)

clean:
	@ $(MAKE) -C $(TOP) clean
	@ rm -f $(TESTS)

.PHONY: clean
