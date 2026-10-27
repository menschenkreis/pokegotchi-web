#ifndef PG_TUTORIAL_H
#define PG_TUTORIAL_H

#include <stdbool.h>
#include <stdint.h>

/* Values are the approved life-save stage values; storage integration is separate. */
typedef enum {
    PG_TUTORIAL_LANDED = 0,
    PG_TUTORIAL_SWIPE = 1,
    PG_TUTORIAL_LAMP_NIGHT = 2,
    PG_TUTORIAL_LAMP_DAY = 3,
    PG_TUTORIAL_DELIVERY = 4
} pg_tutorial_stage;

typedef enum {
    PG_TUTORIAL_SWIPE_ACCEPTED,
    PG_TUTORIAL_LAMP_NIGHT_ACCEPTED,
    PG_TUTORIAL_LAMP_DAY_ACCEPTED
} pg_tutorial_action;

typedef enum {
    PG_TUTORIAL_IDLE,
    PG_TUTORIAL_COMMITTING,
    PG_TUTORIAL_RETRY,
    PG_TUTORIAL_RECOVER
} pg_tutorial_commit_state;

typedef enum {
    PG_TUTORIAL_COMMIT_OK,
    PG_TUTORIAL_COMMIT_FAILED,
    PG_TUTORIAL_COMMIT_UNCERTAIN
} pg_tutorial_commit_result;

typedef struct {
    pg_tutorial_stage committed;
    pg_tutorial_stage requested;
    pg_tutorial_commit_state commit;
    float descent_seconds;
} pg_tutorial;

#define PG_TUTORIAL_DESCENT_SECONDS 3.0f

bool pg_tutorial_stage_valid(uint32_t stage);
bool pg_tutorial_restore(pg_tutorial *tutorial, uint32_t durable_stage);
bool pg_tutorial_action_received(pg_tutorial *tutorial, pg_tutorial_action action);
void pg_tutorial_tick(pg_tutorial *tutorial, float seconds, bool standby);
bool pg_tutorial_commit_requested(const pg_tutorial *tutorial, pg_tutorial_stage *stage);
bool pg_tutorial_resolve_commit(pg_tutorial *tutorial, pg_tutorial_commit_result result);
bool pg_tutorial_retry(pg_tutorial *tutorial);

#endif
