#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include"twixt.h"
#include <stdio.h>

board_t *twixt_create(int size) {
  board_t *board = malloc(sizeof(board_t));
  if (board == NULL) return NULL;
  board->size = size;
  board->data = calloc(size*size, sizeof(node_t));
  if (board->data == NULL) {
    free(board);
    return NULL;
  }
  board->placed_moves = 0;
  return board;
}

void twixt_free(board_t **board) {
  if (board == NULL || *board == NULL) return;
  free((*board)->data);
  free(*board);
  *board = NULL;
}

int twixt_size(const board_t *board) {
  return board->size;
}

board_t *twixt_copy(const board_t *board) {
  if (board == NULL) return NULL;
  board_t *copy = malloc(sizeof(board_t));
  if (copy == NULL) return NULL;
  copy->size = board->size;
  copy->placed_moves = board->placed_moves;
  copy->data = malloc(sizeof(node_t) * board->size * board->size);
  if (copy->data == NULL) {
    free(copy);
    return NULL;
  }
  memcpy(copy->data, board->data, sizeof(node_t) * board->size * board->size);
  return copy;
}

position_t get_coords(int size, int index) {
  position_t p = {.x = index % size, .y = index / size};
  return p;
}


int get_index(int size, position_t p) {
  return p.y * size + p.x;
}
bool is_valid_coordinate(int size, position_t p) {
  return p.x >= 0 && p.y >= 0 && p.x < size && p.y < size;
}

node_t *twixt_peek(position_t p, const board_t *board) {
  assert(is_valid_coordinate(board->size, p));
  return &board->data[get_index(board->size, p)];
}

position_t deltas[8] = {{1, -2}, {2, -1}, {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}};

bool has_link(const board_t *board, position_t p, int index) {
  return (board->data[get_index(board->size, p)].links & (1<<index)) != 0;
}

bool is_valid_move(const board_t *board, position_t p) {
  assert(board != NULL);
  return is_valid_coordinate(board->size, p) && board->data[get_index(board->size, p)].player == NONE;
}

bool is_move_in_player_bounds(int size, player_t player, position_t p) {
  if (player == RED) {
    return p.x != 0 && p.x != size - 1;
  }
  return p.y != 0 && p.y != size - 1;
}

bool *twixt_available_moves(const board_t *board, player_t player, int *count) {
  assert(board != NULL);
  bool *data = calloc(board->size * board->size, sizeof(bool));
  if (data == NULL) return NULL;
  *count = 0;
  for (int i = 0; i < board->size; i++) {
    for (int j = 0; j < board->size; j++) {
      position_t pos = (position_t){i, j};
      if (board->data[get_index(board->size, pos)].player == NONE // ensure cell is empty
          && is_move_in_player_bounds(board->size, player, pos)) {
        data[get_index(board->size, pos)] = true;
        (*count)++;
      }
    }
  }
  return data;
}
// S = placed_moves U cant_access_moves
position_t twixt_random_move(const board_t *board, [[maybe_unused]] player_t player) {
  /*int q = rand() % (board->size * board->size - board->placed_moves); // NOLINT(cert-msc30-c, cert-msc50-cpp)
  int c = 0;
  for (int i = 0; i <= q; i++) c += (is_move_in_player_bounds(board->size, player, get_coords(board->size, i)) && board->data[i].player != NONE);
  while (c > 0) {
    q++;
    if (
      board->data[q].player == NONE
      && !is_move_in_player_bounds(board->size, player, get_coords(board->size, q)))
      c--;
  }
  return get_coords(board->size, q);*/
  int available_moves[board->size * board->size]; // Stores empty move indices
  int count = 0;

  // Collect all unplayed moves
  for (int i = 0; i < board->size * board->size; i++) {
    if (board->data[i].player == NONE) {
      available_moves[count++] = i;
    }
  }

  // If no moves are available, return an invalid position
  if (count == 0) {
    position_t invalid = {-1, -1}; // Define an invalid position
    return invalid;
  }

  // Choose a random available move
  int rand_index = rand() % count;
  return get_coords(board->size, available_moves[rand_index]);
  /*int N = board->size * board->size - (2 * board->size) - board->placed_moves;
  int available_move[N];
  int j = 0;
  printf("%d\n", board->placed_moves);
  for (int i = 0; i < board->size * board->size; i++) {
    if (is_move_in_player_bounds(board->size, player, get_coords(board->size, i))
      && is_valid_move(board, get_coords(board->size, i))) {
      available_move[j] = i;
      j++;
    }
  }
  int index = rand() % N;
  return get_coords(board->size, available_move[index]);*/
}

bool place_move(const board_t *board, player_t player, position_t p) {
  assert(board != NULL);
  if (!is_valid_move(board, p) || !is_move_in_player_bounds(board->size, player, p)) return false;
  board->data[get_index(board->size, p)].player = player;
  return true;
}

bool ccw (position_t p1, position_t p2, position_t p3) {
  return (p3.y-p1.y)*(p2.x-p1.x) > (p2.y-p1.y)*(p3.x-p1.x);
}

bool line_intersects(position_t a1, position_t a2, position_t b1, position_t b2) {
  return (ccw(a1, b1, b2) != ccw(a2,b1,b2)) &&  (ccw(a1, a2, b1) != ccw(a1, a2, b2));
}

bool check_other_links(const board_t *board, position_t p1, position_t p2) {
  int mini, maxi;
  int minj, maxj;
  if (p1.x < p2.x) {
    mini = p1.x-1;
    maxi = p2.x+1;
  }
  else {
    maxi = p1.x+1;
    mini = p2.x-1;
  }
  if (p1.y < p2.y) {
    minj = p1.y-1;
    maxj = p2.y+1;
  }
  else {
    maxj = p1.y+1;
    minj = p2.y-1;
  }
  node_t center_node = board->data[get_index(board->size, p1)];
  for (int i = mini >= 0 ? mini : 0; i <= (maxi < board->size ? maxi : board->size -1); i++) {
    for (int j = minj >= 0 ? minj : 0; j <= (maxj < board->size ? maxj : board->size - 1); j++) {
      position_t current = {i,j};
      node_t current_node = board->data[get_index(board->size, current)];
      for (int k = 0;k < 8; k++) {
        position_t next = {i+deltas[k].x, j+deltas[k].y};
        if (!is_valid_coordinate(board->size, next)) continue;
        if (!has_link(board, current, k)) continue;
        if (current_node.player != center_node.player && line_intersects(p1, p2, current, next)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool create_link(const board_t *board, int delta_index, position_t p1) {
  assert(board != NULL);
  position_t p2 = {p1.x + deltas[delta_index].x, p1.y + deltas[delta_index].y};
  if (!is_valid_coordinate(board->size, p2) || !is_valid_coordinate(board->size, p1)) return false;
  if (board->data[get_index(board->size, p1)].player != board->data[get_index(board->size, p2)].player) return false;
  if (check_other_links(board, p1, p2)) return false;
  board->data[get_index(board->size, p1)].links += 1 << delta_index;
  board->data[get_index(board->size, p2)].links += 1 << ((delta_index + 4) % 8);

  return true;
}

void create_links(const board_t *board, const position_t p) {
  assert(board != NULL);
  if (!is_valid_coordinate(board->size, p)) return;
  for (int i = 0; i < 8; i++) {
    create_link(board, i, p);
  }
}

bool aux(const board_t *board, player_t player, bool visited[board->size][board->size], const position_t p)
{
  visited[p.x][p.y] = true;
  if(player == BLACK && p.x == board->size - 1) return true;
  if(player == RED && p.y == board->size - 1) return true;
  for(int i = 0; i < 8; i++) {
    if(has_link(board, p, i)) {
      const position_t neighbour = {p.x + deltas[i].x, p.y + deltas[i].y};
      if(!is_valid_coordinate(board->size,neighbour) || visited[neighbour.x][neighbour.y] || board->data[get_index(board->size, neighbour)].player != player) continue;
      if(aux(board, player, visited, neighbour)) return true;
    }
  }
  return false;
}

bool dfs(const board_t *board, player_t player, const position_t start)
{
  assert(board != NULL);
  bool visited[board->size][board->size];
  for (int i = 0; i < board->size; i++) {
    for (int j = 0; j < board->size; j++) {
      visited[i][j] = false;
    }
  }
  return aux(board, player, visited, start);
}

outcome_t twixt_check_winner(const board_t *board) {
  for (int i = 1; i < board->size-1; i++) {
    if(board->data[get_index(board->size, (position_t){i,0})].player == RED && dfs(board, RED, (position_t){i, 0})) return RED_WINS;
    if(board->data[get_index(board->size, (position_t){0,i})].player == BLACK && dfs(board, BLACK, (position_t){0, i})) return BLACK_WINS;
  }
  if(board->placed_moves == board->size * board->size-4) return DRAW;
  return ONGOING;
}

bool twixt_play(board_t *board, player_t player, position_t p) {
  assert(board != NULL);
  if (place_move(board, player, p) == false) return false;
  create_links(board, p);
  board->placed_moves = board->placed_moves + 1;
  return true;
}

void twixt_swap(board_t *board) {
  assert(board != NULL);
  // TODO
}