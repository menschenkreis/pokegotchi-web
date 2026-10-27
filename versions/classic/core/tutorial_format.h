#ifndef PG_TUTORIAL_FORMAT_H
#define PG_TUTORIAL_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { PG_TUTORIAL_LIFE_V6_BYTES = 724, PG_TUTORIAL_LIFE_V7_BYTES = 728,
       PG_TUTORIAL_DEVICE_V2_BYTES = 76, PG_TUTORIAL_RESET_V2_BYTES = 864 };

/* Structural framing checks only. Call the full life/device validators as well. */
bool pg_tutorial_life_v7_frame_valid(const uint8_t *life, size_t length);
bool pg_tutorial_life_v7_from_v6(const uint8_t *v6, size_t length,
                                 uint32_t stage, uint8_t *v7, size_t capacity);
bool pg_tutorial_reset_v2_frame_valid(const uint8_t *record, size_t length);
bool pg_tutorial_reset_v2_frame_make(const uint8_t *life, size_t life_length,
                                     const uint8_t *device, size_t device_length,
                                     uint32_t state, uint64_t epoch, uint64_t operation,
                                     uint64_t transaction, uint32_t entry,
                                     uint8_t *record, size_t capacity);
#endif
