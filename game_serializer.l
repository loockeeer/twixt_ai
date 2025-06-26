%{
#include <stdlib.h>
#include <string.h>
#include "game_serializer.tab.h"

int yycharpos = 0;

%}
%option nounput noinput

INT     0|(-?[1-9][0-9]*)
LET     \[[A-Z]+

%%
{INT}           { yycharpos += yyleng;yylval.num = strtol(yytext, NULL, 10); return NUMBER; }
{LET}           { yycharpos += yyleng;yylval.word = strndup(yytext+1,yyleng); return WORD; }

"twixtlive"     { yycharpos += yyleng;return TOK_TWIXTLIVE; }

"i"             { yycharpos += yyleng;return 'i'; }
"t"             { yycharpos += yyleng;return 't'; }
"w"             {yycharpos += yyleng; return 'w'; }
"b"             {yycharpos += yyleng; return 'b'; }

"W"             { yycharpos += yyleng;return 'W'; }
"B"             {yycharpos += yyleng; return 'B'; }
"R"             { yycharpos += yyleng;return 'R'; }
"S"             { yycharpos += yyleng;return 'S'; }

"["             {yycharpos += yyleng; return '['; }
"]"             {yycharpos += yyleng; return ']'; }
";"             {yycharpos += yyleng; return ';'; }
","             {yycharpos += yyleng; return ','; }

.   { yycharpos += yyleng; return *yytext; }
%%