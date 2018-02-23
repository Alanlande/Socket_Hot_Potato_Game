CC = gcc
CFLAGS = -ggdb3 -pedantic -std=gnu99 -Wall -Werror 

all: player ringmaster

player: potato.h player.c
	$(CC) $(CFLAGS) -o player player.c 

ringmaster: potato.h ringmaster.c
	$(CC) $(CFLAGS) -o ringmaster ringmaster.c 

clean:
	rm -rf *.o player ringmaster
