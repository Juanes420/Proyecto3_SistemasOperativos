# Makefile — Editor CEZ
#
# Flags importantes:
#   -Wall -Wextra   : todos los warnings (código limpio)
#   -g              : símbolos de debug para valgrind
#   -O2             : optimización del compilador
#   -lz             : enlazar con zlib

CC      = gcc
CFLAGS  = -Wall -Wextra -g -O2
LDFLAGS = -lz

SRCS    = main.c editor.c compress.c io.c
OBJS    = $(SRCS:.c=.o)
TARGET  = editor

all: $(TARGET) benchmark_clasico benchmark_nuestro

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

benchmark_clasico: benchmark_clasico.c
	$(CC) $(CFLAGS) -o $@ $

benchmark_nuestro: benchmark_nuestro.c compress.c io.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $

clean:
	rm -f $(OBJS) $(TARGET) benchmark_clasico benchmark_nuestro *.cez

.PHONY: all clean