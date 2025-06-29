#ifndef ZOBRIST_H
#define ZOBRIST_H
#include "game.h"
#include "twixt.h"

typedef struct zobrist_s zobrist_t;

zobrist_t *zobrist_create(int nt, int zoom_count);
void zobrist_free(zobrist_t **zobrist);
float zobrist_evaluate(const zobrist_t *zobrist, position_t position, player_t player, const board_t *board);
position_t zobrist_best_move(zobrist_t *zobrist, const player_t player, const board_t *board);
void zobrist_populate(const zobrist_t *zobrist, const game_t *game);
unsigned char *zobrist_serialize(const zobrist_t *zobrist, unsigned long *length);
zobrist_t *zobrist_deserialize(unsigned char *buf, unsigned long length);

#endif //ZOBRIST_H