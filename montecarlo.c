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

typedef struct tree_list_s {
  tree_t *this;
  struct tree_list_s *next;
  int length;
  bool protected;
} tree_list_t;

typedef struct tree_s {
  label_t label;
  tree_list_t *children;
} tree_t;

tree_list_t *mc_tl_create(tree_t *this) {
  tree_list_t *new = malloc(sizeof(tree_list_t));
  if (new == NULL) return NULL;
  new->this = this;
  new->next = NULL;
  new->length = 1;
  new->protected = false;
  return new;
}

tree_list_t *mc_tl_append(tree_list_t *tl, tree_t *tree) {
  assert(tree != NULL);
  tree_list_t *new = mc_tl_create(tree);
  if (tl == NULL) return new;
  new->next = tl;
  new->length = tl->length + 1;
  return new;
}

int mc_tl_length(const tree_list_t *tl) {
  if (tl == NULL) return 0;
  return tl->length;
}

void mc_tl_destroy(tree_list_t *tl) {
  assert(tl != NULL);
  tree_list_t *cursor = tl;
  while (cursor != NULL) {
    tree_list_t *next = cursor->next;
    if (!cursor->protected) {
      mc_destroy_tree(cursor->this);
      free(cursor);
    } else {
      free(cursor);
    }
    cursor = next;
  }
}

tree_t *mc_create_tree() {
  tree_t *t = malloc(sizeof(tree_t));
  if (t == NULL) return NULL;
  t->children = NULL;
  atomic_store(&(t->label.n_visited), 0);
  atomic_store(&(t->label.n_won), 0);
  t->label.prior = 0.5;
  t->label.move = (position_t){0, 0};
  return t;
}

void mc_destroy_tree(tree_t *tree) {
  assert(tree != NULL);
  if (tree->children == NULL) return;
  mc_tl_destroy(tree->children);
  free(tree);
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

outcome_t continue_strategy(board_t *board, player_t player) {
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
  if ((o == RED_WINS && opt == RED) || (o == BLACK_WINS && opt == BLACK)) return 2;
  if (o == DRAW) return -1;
  if (o == ONGOING) return 0;
  return -2;
}

tree_t *get_max_action(tree_list_t *tl) {
  assert(tl != NULL);
  tree_list_t *cursor = tl;
  tree_list_t *best = tl;
  double best_value = mc_calculate_action(tl->this);
  while (cursor != NULL) {
    double av = mc_calculate_action(cursor->this);
    if (av > best_value) {
      best_value = av;
      best = cursor;
    }
    cursor = cursor->next;
  }
  return best->this;
}

outcome_t descend(board_t *board, const tree_t *tree, player_t opt, player_t player) {
  assert(tree != NULL);
  if (mc_tl_length(tree->children) == 0) {
    return continue_strategy(board, player);
  }

  tree_t *best_child = get_max_action(tree->children);
  atomic_fetch_add(&(best_child->label.n_won), -5);  atomic_fetch_add(&(best_child->label.n_visited), 1);

  twixt_play(board, player, best_child->label.move);
  outcome_t outcome = descend(board, best_child, opt, player == RED ? BLACK : RED);
  int ov = get_outcome_value(outcome, opt);
  atomic_fetch_add(&(best_child->label.n_won), 5+ov);
  return outcome;
}

tree_list_t *expand_leaf(const board_t *board, player_t player) {
  assert(board != NULL);
  bool *moves = twixt_available_moves(board, player);
  tree_list_t *tl = NULL;
  int s = twixt_size(board);
  for (int i = 0; i < s; i++) {
    for (int j = 0; j < s; j++) {
      if (!moves[j * s + i]) continue;
      tree_t *tree = mc_create_tree();
      tree->label.move = (position_t){i, j};
      tl = mc_tl_append(tl, tree);
    }
  }
  return tl;
}

tree_t *get_max_visits(tree_list_t *tl) {
  assert(tl != NULL);
  tree_list_t *cursor = tl;
  tree_list_t *best = tl;
  int best_value = INT_MIN;
  while (cursor != NULL) {
    int v = atomic_load(&(cursor->this->label.n_visited));
    if (v > best_value) {
      best_value = v;
      best = cursor;
    }
    cursor = cursor->next;
  }
  return best->this;
}

int random_int(int max) {
  return (int) (random() % (long) max);
}

tree_list_t *knuth_shuffle(tree_list_t *tl)
{
  if (tl == NULL) return tl;
  int length = tl->length;
  tree_t **arrayed_list = malloc(sizeof(tree_t*) * length);
  bool *arrayed_list_protected = malloc(sizeof(bool) * length);
  int i = 0;
  for (tree_list_t *cursor = tl; cursor != NULL; cursor = cursor->next) {
    arrayed_list[i] = cursor->this;
    arrayed_list_protected[i] = cursor->protected;
    cursor->protected = true;
    i++;
  }
  for (i = 0; i < length; i++) {
    tree_t *tmp = arrayed_list[i];
    int j = random_int(i+1);
    arrayed_list[i] = arrayed_list[j];
    arrayed_list[j] = tmp;
    bool tmp2 = arrayed_list_protected[i];
    arrayed_list_protected[i] = arrayed_list[j];
    arrayed_list_protected[j] = tmp2;
  }

  mc_tl_destroy(tl);
  tl = NULL;
  for (i = 0; i < length; i++) {
    tl = mc_tl_append(tl, arrayed_list[i]);
    tl->protected = arrayed_list_protected[i];
  }

  free(arrayed_list_protected);
  free(arrayed_list);
  return tl;
}

void expand(board_t *board, tree_t *tree, player_t player) {
  assert(tree != NULL);
  tree_t *cursor = tree;
  while (mc_tl_length(cursor->children) > 0) {
    cursor->children = knuth_shuffle(cursor->children);
    cursor = get_max_visits(cursor->children);
    twixt_play(board, player, cursor->label.move);
    player = player == RED ? BLACK : RED;
  }
  cursor->children = expand_leaf(board, player);
  cursor->children = knuth_shuffle(cursor->children);
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
  twixt_destroy(board);
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
  twixt_destroy(b2);
}

tree_t *get_max(tree_list_t *tl) {
  assert(tl != NULL);
  tree_list_t *cursor = tl;
  tree_list_t *best = tl;
  int best_value = INT_MIN;
  while (cursor != NULL) {
    int v = atomic_load(&(cursor->this->label.n_visited));
    if (v > best_value) {
      best_value = v;
      best = cursor;
    }
    cursor = cursor->next;
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
  tree_t *best = get_max((*tree)->children);
  mc_destroy_tree(*tree);
  *tree = best;
  return (*tree)->label.move;
}

void mc_advance_tree(tree_t **tree, position_t move) {
  tree_list_t *cursor = (*tree)->children;
  tree_t *keep = NULL;
  while (cursor != NULL) {
    position_t l_move = cursor->this->label.move;
    if (l_move.x == move.x && l_move.y == move.y) {
      cursor->protected = true;
      keep = cursor->this;
      break;
    }
    cursor = cursor->next;
  }
  if (keep == NULL) {
    keep = mc_create_tree();
    keep->label.move = move;
  }
  mc_destroy_tree(*tree);
  *tree = keep;
}
