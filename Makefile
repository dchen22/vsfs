# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Executable names
MAIN_TARGET = main
TESTS_TARGET = tests

# Object files
FS_OBJS = fs.o mkfs.o helpers.o

MAIN_OBJS = main.o $(FS_OBJS)
TESTS_OBJS = tests.o $(FS_OBJS)

# Default rule builds both executables
all: $(MAIN_TARGET) $(TESTS_TARGET)

# Link main program
$(MAIN_TARGET): $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_OBJS)

# Link tests program
$(TESTS_TARGET): $(TESTS_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TESTS_OBJS)

# Compilation rules
main.o: main.c fs.h mkfs.h
	$(CC) $(CFLAGS) -c main.c

tests.o: tests.c fs.h mkfs.h helpers.h
	$(CC) $(CFLAGS) -c tests.c

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

mkfs.o: mkfs.c mkfs.h helpers.h
	$(CC) $(CFLAGS) -c mkfs.c

helpers.o: helpers.c helpers.h
	$(CC) $(CFLAGS) -c helpers.c

# Clean up
clean:
	rm -f *.o $(MAIN_TARGET) $(TESTS_TARGET)
