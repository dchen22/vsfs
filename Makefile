# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Executable names
MAIN_TARGET = main
TEST_FS_TARGET = test_fs
TEST_FORMAT_TARGET = test_format

# Object files
FS_OBJS = fs.o mkfs.o helpers.o

MAIN_OBJS = main.o $(FS_OBJS)
TEST_FS_OBJS = test_fs.o $(FS_OBJS)
TEST_FORMAT_OBJS = test_format.o $(FS_OBJS)

# Default rule builds all executables
all: $(MAIN_TARGET) $(TEST_FS_TARGET) $(TEST_FORMAT_TARGET)

# Link main program
$(MAIN_TARGET): $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_OBJS)

# Link test_fs program
$(TEST_FS_TARGET): $(TEST_FS_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_FS_OBJS)

# Link test_format program
$(TEST_FORMAT_TARGET): $(TEST_FORMAT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_FORMAT_OBJS)

# Compilation rules
main.o: main.c fs.h mkfs.h
	$(CC) $(CFLAGS) -c main.c

test_fs.o: test_fs.c fs.h mkfs.h
	$(CC) $(CFLAGS) -c test_fs.c

test_format.o: test_format.c fs.h mkfs.h helpers.h
	$(CC) $(CFLAGS) -c test_format.c

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

mkfs.o: mkfs.c mkfs.h helpers.h
	$(CC) $(CFLAGS) -c mkfs.c

helpers.o: helpers.c helpers.h
	$(CC) $(CFLAGS) -c helpers.c

# Clean up
clean:
	rm -f *.o $(MAIN_TARGET) $(TEST_FS_TARGET) $(TEST_FORMAT_TARGET)
