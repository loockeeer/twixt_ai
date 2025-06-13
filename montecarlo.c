#include<float.h>
#include<limits.h>
#include<pthread.h>
#include<stdlib.h>
#include<assert.h>
#include<stdatomic.h>
#include<stdio.h>

#include"montecarlo.h"
#include"twixt.h"
typedef struct label_s {
  atomic_int n_visited;
  atomic_int n_won;
  double prior;
  position_t move;
} label_t;

typedef struct children_container_s {
  tree_t *this;
  bool protected;
} children_container_t;

typedef struct tree_s {
  label_t label;
  children_container_t *children;
  int children_length;
} tree_t;

tree_t *mc_create_tree() {
  tree_t *t = malloc(sizeof(tree_t));
  if (t == NULL) return NULL;
  t->children = NULL;
  atomic_store(&(t->label.n_visited), 0);
  atomic_store(&(t->label.n_won), 0);
  t->label.prior = 0.5;
  t->label.move = (position_t){0, 0};
  t->children = NULL;
  t->children_length = 0;
  return t;
}

void mc_free_tree(tree_t **tree) {
  if (tree == NULL || *tree == NULL) return;
  if ((*tree)->children != NULL) {
    for (int i = 0; i < (*tree)->children_length; i++) {
      if (!(*tree)->children[i].protected) {
        mc_free_tree(&(*tree)->children[i].this);
      }
    }
    free((*tree)->children);
  }
  free(*tree);
  *tree = NULL;
}

double mc_calculate_action(tree_t *tree) {
  assert(tree != NULL);
  int v = atomic_load(&(tree->label.n_visited));
  if (v > 0) {
    int w = atomic_load(&(tree->label.n_won));
    return ((double) w / (double) v) + (tree->label.prior / (1. + (double) v));
  }
  return tree->label.prior;
}

outcome_t heuristic(board_t *board, player_t player) {
  while (true) {
    position_t pos = twixt_random_move(board, player);
    if (!twixt_play(board, player, pos))
      return DRAW;
    outcome_t o = twixt_check_winner(board);
    if (o == ONGOING) {
      player = player == RED ? BLACK : RED;
      continue;
    }
    return o;
  }
}

int get_outcome_value(outcome_t o, player_t opt) {
  if ((o == RED_WINS && opt == RED) || (o == BLACK_WINS && opt == BLACK)) return 5;
  if (o == DRAW) return -1;
  if (o == ONGOING) return 0;
  return -5;
}

tree_t *get_max_action(const tree_t *t) {
  assert(t != NULL);
  tree_t *best = NULL;
  double best_value = DBL_MIN;
  for (int i = 0;i < t->children_length; i++) {
    double av = mc_calculate_action(t->children[i].this);
    if (av > best_value || best == NULL) {
      best_value = av;
      best = t->children[i].this;
    }
  }
  return best;
}

outcome_t descend(board_t *board, const tree_t *tree, player_t opt, player_t player) {
  assert(tree != NULL);
  if (tree->children_length == 0) {
    return heuristic(board, player);
  }

  tree_t *best_child = get_max_action(tree);
  atomic_fetch_add(&(best_child->label.n_won), -5);
  atomic_fetch_add(&(best_child->label.n_visited), 1);

  twixt_play(board, player, best_child->label.move);
  outcome_t outcome = descend(board, best_child, opt, player == RED ? BLACK : RED);
  int ov = get_outcome_value(outcome, opt);
  atomic_fetch_add(&(best_child->label.n_won), 5+ov);
  return outcome;
}

children_container_t *expand_leaf(const board_t *board, player_t player, int *count) {
  assert(board != NULL);
  bool *moves = twixt_available_moves(board, player, count);
  children_container_t *children = calloc(*count,sizeof(children_container_t));
  if (children == NULL) return NULL;
  int n = 0;
  int s = twixt_size(board);
  for (int i = 0; i < s; i++) {
    for (int j = 0; j < s; j++) {
      if (!moves[j * s + i]) continue;
      tree_t *tree = mc_create_tree();
      tree->label.move = (position_t){i, j};
      children[n].this = tree;
      children[n].protected = false;
      n++;
    }
  }
  free(moves);
  return children;
}

tree_t *get_max_visits(tree_t *tree) {
  assert(tree != NULL);
  tree_t *best = NULL;
  int best_value = INT_MIN;
  for (int i = 0; i < tree->children_length; i++) {
    tree_t *child = tree->children[i].this;
    int v = atomic_load(&(child->label.n_visited));
    if (v > best_value || best == NULL) {
      best_value = v;
      best = child;
    }
  }
  return best;
}

int random_int(int max) {
  return (int) (random() % (long) max);
}

void shuffle_children(tree_t *tree)
{
  for (int i = 0; i < tree->children_length; i++) {
    children_container_t tmp = tree->children[i];
    int j = random_int(i+1);
    tree->children[i] = tree->children[j];
    tree->children[j] = tmp;
  }
}

void expand(board_t *board, tree_t *tree, player_t player) {
  assert(tree != NULL);
  tree_t *cursor = tree;
  while (cursor->children_length > 0) {
    cursor = get_max_visits(cursor);
    twixt_play(board, player, cursor->label.move);
    player = player == RED ? BLACK : RED;
  }
  cursor->children = expand_leaf(board, player, &(cursor->children_length));
  shuffle_children(cursor);
}

typedef struct {
  const board_t *board;
  tree_t *tree;
  player_t opt;
  player_t player;
} descend_args_t;

void *descend_wrapper(void *data) {
  descend_args_t args = *(descend_args_t *) data;
  board_t *board = twixt_copy(args.board);
  descend(board, args.tree, args.opt, args.player);
  twixt_free(&board);
  return NULL;
}

void step(int nt, const board_t *board, tree_t *tree, player_t player) {
  pthread_t *pool = malloc(sizeof(pthread_t) * nt);
  assert(pool != NULL);
  descend_args_t data = {.board = board, .tree = tree, .opt = player, .player = player};

  for (int i = 0; i < nt; i++) {
    pthread_create(&pool[i], NULL, descend_wrapper, &data);
  }
  for (int i = 0; i < nt; i++) {
    pthread_join(pool[i], NULL);
  }
  free(pool);
  board_t *b2 = twixt_copy(board);
  expand(b2, tree, player);
  twixt_free(&b2);
}

tree_t *get_max(tree_t *tree) {
  assert(tree != NULL);
  children_container_t *best = NULL;
  int best_value = INT_MIN;

  for (int i = 0;i < tree->children_length; i++) {
    int v = atomic_load(&(tree->children[i].this->label.n_visited));
    if (v > best_value || best == NULL) {
      best_value = v;
      best = &(tree->children[i]);
    }
  }
  best->protected = true;
  return best->this;
}

position_t mc_search(int ns, int nt, const board_t *board, tree_t **tree, player_t player) {
  assert(tree != NULL && *tree != NULL);
  for (int s = 0; s < ns; s++) {
    step(nt, board, *tree, player);
    printf("[LOG] Stepped %d/%d\n", s + 1, ns);
  }
  tree_t *best = get_max(*tree);
  mc_free_tree(tree);
  *tree = best;
  return (*tree)->label.move;
}

void mc_advance_tree(tree_t **tree, position_t move) {
  tree_t *keep = *tree;
  for (int i = 0; i < keep->children_length; i++) {
    position_t l_move = keep->children[i].this->label.move;
    if (l_move.x == move.x && l_move.y == move.y) {
      keep->children[i].protected = true;
      keep = keep->children[i].this;
      break;
    }
  }
  if (keep == *tree) {
    keep = mc_create_tree();
    keep->label.move = move;
  }
  mc_free_tree(tree);
  *tree = keep;
}
