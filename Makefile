SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)
BINS=$(SOURCES:.c=)

CFLAGS+=-std=gnu11 -Wall -O1 -Wpedantic -Werror -pthread -g


all: $(BINS)

.PHONY: clean

clean:
	$(RM) $(OBJECTS) $(BINS)

-include $(DEPS)
