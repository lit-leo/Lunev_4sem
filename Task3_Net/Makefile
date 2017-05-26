CFLAGS = -Wall -g -std=gnu99
COMPILER = gcc

SRC = server.c client.c server_soft_restart.c

main: $(SRC)
	$(COMPILER) $(CFLAGS) server_soft_restart.c integral.c -o server_soft -pthread
	$(COMPILER) $(CFLAGS) server.c integral.c -o server -pthread
	$(COMPILER) $(CFLAGS) client.c -o client