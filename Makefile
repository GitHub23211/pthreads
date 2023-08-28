CC = gcc
CFLAGS = -Wall -Wextra
DEPS = th-lookup.h
OBJ = th-lookup.o queue.o util.o

.PHONY: clean

th-lookup: $(OBJ) $(DEPS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f th-lookup
	rm -f *.o