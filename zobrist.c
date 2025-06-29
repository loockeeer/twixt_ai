#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "twixt.h"
#include "rand.h"

pcg64_t zobrist_random_init() {
    return random_seed((pcg64_t){ 10592837502938, 5938201948 });
}

uint64_t *make_bitstrings(const unsigned char zoom) {
    int s = (1+2*zoom)*(1+2*zoom) * 3 * 256; // 1 for empty, 2 * 256 for every link configuration possible for every player
    uint64_t *bitstrings = malloc(s * sizeof(uint64_t));
    if (bitstrings == NULL) return NULL;
    pcg64_t rng = zobrist_random_init();
    for (int i = 0; i < s; i++) bitstrings[i] = random_pull(&rng);
    return bitstrings;
}

static unsigned get_bitstring_index(const unsigned char zoom, const int deltax, const int deltay, const player_t player, const unsigned char links) {
    const int deltax_shift = deltax+zoom;
    const int deltay_shift = deltay+zoom;
    const int D0 = 1 + 2 * zoom;
    constexpr int D1 = 3;
    constexpr int D2 = 256;
    return ((deltax_shift * D0 + deltay_shift) * D1 + player) * D2 + links;
}

uint64_t hash(const position_t pos, const board_t *board, const unsigned char zoom, const uint64_t *bitstrings)
{
    uint64_t ret = 0;
    for (int i = -zoom;i<=zoom;i++) {
        for (int j = -zoom;j<=zoom;j++) {
            if (abs(i) + abs(j) > zoom) continue;
            position_t p = {.x=pos.x + i, .y=pos.y + j};
            if (0 > p.x || 0 > p.y || p.x >= board->size || p.y >= board->size) continue;
            node_t *peek = twixt_peek(p, board);
            ret ^= bitstrings[get_bitstring_index(zoom, i, j, peek->player, peek->links)];
        }
    }
    return ret;
}

typedef struct observation_s {
    unsigned int bv; // visited (black)
    unsigned int bc; // chosen (black)
    unsigned int rv; // visited (red)
    unsigned int rc; // chosen (red)
} observation_t;

typedef struct zobrist_s {
    unsigned int nt;
    unsigned char zc;
    observation_t **observations; // first array : per zoom level, second : every observation (by-hash)
    uint64_t **zoom_bitstrings;
} zobrist_t;

zobrist_t *zobrist_create(const unsigned nt, const unsigned zoom_count) {
    zobrist_t *zobrist = malloc(sizeof(zobrist_t));
    if (zobrist == NULL) return NULL;
    zobrist->zc = zoom_count;
    zobrist->nt = nt;
    zobrist->observations = malloc(zoom_count * sizeof(observation_t *));
    if (zobrist->observations == NULL) return NULL;
    zobrist->zoom_bitstrings = malloc(zoom_count * sizeof(uint64_t *));
    if (zobrist->zoom_bitstrings == NULL) return NULL;
    for (unsigned i = 0; i < zoom_count; i++) {
        zobrist->observations[i] = calloc(nt, sizeof(observation_t));
        zobrist->zoom_bitstrings[i] = make_bitstrings(i);
    }
    return zobrist;
}

void zobrist_free(zobrist_t **zobrist) {
    if (zobrist == NULL || *zobrist == NULL) return;
    for (unsigned i =  0;i<(*zobrist)->zc; i++) {
        free((*zobrist)->observations[i]);
        free((*zobrist)->zoom_bitstrings[i]);
    }
    free((*zobrist)->observations);
    free((*zobrist)->zoom_bitstrings);
    free(*zobrist);
    *zobrist = NULL;
}

float zobrist_evaluate(const zobrist_t *zobrist, const position_t position, const player_t player, const board_t *board) {
    assert(player != NONE);
    assert(board != NULL);
    assert(zobrist != NULL);
    for (unsigned zoom = zobrist->zc; zoom > 0; zoom--) {
        uint64_t current_hash = hash(position, board, zoom, zobrist->zoom_bitstrings[zoom-1]);
        observation_t observation = zobrist->observations[zoom-1][current_hash % zobrist->nt];
        if (player == BLACK && observation.bv != 0) {
            return (float) observation.bc / (float) observation.bv;
        }

        if (player == RED && observation.rv != 0) {
            return (float) observation.rc / (float) observation.rv;
        }
    }
    return .0f;
}

position_t zobrist_best_move(const zobrist_t *zobrist, const player_t player, const board_t *board) {
    assert(player != NONE);
    assert(board != NULL);
    assert(zobrist != NULL);
    position_t best_pos = {0,0};
    float best_score = 0.0f;
    int count;
    bool *moves = twixt_available_moves(board, player, &count);
    for (int i = 0; i < twixt_size(board); i++) {
        for (int j = 0; j < twixt_size(board); j++) {
            if (!moves[twixt_size(board) * j + i]) continue;
            position_t current = {i,j};
            float score = zobrist_evaluate(zobrist, current, player, board);
            if (score > best_score) {
                best_pos = current;
                best_score = score;
            }
        }
    }
    return best_pos;
}

void zobrist_populate(const zobrist_t *zobrist, const game_t *game) {
    assert(zobrist != NULL);
    const moves_t *move = game->moves;
    board_t *board = twixt_create(game->size);
    while (move != NULL) {
        if (move->type == SWAP) {
            twixt_swap(board);
            move = move->next;
            continue;
        }
        if (move->type != PEG) {
            move = move->next;
            continue;
        }

        for (unsigned zoom = zobrist->zc; zoom > 0; zoom--) {
            for (int i = 0;i<game->size;i++) {
                for (int j = 0;j<game->size;j++) {
                    position_t p = {i,j};
                    node_t *peek = twixt_peek(p, board);
                    if (peek->player != NONE) continue;
                    uint64_t current_hash = hash(p, board, zoom-1, zobrist->zoom_bitstrings[zoom-1]);
                    observation_t *obs = &zobrist->observations[zoom-1][current_hash % zobrist->nt];
                    if (move->player == BLACK) obs->rv++;
                    if (move->player == RED) obs->bv++;
                    if (move->next != NULL && move->next->peg.x == p.x && move->next->peg.y == p.y) {
                        if (move->next->player == BLACK) {
                            obs->bv++;
                            obs->rc++;
                        }
                        if (move->next->player == RED) {
                            obs->rv++;
                            obs->bc++;
                        }
                    }
                }
            }
        }

        twixt_play(board, move->player, move->peg);
        move = move->next;
    }
    twixt_free(&board);
}



void bytes_write(unsigned char **cur, const void *src, size_t size) {
    memcpy(*cur, src, size);
    *cur += size;
}

void observation_serialize(unsigned char **cur, const observation_t observation) {
    bytes_write(cur, &observation.bv, sizeof(unsigned int));
    bytes_write(cur, &observation.bc, sizeof(unsigned int));
    bytes_write(cur, &observation.rv, sizeof(unsigned int));
    bytes_write(cur, &observation.rc, sizeof(unsigned int));
}

observation_t observation_deserialize(const unsigned int *buf) {
    assert(buf != NULL);
    observation_t observation;
    observation.bv = buf[0];
    observation.bc = buf[1];
    observation.rv = buf[2];
    observation.rc = buf[3];
    return observation;
}

unsigned char *zobrist_serialize(const zobrist_t *zobrist, unsigned long *length) {
    assert(zobrist != NULL);
    *length = sizeof(unsigned int) + sizeof(unsigned char) + 4 * sizeof(unsigned int) *  zobrist->nt * zobrist->zc;
    unsigned char *buffer = malloc(*length);
    unsigned char *cur = buffer;
    bytes_write(&cur, &zobrist->nt, sizeof(unsigned int));
    bytes_write(&cur, &zobrist->zc, sizeof(unsigned char));
    for (unsigned char zoom = 0; zoom < zobrist->zc; zoom++) {
        for (unsigned int t = 0; t < zobrist->nt; t++) {
            observation_serialize(&cur, zobrist->observations[zoom][t]);
        }
    }
    return buffer;
}

zobrist_t *zobrist_deserialize(unsigned char *buf, unsigned long length) {
    unsigned char *cur = buf;
    unsigned char *end = buf+length;
    unsigned int nt;
    unsigned char zc;
    if (cur + sizeof(unsigned int) > end) return NULL;
    memcpy(&nt, cur, sizeof(unsigned int));
    cur += sizeof(unsigned int);
    if (cur + sizeof(unsigned char) > end) return NULL;
    memcpy(&zc, cur, sizeof(unsigned char));
    zobrist_t *zobrist = zobrist_create(nt, zc);

    cur += sizeof(unsigned char);
    for (unsigned char zoom = 0;zoom<zobrist->zc;zoom++) {
        for (unsigned int t = 0; t < zobrist->nt; t++) {
            if (cur + 4 * sizeof(unsigned int) > end) {
                zobrist_free(&zobrist);
                return NULL;
            }
            observation_t observation = observation_deserialize((unsigned int *)cur);
            cur += 4 * sizeof(unsigned int);
            zobrist->observations[zoom][t] = observation;
        }
    }
    return zobrist;
}