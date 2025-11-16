# Makefile for BCL Extensions
# Compiles extension modules as shared objects (.so)

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 -I../include
LDFLAGS = -shared

# Extension targets
EXTENSIONS = socket.so

.PHONY: all clean

all: $(EXTENSIONS)

# Compile socket extension
socket.so: socket.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# Clean compiled extensions
clean:
	rm -f *.so *.o

# Install extensions to lib directory (optional)
install: $(EXTENSIONS)
	mkdir -p ../lib/extensions
	cp *.so ../lib/extensions/
