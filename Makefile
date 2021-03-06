#CC=gcc
SRCS=giftop.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-O2 -std=c11 -Wall
LDFLAGS=-s -static -lgdi32 -lgif 
ARCH=32
NAME=giftop
EXE=$(NAME).exe

.PHONY: all clean
all: $(EXE)
$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)
.c.o:
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(EXE)
