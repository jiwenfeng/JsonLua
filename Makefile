CFLAGS=-g -Wall
CC=gcc
all:json json.so

json:json.c
	$(CC) $(CFLAGS) $< -o $@ -llua -ldl -lm

json.so:json.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@ -llua

clean:
	rm -rf json json.so core
