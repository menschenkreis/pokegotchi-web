#ifndef PG_TUTORIAL_RESET_H
#define PG_TUTORIAL_RESET_H

#include "tutorial_format.h"

/* An adapter must implement durable replacement and checked readback. */
typedef enum { PG_RESET_READ_OK, PG_RESET_READ_ABSENT, PG_RESET_READ_ERROR } pg_reset_read_result;
typedef enum { PG_RESET_RECORD, PG_RESET_LIFE, PG_RESET_DEVICE, PG_RESET_CHECKPOINT, PG_RESET_CKMODE } pg_reset_object;
typedef struct {
    void *context;
    pg_reset_read_result (*read)(void *context, pg_reset_object object, unsigned slot,
                                 uint8_t *bytes, size_t capacity, size_t *length);
    bool (*write_durable)(void *context, pg_reset_object object, unsigned slot,
                          const uint8_t *bytes, size_t length);
    /* Full semantic checks are required in addition to structural frame checks. */
    bool (*validate_payload)(void *context, const uint8_t *life, size_t life_length,
                             const uint8_t *device, size_t device_length);
    bool checkpoints;
} pg_tutorial_reset_io;

typedef enum { PG_RESET_NONE, PG_RESET_APPLIED, PG_RESET_RECOVERY_REQUIRED } pg_tutorial_reset_result;

/* Never exposes a world: callers must separately load and validate current-epoch lives. */
pg_tutorial_reset_result pg_tutorial_reset_resume(const pg_tutorial_reset_io *io);
/* Query accepted boundary metadata after resume; absent means epoch and operation zero. */
bool pg_tutorial_reset_boundary(const pg_tutorial_reset_io *io,uint64_t *epoch,uint64_t *operation);
/* The candidate must be a complete validated COMMITTED v2 frame in separate storage. */
pg_tutorial_reset_result pg_tutorial_reset_begin(const pg_tutorial_reset_io *io,
                                                   const uint8_t *candidate, size_t length);
#endif
