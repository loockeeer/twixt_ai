#ifndef MONTECARLO_H
#define MONTECARLO_H
#include"twixt.h"

typedef struct label_s label_t;
typedef struct tree_s tree_t;

tree_t *mc_create_tree();

void mc_destroy_tree(tree_t *tree);

position_t mc_search(int ns, int nt, const board_t *board, tree_t **tree, player_t player);

void mc_advance_tree(tree_t **tree, position_t position);

#endif //MONTECARLO_H
