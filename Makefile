CC = gcc
CFLAGS = -g -Wall 
DEPS = hash_table.c

all: server.c client.c
	$(CC) $(CFLAGS) -o server server.c $(DEPS) -lpthread -lrt
	$(CC) $(CFLAGS) -o client client.c $(DEPS) -lpthread -lrt

clean:
	$(RM) server
	$(RM) client