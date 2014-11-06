CC=gcc
SRCS=giftop.c anim.c dib.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-O2 -std=c90 -Wall 
LDFLAGS=-s -lgdi32 -lgif
ARCH=32
NAME=giftop
EXE=$(NAME).exe

.PHONY: all clean
all: $(EXE)
$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS) -m$(ARCH) $(LDFLAGS)
.c.o:
	$(CC) -o $@ $(CFLAGS) -m$(ARCH) -c $<

clean:
	rm -f $(OBJS) $(EXE)
