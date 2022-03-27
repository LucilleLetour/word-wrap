CC = gcc
CFLAGS = -g -Wall -fsanitize=undefined -fsanitize=address -std=c99

ww: ww.c
	$(CC) $(CFLAGS) -o $@ $^
