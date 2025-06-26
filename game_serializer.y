%code requires {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "serializer.h"
#include "twixt.h"
#include <stdarg.h>

//#include "game_serializer.yy.h"
}

%code {
typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern int yylex();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

int yywrap()
{
    return (1);
}

extern int yycharpos;
extern char *yytext;
extern YY_BUFFER_STATE yy_current_buffer;
char *currently_parsing;
char *error;

#ifdef DEBUG
void yyerror(const char *s, ...) {
    va_list args;
    va_start(args, s);
    fprintf(stderr, "Parse error at character %d: ", yycharpos);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    if (yytext) {
    	fprintf(stderr, "%s\n", currently_parsing);
        fprintf(stderr, "Context near: \"%.20s\"\n", yytext);
    }
    error = strdup(s);
    va_end(args);
}
#else
void yyerror(const char *s) {
  error = strdup(s);
  return;
}
#endif
game_t *parsed_game = NULL;

game_t *y_game_deserialize(char *data) {
    YY_BUFFER_STATE buffer = yy_scan_string(data);
    parsed_game = NULL;
    if (yyparse() != 0 || parsed_game == NULL) {
    	if(parsed_game != NULL) {
    	    game_free(&parsed_game);
    	}
        yy_delete_buffer(buffer);
        return NULL;
    }
    yy_delete_buffer(buffer);
    return parsed_game;
}

int word_to_int(char *word)
{
  int pow = 1;
  int res = 0;
  for(char *cur = word;*cur != '\0';cur++)
  {
    if(*cur < 'A' || *cur > 'Z')
    {
    	yyerror("cannot parse word properly.");
    	return -1;
    }
    res += pow * (*cur - 'A' + 1);
    pow *= 26;
   }
   return res - 1;
}
}

%union {
    long num;
    char *word;
    metadata_twixtlive_t *metadata_twixtlive;
    moves_t *moves;
    player_t player;
}

%token <num> NUMBER
%token <word> WORD

%token TOK_TWIXTLIVE

%type <metadata_twixtlive> metadata_twixtlive
%type <moves> moves
%type <moves> move
%type <moves> opt_moves
%type <player> player

%%

input:
    NUMBER ';' TOK_TWIXTLIVE ',' metadata_twixtlive ';' opt_moves {
        parsed_game = malloc(sizeof(game_t));
        parsed_game->size = $1;
        parsed_game->provider = PROVIDER_TWIXTLIVE;
        parsed_game->metadata = $5;
        parsed_game->moves = $7;
        parsed_game->board = NULL;
    }
    | NUMBER ';' opt_moves {
        parsed_game = malloc(sizeof(game_t));
        parsed_game->size = $1;
        parsed_game->provider = PROVIDER_NONE;
        parsed_game->metadata = NULL;
        parsed_game->moves = $3;
	parsed_game->board = NULL;
    }
    ;

opt_moves: { $$ = NULL;}
	| moves

metadata_twixtlive:
    'i' NUMBER ',' 't' NUMBER ',' 'w' NUMBER ',' 'b' NUMBER {
        metadata_twixtlive_t *m = malloc(sizeof(metadata_twixtlive_t));
        m->gid = $2;
        m->timestamp = $5;
        m->whiteid = $8;
        m->blackid = $11;
        $$ = m;
    }
    | 'i' NUMBER ',' 't' NUMBER ',' 'w' NUMBER {
        metadata_twixtlive_t *m = malloc(sizeof(metadata_twixtlive_t));
        m->gid = $2;
        m->timestamp = $5;
        m->whiteid = $8;
        m->blackid = -1;
        $$ = m;
    }
    ;

moves:
     move
    | move ';' moves {
        $$ = moves_append($1, $3);
    }
    ;

move:
    player WORD NUMBER ']' {
        moves_t *m = moves_create(PEG, $1, word_to_int($2), (int)$3 - 1);
        free($2);
        $$ = m;
    }
    | player 'S' {
    	$$ = moves_create(SWAP, $1, -1, -1);
    }
    | player 'R' {
    	$$ = moves_create(RESIGN, $1, -1, -1);
    }
    | player 'W' {
    	$$ = moves_create(WIN, $1, -1, -1);
    }
    ;

player:
    'R' { $$ = RED; }
    | 'B' { $$ = BLACK; }
    ;