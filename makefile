CC = gcc
CFLAGS = -Wall -g

all: RUDP_Reciever RUDP_Sender

RUDP_Reciever: RUDP_Reciever.o RUDP_API.o
	$(CC) $(CFLAGS) -o RUDP_Reciever RUDP_Reciever.o RUDP_API.o

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(CFLAGS) -o RUDP_Sender RUDP_Sender.o RUDP_API.o

RUDP_Reciever.o: RUDP_Reciever.c
	$(CC) $(CFLAGS) -c RUDP_Reciever.c -o RUDP_Reciever.o

RUDP_Sender.o: RUDP_Sender.c
	$(CC) $(CFLAGS) -c RUDP_Sender.c -o RUDP_Sender.o

RUDP_API.o: RUDP_API.c
	$(CC) $(CFLAGS) -c RUDP_API.c -o RUDP_API.o

clean:
	rm -f RUDP_Reciever RUDP_Sender *.o

.PHONY: all clean
