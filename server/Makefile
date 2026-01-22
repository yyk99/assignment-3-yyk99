#
#
#

CC = $(CROSS_COMPILE)gcc
CFLAGS += -g -Wall -Werror
ALL = aesdsocket sample

all: $(ALL)

aesdsocket: aesdsocket.c queue.h Makefile
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c

sample: sample.c queue.h shannon.c Makefile
	$(CC) $(CFLAGS) -o sample sample.c

clean:
	$(RM) *.o *~ $(ALL)
