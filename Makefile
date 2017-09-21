CC = gcc
CFLAGS = -Wall -std=c99 -g
PORT = 50134
CFLAGS += -DPORT=\$(PORT)
all: mismatch

mismatch: mismatch.c utils.c
	$(CC) $(CFLAGS) mismatch.c utils.c -o mismatch_server


clean:
	rm mismatch_server
