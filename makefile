CC=gcc
CFLAGS=-lpthread
SOURCES=manager.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=manager

all: 
	$(CC) $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)
clean:
	rm -f $(OBJECTS)
	rm -f $(EXECUTABLE)
	rm *.out


