#include"serializer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

metadata_twixtlive_t *metadata_twixtlive_deserialize(char *buf, char **end)
{
    metadata_twixtlive_t *mt = malloc(sizeof(metadata_twixtlive_t));
    if (mt == NULL) return NULL;
    char *cursor = buf;
    if (*cursor != 'i') {
        free(mt);
        return NULL;
    }
    cursor++;
    char *endptr = cursor;
    long int gid = strtol(cursor, &endptr, 10);
    if (endptr == cursor || *endptr != ',' || gid < 0 || gid > 1000000000 ) {
        free(mt);
        return NULL;
    };
    cursor = endptr;
    long int timestamp = strtol(cursor, &endptr, 10);
    if (endptr == cursor || *endptr != ',' || timestamp < 0 || timestamp > 1000000000) {
        free(mt);
        return NULL;
    }
    cursor = endptr;
    long int white_id = strtol(cursor, &endptr, 10);
    if (endptr == cursor || *endptr != ',' || white_id < 0 || white_id > 1000000000) {
        free(mt);
        return NULL;
    }
    cursor = endptr;
    long int black_id = strtol(cursor, &endptr, 10);
    if (*endptr != ',' || black_id < 0 || black_id > 1000000000) {
        free(mt);
        return NULL;
    }
    if (endptr == cursor) {
        black_id = -1;
    }
    *end = endptr;
    mt->blackid = black_id;
    mt->gid = gid;
    mt->timestamp = timestamp;
    mt->whiteid = white_id;
    return mt;
}
char *metadata_twixtlive_serialize(metadata_twixtlive_t *twixtlive)
{
    char *buf = malloc(55);
    if (twixtlive->blackid == -1)
        sprintf(buf,"twixtlive,i%ld,t%ld,w%ld",twixtlive->gid, twixtlive->timestamp, twixtlive->whiteid);
    else sprintf(buf, "twixtlive,i%ld,t%ld,w%ld,b%ld",twixtlive->gid, twixtlive->timestamp, twixtlive->whiteid, twixtlive->blackid);
    return buf;
}

metadata_provider_t metadata_deserialize(char *buf, void **dst, char **end)
{
    if (strncmp("twixtlive,", buf, 10) == 0) {
        *dst = metadata_twixtlive_deserialize(buf + 10, end);
        return TWIXTLIVE;
    }
    *dst = NULL;
    *end = buf;
    return -1;
}
char *metadata_serialize(void *data, metadata_provider_t metadata_provider) {
    if (metadata_provider == TWIXTLIVE) {
        return metadata_twixtlive_serialize(data);
    }
    return NULL;
}
void metadata_free(void **data, metadata_provider_t metadata_provider) {
    if (metadata_provider == TWIXTLIVE) {
        if (data == NULL || *data == NULL) return;
        free(*data);
        *data = NULL;
    }
}

moves_t *moves_create(move_type_t type, player_t player, int red_pos, int black_pos) {
    moves_t *moves = malloc(sizeof(moves_t));
    if (moves == NULL) return NULL;
    moves->player = player;
    if (moves->type == PEG) {
        moves->peg.y = red_pos;
        moves->peg.x = black_pos;
    } else {
        moves->peg.y = -1;
        moves->peg.x = -1;
    }
    return moves;
}
moves_t *moves_append(moves_t *before, moves_t *new) {
    if (before == NULL) return new;
    if (new == NULL) return before;
    new->next = before;
    return new;
}

void moves_free(moves_t **moves) {
    if (moves == NULL || *moves == NULL) return;
    moves_t *cur = *moves;
    while (cur != NULL) {
        moves_t *next = cur->next;
        free(*moves);
        cur = next;
    }
    *moves = NULL;
}

game_t *game_deserialize(char *buf) {
    game_t *gt = malloc(sizeof(game_t));
    if (gt == NULL) return NULL;
    char *cursor = buf;
    char *endptr = cursor;
    long int size = strtol(cursor, &endptr, 10);
    if (endptr == cursor || *endptr != ',' || size < 0 || size > 1000000000 ) {
        free(gt);
        return NULL;
    }
    metadata_provider_t provider = metadata_deserialize(endptr, &(gt->metadata), &endptr);
    if (provider == -1) {
        //free(gt);
        //return NULL;
        // je pense qu'on peut le garder comme ça
    }
    gt->provider = provider;
    cursor = endptr;
    cursor++;
    player_t player = -1;
    int red_id = 0;
    int black_id = 0;
    int step = 0;
    int substep = 1;
    while (cursor != NULL && *cursor != '\0')
    {
        if (*cursor == ',') {
            cursor++;
            continue;
        }
        if (step == 0 && *cursor == 'R') {
            player = RED;
            step++;
            cursor++;
            continue;
        }
        if (step == 0 && *cursor == 'B') {
            player = BLACK;
            step++;
            cursor++;
            continue;
        }

        if (step == 1 && *cursor == '[') {
            step++;
            cursor++;
            continue;
        }

        if (step == 1 && *cursor == 'R') {
            gt->moves = moves_append(gt->moves, moves_create(RESIGN, player, -1, -1));
            break;
        }

        if (step == 1 && *cursor == 'W') {
            gt->moves = moves_append(gt->moves, moves_create(WIN, player, -1, -1));
            break;
        }
        if (step == 2) {
            if (*cursor >= 'A' && *cursor <= 'Z') {
                // cursor is a caps letter
                red_id += substep * (*cursor - 64);
                substep*=26;
                cursor++;
                continue;
            }
            if (red_id == -1) {
                game_free(gt);
                return NULL;
            }
            if (*cursor >= '0' && *cursor <= '9') {
                if (red_id < 0 || red_id > size) {
                    free(gt);
                    return NULL;
                }
                long int tmp = strtol(cursor, &endptr, 10);
                if (tmp < 0 || tmp > size) {
                    game_free(gt);
                    return NULL;
                }
                black_id = (int)tmp;
                step++;
                substep = 1;
                cursor = endptr;
                continue;
            }
        }

        if (step == 3 && *cursor == ']') {
            step = 0;
            cursor++;
            gt->moves = moves_append(gt->moves, moves_create(PEG, player, red_id, black_id));
            continue;
        }
        game_free(gt);
        return NULL;
    }
    return gt;
}
char *game_serialize(game_t *game) {
    return "WIP";
}
void game_free(game_t **game) {
    if (game == NULL || *game == NULL) return;
    moves_free(&(*game)->moves);
    if ((*game)->board != NULL) twixt_free(&(*game)->board);
    free(*game);
    *game = NULL;
}