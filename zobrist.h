#ifndef ZOBRIST_H
#define ZOBRIST_H
#include "serializer.h"
#include "twixt.h"

typedef struct zobrist_s zobrist_t;
typedef struct hashset_s hashset_t;

hashset_t *hashset_create(int size);
void hashset_free(hashset_t **set);
void hashset_update(hashset_t *set, board_t *board, position_t *position, player_t *player);

zobrist_t *zobrist_create(int nt, int zoom_count);
void zobrist_free(zobrist_t **zobrist);
float zobrist_evaluate(const zobrist_t *zobrist, position_t position, player_t player, const board_t *board);
float zobrist_evalute_hs(const zobrist_t *zobrist, const hashset_t *set, position_t position, player_t player);
void zobrist_populate(const zobrist_t *zobrist, const game_t *game);
char *zobrist_serialize(const zobrist_t *zobrist);
zobrist_t *zobrist_deserialize(char *buf);

#endif //ZOBRIST_H
