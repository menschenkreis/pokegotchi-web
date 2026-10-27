#ifndef PG_CHECKPOINT_H
#define PG_CHECKPOINT_H
#include "tutorial_life.h"
#include "tutorial_reset.h"

enum { PG_CHECKPOINT_BYTES=868, PG_CKMODE_BYTES=24, PG_SCENE_BYTES=56 };
typedef struct {
    pg_tutorial_life life;
    pg_sleep_pose scene;
    uint64_t sequence;
    uint32_t life_sequence;
    uint32_t sleep_flags, capsule_base_sequence;
    uint64_t capsule_session, applied_us, total_us, capsule_start_us;
} pg_checkpoint;
enum { PG_CK_IDLE=0, PG_CK_OPEN=1, PG_CK_COMPLETE=2, PG_CK_BOUND=4 };
typedef enum { PG_CHECKPOINT_LEGACY, PG_CHECKPOINT_VALID, PG_CHECKPOINT_RECOVERY } pg_checkpoint_result;

bool pg_checkpoint_encode(const pg_checkpoint *c,uint8_t *out,size_t capacity);
bool pg_checkpoint_decode(pg_checkpoint *c,const uint8_t *bytes,size_t length);
/* Select and repair before returning VALID. Invalid authoritative records fail closed. */
pg_checkpoint_result pg_checkpoint_load(const pg_tutorial_reset_io *io,uint64_t epoch,pg_checkpoint *out);
/* Recover a partially initialized fresh store only after legacy absence is established. */
pg_checkpoint_result pg_checkpoint_resume_fresh(const pg_tutorial_reset_io *io,pg_checkpoint *out);
/* Activate only after both new checkpoint copies are verified; marker commits migration. */
bool pg_checkpoint_activate(const pg_tutorial_reset_io *io,const pg_checkpoint *initial);
bool pg_checkpoint_commit(const pg_tutorial_reset_io *io,pg_checkpoint *current,const pg_tutorial_life *life,const pg_sleep_pose *scene);
/* Commit a complete next checkpoint, including an OPEN/COMPLETE sleep token. */
bool pg_checkpoint_commit_full(const pg_tutorial_reset_io *io,pg_checkpoint *current,const pg_checkpoint *desired);
/* Called only while a COMMITTED reset is replayed, before APPLIED. */
bool pg_checkpoint_install_reset(const pg_tutorial_reset_io *io,const uint8_t *life,size_t length);
#endif
