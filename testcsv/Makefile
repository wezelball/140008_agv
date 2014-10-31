
## Created by Anjuta

CC = gcc
CFLAGS = -g -Wall
OBJECTS = testcsv.o
INCFLAGS = -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS =

all: testcsv

testcsv: $(OBJECTS)
	$(CC) $(CFLAGS) -o testcsv $(OBJECTS) $(LDFLAGS) $(LIBS)


testcsv.o: testcsv.c
	$(CC) $(CFLAGS) -c testcsv.c -o testcsv.o

.SUFFIXES:
.SUFFIXES:	.c .cc .C .cpp .o

.c.o :
	$(CC) -o $@ -c $(CFLAGS) $< $(INCFLAGS)

count:
	wc *.c *.h

clean:
	rm -f *.o testcsv *~

.PHONY: all
.PHONY: count
.PHONY: clean
