//
// Created by lucas on 13/06/2025.
//

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "serializer.h"
#include "twixt.h"

// Credit : https://dotat.at/@/2023-06-21-pcg64-dxsm.html
typedef struct pcg64 {
    __uint128_t state, inc;
} pcg64_t;

uint64_t _random_pull(pcg64_t *rng) {
    /* cheap (half-width) multiplier */
    const uint64_t mul = 15750249268501108917ULL;
    /* linear congruential generator */
    __uint128_t state = rng->state;
    rng->state = state * mul + rng->inc;
    /* DXSM (double xor shift multiply) permuted output */
    uint64_t hi = (uint64_t)(state >> 64);
    uint64_t lo = (uint64_t)(state | 1);
    hi ^= hi >> 32;
    hi *= mul;
    hi ^= hi >> 48;
    hi *= lo;
    return(hi);
}

pcg64_t _random_seed(pcg64_t rng) {
    /* must ensure rng.inc is odd */
    rng.inc = (rng.inc << 1) | 1;
    rng.state += rng.inc;
    _random_pull(&rng);
    return(rng);
}
// End credit

pcg64_t _random_init() {
    return _random_seed((pcg64_t){ 1 });
}

size_t no_pos_for_zoom(uint8_t zoom) {
    return 1 + 2 * zoom * (zoom - 1);
}

uint64_t *make_bitstrings(const uint8_t zoom) {
    size_t s = no_pos_for_zoom(zoom) * (1 + 2 * 256); // 1 for empty, 2 * 256 for every link configuration possible for every player
    uint64_t *bitstrings = malloc(s);
    pcg64_t rng = _random_init();
    if (bitstrings == NULL) return NULL;
    for (int i = 0; i < s; i++) bitstrings[i] = _random_pull(&rng);
    return bitstrings;
}

size_t get_position_index(int8_t dx, int8_t dy) {
    uint8_t d = abs(dx) + abs(dy);
    if (d == 0) return 0;
    size_t start = 1 + 2 * d * (d - 1);
    size_t offset;
    if (d == 0) offset= 0;

    if (dx == d) offset= dy + d;
    if (dy == d) offset= 3*d - dx;
    if (dx == -d) offset= 5*d + dy;

    offset= 7*d + dx;
    return start + offset;
}

size_t get_bitstring_index(int8_t deltax, int8_t deltay, player_t player, uint8_t links) {
    assert(player == 1 || player == 2);

    size_t pos_index = get_position_index(deltax, deltay);
    size_t player_bit = player - 1;

    size_t index = (((pos_index * 2) + player_bit) * 256) + links;

    return index;
}

uint64_t hash(position_t pos, board_t *board, uint8_t zoom, const uint64_t *bitstrings)
{
    uint64_t ret = 0;
    for (int8_t i = -((int8_t) zoom); i <= ((int8_t) zoom); i++) {
        for (int8_t j = -((int8_t) zoom); j <= ((int8_t) zoom); j++) {
            if (abs(i) + abs(j) > zoom) continue;
            node_t *peek = twixt_peek(pos, board);
            ret ^= bitstrings[get_bitstring_index(i, j, peek->player, peek->links)];
        }
    }
    return ret;
}

uint64_t update_hash(uint64_t hash, position_t pos, player_t player, uint8_t links, const uint64_t *bitstrings) {
    return hash ^ (bitstrings[get_bitstring_index(0, 0, NONE, 0)]) ^ bitstrings[get_bitstring_index(0, 0, player, links)];
}

typedef struct observation_s {
    uint bv; // visited (black)
    uint bc; // chosen (black)
    uint rv; // visited (red)
    uint rc; // chosen (red)
} observation_t;

typedef struct zobrist_s {
    size_t nt;
    uint8_t zc;
    observation_t **observations; // first array : per zoom level, second : every observation (by-hash)
    uint64_t **zoom_bitstrings;

} zobrist_t;

zobrist_t *zobrist_create(int nt, int zoom_count) {
    zobrist_t *zobrist = malloc(sizeof(zobrist_t));
    if (zobrist == NULL) return NULL;
    zobrist->zc = zoom_count;
    zobrist->nt = nt;
    zobrist->observations = malloc(zoom_count * sizeof(observation_t *));
    if (zobrist->observations == NULL) return NULL;
    zobrist->zoom_bitstrings = malloc(zoom_count * sizeof(uint64_t*));
    if (zobrist->zoom_bitstrings == NULL) return NULL;
    for (int i = 1; i <= zoom_count; i++) {
        zobrist->observations[i] = malloc(nt * sizeof(observation_t));
        zobrist->zoom_bitstrings[i] = make_bitstrings(i);
    }
    return zobrist;
}

void zobrist_destroy(zobrist_t *zobrist) {
    if (zobrist == NULL) return;
    for (int i =  0;i<zobrist->zc; i++) {
        free(zobrist->observations[i]);
        free(zobrist->zoom_bitstrings[i]);
    }
    free(zobrist);
}

float zobrist_evaluate(zobrist_t *zobrist, position_t position, player_t player, board_t *board) {
    assert(player != NONE);
    assert(board != NULL);
    assert(zobrist != NULL);
    uint total_visited = 0;
    uint total_chosen = 0;
    for (int zoom = zobrist->zc; zoom > 0; zoom--) {
        uint64_t current_hash = hash(position, board, zoom, zobrist->zoom_bitstrings[zoom]);
        observation_t *observation = zobrist->observations[current_hash % zobrist->nt];
        if (player == BLACK && observation->bv != 0) {
            return (float) observation->bc / (float) observation->bv;
        }

        if (player == RED && observation->rv != 0) {
            return (float) observation->rc / (float) observation->rv;
        }
    }
    assert(false); // should not happen, very weird bug
}

void zobrist_update(zobrist_t *zobrist, game_t *game) {
    assert(zobrist != NULL);
    moves_t *move = game->moves;
    board_t *board = twixt_create(game->size);
    while (move != NULL) {
        if (move->type != PEG) continue;
        if (move->next == NULL || move->next->type != PEG) continue;
        for (int zoom = zobrist->zc; zoom > 0; zoom--) {
            uint64_t current_hash = hash(move->peg,game->board, zoom, zobrist->zoom_bitstrings[zoom]);
            observation_t obs = zobrist->observations[zoom][current_hash % zobrist->nt];
            if (move->player == BLACK) obs.rv++;
            if (move->player == RED) obs.bv++;
        }
    }
}
char *zobrist_serialize(zobrist_t *zobrist);
zobrist_t *zobrist_deserialize(char *buf, char **end);
