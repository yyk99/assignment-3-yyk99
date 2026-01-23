#
#
#

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket
ALL = $(TARGET) sample

all: $(ALL)

$(TARGET): aesdsocket.c queue.h Makefile
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

sample: sample.c queue.h shannon.c Makefile
	$(CC) $(CFLAGS) -o sample sample.c

clean:
	$(RM) *.o *~ $(ALL)
