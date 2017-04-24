CFLAGS = -Wall -g -fprofile-arcs -ftest-coverage
COMPILER = gcc

SRC = test.c hashlib.c bitarraylib.c
LIBH = hashlib.h bitarraylib.h

main: $(SRC) $(LIBH)
	$(COMPILER) $(CFLAGS) $(SRC) -o test

clear:
	rm *.gcov *.gcda *.gcno