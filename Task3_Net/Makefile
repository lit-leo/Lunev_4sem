CFLAGS = -Wall -g
COMPILER = gcc

SRC = server.c client.c

main: $(SRC)
	$(COMPILER) $(CFLAGS) server.c integral.c -o server -pthread
	$(COMPILER) $(CFLAGS) client.c -o client