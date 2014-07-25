# Makefile Mac OS/Linux
# PS 6 Part 2, reverb
#

CC	= gcc
CFLAGS	= -g -std=c99 -Wall -lsndfile
EXE	= reverb delay

all:	$(EXE)

reverb:	reverb.c
	$(CC) $(CFLAGS) reverb.c convolve.c -o reverb

delay:	delay.c
	$(CC) $(CFLAGS) delay.c -o delay

clean:
	rm -f *~ core $(EXE) *.o
