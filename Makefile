CC = gcc
LIBS = cairo gtk+-3.0
LINKFLAGS = `pkg-config --cflags --libs $(LIBS)`
CFLAGS = -Wall -Wextra `pkg-config --cflags --libs $(LIBS)`
TGT = twixt_ai

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

debug: CFLAGS += -DDEBUG -g
debug: $(TGT)

release: CFLAGS += -DRELEASE -Ofast
release: $(TGT)

$(TGT) : $(OBJ)
	$(CC) $(LINKFLAGS) $^ -o $@


%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean :
	rm -f *.o $(TGT)