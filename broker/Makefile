CC=gcc
CFLAGS=-Wall -Wextra -O2 -DMSG_SIZE=8 -DNDEBUG
SRCS=broker.c server.c
OBJS=$(patsubst %.c, %.o, $(SRCS))
HDRS=globals.h
TARGET=broker

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(OBJS) : $(SRCS) $(HDRS)

clean:
	rm -f *.o *.out broker
