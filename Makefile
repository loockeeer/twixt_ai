CC = gcc
LEX = flex
YACC = bison
LIBS = cairo gtk+-3.0
PKG_CFLAGS = $(shell pkg-config --cflags $(LIBS))
PKG_LIBS = $(shell pkg-config --libs $(LIBS))
CFLAGS = -Wall -Wextra $(PKG_CFLAGS)
LDFLAGS = $(PKG_LIBS)

TGT = twixt_ai

# Bison/Flex sources
PARSER_NAME = game_serializer
Y_FILE = $(PARSER_NAME).y
L_FILE = $(PARSER_NAME).l

Y_C = $(PARSER_NAME).tab.c
Y_H = $(PARSER_NAME).tab.h
L_C = $(PARSER_NAME).yy.c
L_H = $(PARSER_NAME).yy.h

SRC = $(filter-out $(Y_C) $(L_C), $(wildcard *.c))
OBJ = $(SRC:.c=.o)

# Targets

debug: CFLAGS += -DDEBUG -g
debug: $(TGT)

release: CFLAGS += -DRELEASE -Ofast
release: $(TGT)

ly: $(Y_C) $(L_C) $(Y_H) $(L_H)

$(TGT): ly $(OBJ) $(Y_C:.c=.o) $(L_C:.c=.o)
	$(CC) $(OBJ) $(Y_C:.c=.o) $(L_C:.c=.o) $(LDFLAGS) -o $@

%.o: %.c $(Y_H) $(L_H)
	$(CC) $(CFLAGS) -c $< -o $@

$(Y_C) $(Y_H): $(Y_FILE)
	$(YACC) -d -b $(PARSER_NAME) $<

$(L_C) $(L_H): $(L_FILE)
	$(LEX) --header-file=$(L_H) -o $(L_C) $<

clean:
	rm -f *.o $(TGT) $(Y_C) $(Y_H) $(L_C) $(L_H)
