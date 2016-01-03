CFLAGS=-g -Wall
CC=gcc
all:json json.so

json:json.c
	$(CC) $(CFLAGS) $< -o $@ -llua -ldl -lm

json.so:json.c
	$(CC) $(CFLAGS) -fpic -shared $< -o $@

clean:
	rm -rf json json.so core
