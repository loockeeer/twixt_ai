#ifndef SERIALIZER_H
#define SERIALIZER_H
#include "twixt.h"

typedef enum metadata_provider {
    TWIXTLIVE
} metadata_provider_t;

typedef struct metadata_twixtlive_s {
    long int gid;
    long int timestamp;
    long int whiteid;
    long int blackid;
} metadata_twixtlive_t;

metadata_twixtlive_t *metadata_twixtlive_deserialize(char *buf, char **end);
char *metadata_twixtlive_serialize(metadata_twixtlive_t *twixtlive);

metadata_provider_t metadata_deserialize(char *buf, void **dst, char **end);
char *metadata_serialize(void *data, metadata_provider_t metadata_provider);
void metadata_free(void *data, metadata_provider_t metadata_provider);

typedef enum move_type {
    PEG,
    SWAP,
    RESIGN,
    WIN
} move_type_t;

typedef struct moves_s {
    move_type_t type;
    player_t player;
    position_t peg;
    struct moves_s *next;
} moves_t;

moves_t *moves_create(move_type_t type, player_t player, int red_pos, int black_pos);
moves_t *moves_append(moves_t *before, moves_t *new);
void moves_free(moves_t *moves);

typedef struct game_s {
    metadata_provider_t provider;
    void *metadata;
    int size;
    board_t *board; // can be null ! -> if so game not generated already !
    moves_t *moves;
    bool finished;
    bool resigned;
    player_t winner;
} game_t;

game_t *game_deserialize(char *buf);
char *game_serialize(game_t *game);
void game_free(game_t *game);


#endif //SERIALIZER_H
