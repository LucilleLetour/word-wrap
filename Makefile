CC = gcc
CFLAGS = -g -Wall -fsanitize=address,undefined -std=c99
CFLAGS2 = -g -std=c99 -pthread

all: threadww kekW
	@:

#ww: ww.c
#	$(CC) $(CFLAGS) -o $@ $^

threadww: threadww.c
	$(CC) $(CFLAGS2) -o $@ $^

kekW: testing.c
	$(CC) $(CFLAGS2) -o $@ $^

cleant:
	find ./test -type f -name 'wrap.*' -delete
	find ./testingDirectory -type f -name 'wrap.*' -delete
