//
// Created by lucas on 02/06/2025.
//
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
void metadata_free(void *data, metadata_provider_t metadata_provider) {
    if (metadata_provider == TWIXTLIVE) {
        free(data);
    }


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
        free(gt);
        return NULL;
    }
    gt->provider = provider;
    cursor = endptr;
    cursor++;
    // WIP
    return gt;
}
char *game_serialize(game_t *game);
void game_free(game_t *game);
