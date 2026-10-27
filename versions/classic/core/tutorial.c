#include "tutorial.h"
#include <math.h>

bool pg_tutorial_stage_valid(uint32_t stage) {
    return stage <= PG_TUTORIAL_DELIVERY;
}

bool pg_tutorial_restore(pg_tutorial *t, uint32_t stage) {
    if (!t || !pg_tutorial_stage_valid(stage)) return false;
    t->committed = (pg_tutorial_stage)stage;
    t->requested = t->committed;
    t->commit = PG_TUTORIAL_IDLE;
    t->descent_seconds = 0;
    return true;
}

static bool request_stage(pg_tutorial *t, pg_tutorial_stage next) {
    if (!t || t->commit != PG_TUTORIAL_IDLE) return false;
    t->requested = next;
    t->commit = PG_TUTORIAL_COMMITTING;
    return true;
}

bool pg_tutorial_action_received(pg_tutorial *t, pg_tutorial_action action) {
    if (!t || t->commit != PG_TUTORIAL_IDLE) return false;
    if (t->committed == PG_TUTORIAL_SWIPE && action == PG_TUTORIAL_SWIPE_ACCEPTED)
        return request_stage(t, PG_TUTORIAL_LAMP_NIGHT);
    if (t->committed == PG_TUTORIAL_LAMP_NIGHT && action == PG_TUTORIAL_LAMP_NIGHT_ACCEPTED)
        return request_stage(t, PG_TUTORIAL_LAMP_DAY);
    if (t->committed == PG_TUTORIAL_LAMP_DAY && action == PG_TUTORIAL_LAMP_DAY_ACCEPTED)
        return request_stage(t, PG_TUTORIAL_DELIVERY);
    return false;
}

void pg_tutorial_tick(pg_tutorial *t, float seconds, bool standby) {
    if (!t || t->committed != PG_TUTORIAL_DELIVERY || t->commit != PG_TUTORIAL_IDLE ||
        standby || !isfinite(seconds) || seconds <= 0) return;
    t->descent_seconds = fminf(PG_TUTORIAL_DESCENT_SECONDS, t->descent_seconds + seconds);
    if (t->descent_seconds >= PG_TUTORIAL_DESCENT_SECONDS)
        (void)request_stage(t, PG_TUTORIAL_LANDED);
}

bool pg_tutorial_commit_requested(const pg_tutorial *t, pg_tutorial_stage *stage) {
    if (!t || !stage || t->commit != PG_TUTORIAL_COMMITTING) return false;
    *stage = t->requested;
    return true;
}

bool pg_tutorial_resolve_commit(pg_tutorial *t, pg_tutorial_commit_result result) {
    if (!t || t->commit != PG_TUTORIAL_COMMITTING) return false;
    if (result == PG_TUTORIAL_COMMIT_OK) {
        t->committed = t->requested;
        t->commit = PG_TUTORIAL_IDLE;
        t->descent_seconds = 0;
        return true;
    }
    if (result == PG_TUTORIAL_COMMIT_FAILED) {
        t->commit = PG_TUTORIAL_RETRY;
        return true;
    }
    if (result == PG_TUTORIAL_COMMIT_UNCERTAIN) {
        t->commit = PG_TUTORIAL_RECOVER;
        return true;
    }
    return false;
}

bool pg_tutorial_retry(pg_tutorial *t) {
    if (!t || t->commit != PG_TUTORIAL_RETRY) return false;
    t->commit = PG_TUTORIAL_COMMITTING;
    return true;
}
