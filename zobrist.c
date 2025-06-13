#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "serializer.h"
#include "twixt.h"
#include "rand.h"

pcg64_t zobrist_random_init() {
    return random_seed((pcg64_t){ 1 });
}

size_t no_pos_for_zoom(uint8_t zoom) {
    return 1 + 2 * zoom * (zoom - 1);
}

uint64_t *make_bitstrings(const uint8_t zoom) {
    size_t s = no_pos_for_zoom(zoom) * (1 + 2 * 256); // 1 for empty, 2 * 256 for every link configuration possible for every player
    uint64_t *bitstrings = malloc(s);
    pcg64_t rng = zobrist_random_init();
    if (bitstrings == NULL) return NULL;
    for (int i = 0; i < s; i++) bitstrings[i] = random_pull(&rng);
    return bitstrings;
}

size_t get_position_index(int8_t dx, int8_t dy) {
    const uint8_t d = abs(dx) + abs(dy);
    if (d == 0) return 0;
    const size_t start = 1 + 2 * d * (d - 1);
    size_t offset;
    if (d == 0) offset= 0;
    else if (dx == d) offset= dy + d;
    else if (dy == d) offset= 3*d - dx;
    else if (dx == -d) offset= 5*d + dy;
    else offset= 7*d + dx;
    return start + offset;
}

size_t get_bitstring_index(int8_t deltax, int8_t deltay, player_t player, uint8_t links) {
    assert(player == 1 || player == 2);

    const size_t pos_index = get_position_index(deltax, deltay);
    const size_t player_bit = player - 1;

    const size_t index = (pos_index * 2 + player_bit) * 256 + links;

    return index;
}
uint64_t hash(position_t pos, const board_t *board, uint8_t zoom, const uint64_t *bitstrings)
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

typedef struct hashset_s {
    int size;
    uint64_t *hashes;
} hashset_t;

hashset_t *hashset_create(int size) {
    hashset_t *set = malloc(sizeof(hashset_t));
    if (set == NULL) return NULL;
    set->size = size;
    set->hashes = malloc(size * sizeof(uint64_t));
    return set;
};
void hashset_free(hashset_t **set) {
    if (set == NULL || *set == NULL) return;
    free((*set)->hashes);
    free(*set);
    *set = NULL;
}
void hashset_update(hashset_t *set, board_t *board, position_t *position, player_t *player);

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

void zobrist_free(zobrist_t **zobrist) {
    if (zobrist == NULL || *zobrist == NULL) return;
    for (int i =  0;i<(*zobrist)->zc; i++) {
        free((*zobrist)->observations[i]);
        free((*zobrist)->zoom_bitstrings[i]);
    }
    free(*zobrist);
    *zobrist = NULL;
}

float zobrist_evaluate(const zobrist_t *zobrist, position_t position, player_t player, const board_t *board) {
    assert(player != NONE);
    assert(board != NULL);
    assert(zobrist != NULL);
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

void zobrist_populate(const zobrist_t *zobrist, const game_t *game) {
    assert(zobrist != NULL);
    const moves_t *move = game->moves;
    board_t *board = twixt_create(game->size);
    while (move != NULL) {
        if (move->type == SWAP) {
            twixt_swap(board);
        }
        if (move->type != PEG) {
            move = move->next;
            continue;
        };
        twixt_play(board, move->player, move->peg);
        if (move->next == NULL || move->next->type != PEG) {
            move = move->next->next;
            continue;
        }
        for (int zoom = zobrist->zc; zoom > 0; zoom--) {
            uint64_t current_hash = hash(move->peg,board, zoom, zobrist->zoom_bitstrings[zoom]);
            observation_t obs = zobrist->observations[zoom][current_hash % zobrist->nt];
            if (move->player == BLACK) obs.rv++;
            if (move->player == RED) obs.bv++;
            if (move->next->player == BLACK) obs.bc++;
            if (move->next->player == RED) obs.rc++;

        }
        twixt_play(board, move->next->player, move->next->peg);
        move = move->next->next;
    }
    twixt_free(&board);
}
char *zobrist_serialize(const zobrist_t *zobrist) { // TODO : à tester
    assert(zobrist != NULL);
    size_t total_size = sizeof(uint64_t) + sizeof(uint8_t);

    for (uint8_t i = 0; i < zobrist->zc; i++) {
        total_size += zobrist->nt * sizeof(observation_t);
    }

    char *buffer = malloc(total_size+1);
    if (!buffer) return NULL;

    char *p = buffer;

    memcpy(p, &zobrist->nt, sizeof(zobrist->nt));
    p += sizeof(zobrist->nt);

    memcpy(p, &zobrist->zc, sizeof(zobrist->zc));
    p += sizeof(zobrist->zc);

    for (uint8_t i = 0; i < zobrist->zc; i++) {
        memcpy(p, zobrist->observations[i], zobrist->nt * sizeof(observation_t));
        p += zobrist->nt * sizeof(observation_t);
    }

    *p = '\0';
    return buffer;
}
zobrist_t *zobrist_deserialize(char *buf) {
    if (!buf) return NULL;

    char *p = buf;

    uint64_t nt;
    char *o = strncpy((char*)&nt, p, sizeof(nt));
    p += sizeof(nt);
    if (o != p) return NULL;

    uint8_t zc;
    o = strncpy((char*)&zc, p, sizeof(zc));
    p += sizeof(zc);
    if (o != p) return NULL;

    zobrist_t *z = malloc(sizeof(zobrist_t));
    if (!z) return NULL;
    z->nt = nt;
    z->zc = zc;

    z->observations = malloc(zc * sizeof(observation_t*));
    z->zoom_bitstrings = malloc(zc * sizeof(uint64_t*));
    if (!z->observations || !z->zoom_bitstrings) {
        free(z->observations);
        free(z->zoom_bitstrings);
        free(z);
        return NULL;
    }

    for (uint8_t i = 0; i < zc; i++) { // TODO : s'assurer que c'est **safe**
        z->observations[i] = malloc(nt * sizeof(observation_t));
        if (!z->observations[i]) {
            for (uint8_t j = 0; j < i; j++) free(z->observations[j]);
            free(z->observations);
            free(z->zoom_bitstrings);
            free(z);
            return NULL;
        }
        o = strncpy((char*)z->observations[i], p, nt * sizeof(observation_t));
        p += nt * sizeof(observation_t);
        if (o != p) {
            for (uint8_t j = 0; j <= i; j++) free(z->observations[j]);
            free(z->observations);
            for (uint8_t j = 0; j < i; j++) free(z->zoom_bitstrings[j]);
            free(z->zoom_bitstrings);
            free(z);
            return NULL;
        }
        z->zoom_bitstrings[i] = make_bitstrings(i);
        if (!z->zoom_bitstrings[i]) {
            for (uint8_t j = 0; j <= i; j++) free(z->observations[j]);
            free(z->observations);
            for (uint8_t j = 0; j < i; j++) free(z->zoom_bitstrings[j]);
            free(z->zoom_bitstrings);
            free(z);
            return NULL;
        }
    }
    return z;
}