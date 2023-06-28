CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: byteshell

byteshell: byteshell.c
	$(CC) $(CFLAGS) -o byteshell byteshell.c

clean veryclean:
	$(RM) byteshell