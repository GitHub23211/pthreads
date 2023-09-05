CC = gcc
CFLAGS = -Wall -Wextra
DEPS = th-lookup.h
OBJ = th-lookup.o queue.o util.o

.PHONY: clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

th-lookup: $(OBJ) $(DEPS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f th-lookup
	rm -f *.o