#ifndef PG_CHIP_H
#define PG_CHIP_H
#include "game.h"
#define PG_CHIP_RATE 16000
#define PG_CHIP_NOTES 12
/* Original one-voice handheld scores. No heap, files, or audio backend required. */
typedef struct { int8_t midi; uint16_t ms; } pg_chip_note;
typedef struct {
    pg_chip_note notes[PG_CHIP_NOTES];
    uint32_t offset, length;
    float phase, step;
    uint8_t count, cursor, voice;
    bool active;
} pg_chip;
void pg_chip_start(pg_chip *s, pg_sound sound, pg_species species, uint32_t variation);
size_t pg_chip_read(pg_chip *s, int16_t *out, size_t samples);
int pg_sound_priority(pg_sound sound);
#endif
