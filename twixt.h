#ifndef TWIXT_H
#define TWIXT_H
#include<stdbool.h>
typedef enum {
  ONGOING,
  RED_WINS,
  BLACK_WINS,
  DRAW
} outcome_t;

typedef enum player_t {
  NONE = 0,
  RED,
  BLACK,
} player_t;

typedef struct node_s {
  player_t player;
  unsigned char links;
} node_t;

typedef struct board_s {
  int size;
  node_t *data;
  int placed_moves;
} board_t;

typedef struct {
  int x;
  int y;
} position_t;

typedef struct board_s board_t;

board_t *twixt_create(int size);

board_t *twixt_copy(const board_t *board);

int twixt_size(const board_t *board);

void twixt_destroy(board_t *twixt);

bool twixt_play(board_t *twixt, player_t player, position_t position);

outcome_t twixt_check_winner(const board_t *twixt);

position_t twixt_random_move(const board_t *twixt, player_t player);

bool *twixt_available_moves(const board_t *twixt, player_t player);
#endif //TWIXT_H
