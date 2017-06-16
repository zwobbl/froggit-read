CC=gcc

CFLAGS=-std=gnu99
LIBRARIES=-lwiringPi -lpthread

all:  froggitread

froggitread: froggitread.h froggitread.c
	$(CC) $(CFLAGS) -o froggitread froggitread.c $(LIBRARIES)
	chmod +x froggitread

clean: 
	rm froggitread
