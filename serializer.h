//
// Created by lucas on 02/06/2025.
//

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

metadata_twixtlive_t *metadata_twixtlive_deserialize(char *buf);
char *metadata_twixtlive_serialize(metadata_twixtlive_t *twixtlive);

metadata_provider_t metadata_deserialize(char *buf, void **dst);
char *metadata_serialize(void *data, metadata_provider_t metadata_provider);
void metadata_free(void *data, metadata_provider_t metadata_provider);

typedef struct moves_s {
    player_t player;
    int red_pos;
    int black_pos;
    bool swap;
    bool win;
} moves_t; // if swap is true -> player swapped. if win is true -> player won. otherwise player paced a peg. ALL! possible links created

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
