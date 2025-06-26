#include"serializer.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"game_serializer.tab.h"
char *metadata_twixtlive_serialize(metadata_twixtlive_t *twixtlive)
{
    char *buf = malloc(55);
    if (twixtlive->blackid == -1)
        sprintf(buf,"twixtlive,i%ld,t%ld,w%ld",twixtlive->gid, twixtlive->timestamp, twixtlive->whiteid);
    else sprintf(buf, "twixtlive,i%ld,t%ld,w%ld,b%ld",twixtlive->gid, twixtlive->timestamp, twixtlive->whiteid, twixtlive->blackid);
    return buf;
}

char *metadata_serialize(void *data, metadata_provider_t metadata_provider) {
    if (metadata_provider == PROVIDER_TWIXTLIVE) {
        return metadata_twixtlive_serialize(data);
    }
    return NULL;
}

void metadata_free(void **data, metadata_provider_t metadata_provider) {
    if (metadata_provider == PROVIDER_TWIXTLIVE) {
        if (data == NULL || *data == NULL) return;
        free(*data);
        *data = NULL;
    }
}

moves_t *moves_create(move_type_t type, player_t player, int red_pos, int black_pos) {
    moves_t *move = malloc(sizeof(moves_t));
    if (move == NULL) return NULL;
    move->type = type;
    move->player = player;
    if (type == PEG) {
        move->peg.y = red_pos;
        move->peg.x = black_pos;
    } else {
        move->peg.y = -1;
        move->peg.x = -1;
    }
    move->next = NULL;
    return move;
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
        free(cur);
        cur = next;
    }
    *moves = NULL;
}

extern game_t *y_game_deserialize(char *);
game_t *game_deserialize([[maybe_unused]] char *buf) {
    return y_game_deserialize(buf);
}

board_t *game_make_board(game_t *game) {
    board_t *board = twixt_create(game->size);
    moves_t *move = game->moves;
    while (move != NULL) {
        if (move->type == SWAP)
            twixt_swap(board);
        if (move->type == PEG)
            twixt_play(board, move->player, move->peg);
        move = move->next;
    }
    game->board = board;
    return board;
}

char *game_serialize([[maybe_unused]] game_t *game) {
    return "WIP";
}

void game_free(game_t **game) {
    if (game == NULL || *game == NULL) return;
    if ((*game)->moves != NULL) moves_free(&(*game)->moves);
    if ((*game)->board != NULL) twixt_free(&(*game)->board);
    if ((*game)->metadata != NULL) metadata_free(&(*game)->metadata, (*game)->provider);
    free(*game);
    *game = NULL;
}