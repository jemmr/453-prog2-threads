# New simplified Makefile. Edit SHAREDLIBS line to use Dr. Nico's library
CC      = gcc
CFLAGS  = -Wall -fPIC -g -I. -I../../include
# SHAREDLIBS = util64.o libPLN.so libsnakes.so
SHAREDLIBS = util64.o liblwp.so libsnakes.so

SNAKEOBJS  = randomsnakes.o util.o
SNAKELIBS = -lncurses -lrt

LIBS 	= $(SHAREDLIBS)
OBJS 	= $(OBJS64) $(SNAKEOBJS)
OBJS64	= lwp64.o magic64.o rr64.o util64.o
SRCS	= lwp.c util.c rr.c tid.c magic.s
HDRS	= 

all: 	$(LIBS)

clean:	
	rm -f $(OBJS) *~ TAGS *.a *.o nums hungry snakes liblwp.so

nums: numbersmain.o $(SHAREDLIBS)
	$(CC) $(CFLAGS) -g -o nums numbersmain.o $(SNAKELIBS) $(SHAREDLIBS)

numbersmain.o: numbersmain.c AlwaysZero.c lwp.h
	$(CC) $(CFLAGS) -c numbersmain.c -L. -llwp

magic.o: magic.s
	$(CC) $(CFLAGS) -c -o magic.o magic.s

%64.o: %.c
	$(CC) $(CFLAGS) -m64 -c -o $@ $*.c

%64.o: %64.S
	$(CC) $(CFLAGS) -m64 -c -o $@ $<

lwp.o: lwp.h

liblwp.so: $(OBJS64)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS64)

snakes: randomsnakes.o AlwaysZero.c $(SHAREDLIBS)
	$(CC) $(CFLAGS) -o snakes randomsnakes.o $(SNAKELIBS) $(SHAREDLIBS)

hungry: hungrysnakes.o AlwaysZero.c $(SHAREDLIBS)
	$(CC) $(CFLAGS) -o hungry hungrysnakes.o $(SNAKELIBS) $(SHAREDLIBS)

hungrysnakes.o: hungrysnakes.c AlwaysZero.c util.c lwp.h snakes.h
	$(CC) $(CFLAGS) -c hungrysnakes.c -L. -llwp

randomsnakes.o: randomsnakes.c AlwaysZero.c util.c lwp.h snakes.h
	$(CC) $(CFLAGS) -c randomsnakes.c -L. -llwp

numbermain.o: numbersmain.c lwp.h
	$(CC) $(CFLAGS) -c numbersmain.c

rs: snakes
	(export LD_LIBRARY_PATH=.; ./snakes)

hs: hungry
	(export LD_LIBRARY_PATH=.; ./hungry)

ns: nums
	(export LD_LIBRARY_PATH=.; ./nums)

submit: lwp.c README.txt Makefile
	tar -czf program2_submission.tgz lwp.c README.txt Makefile
