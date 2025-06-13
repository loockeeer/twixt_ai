//
// Created by lucas on 13/06/2025.
//

#ifndef ZOBRIST_H
#define ZOBRIST_H
#include "serializer.h"
#include "twixt.h"

typedef struct zobrist_s zobrist_t;

zobrist_t *zobrist_create(int zoom_count, int *zooms);
void zobrist_destroy(zobrist_t *zobrist);
float zobrist_evaluate(zobrist_t *zobrist, position_t position, board_t *board);

void zobrist_update(zobrist_t *zobrist, game_t *game);
char *zobrist_serialize(zobrist_t *zobrist);
zobrist_t *zobrist_deserialize(char *buf, char **end);

#endif //ZOBRIST_H
