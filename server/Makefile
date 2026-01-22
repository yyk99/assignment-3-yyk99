#
#
#

CC = $(CROSS_COMPILE)gcc
CFLAGS += -g -Wall -Werror
ALL = aesdsocket

all: $(ALL)

aesdsocket: aesdsocket.c Makefile
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c

clean:
	$(RM) *.o *~ $(ALL)
