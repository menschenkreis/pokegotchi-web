#ifndef PG_RETAINED_H
#define PG_RETAINED_H
#include "checkpoint.h"

enum { PG_RETAINED_CAPSULE_BYTES=132 };
typedef enum { PG_RETAINED_READY, PG_RETAINED_CLEAR_CAPSULE, PG_RETAINED_RECOVERY } pg_retained_result;
bool pg_retained_capsule_epoch(const uint8_t *capsule,size_t length,uint64_t *epoch);
/* Reconstruct a capsule for an already committed OPEN/COMPLETE migration token. */
bool pg_retained_rebuild(const pg_checkpoint *current,const pg_device_data *device,uint32_t device_sequence,
                         int64_t wall_us,double base_elapsed,uint8_t *capsule,size_t capacity);
/* Build capsule and commit OPEN before the caller installs it in retained memory. */
bool pg_retained_begin(const pg_tutorial_reset_io *io,pg_checkpoint *current,
                       const pg_device_data *device,uint32_t device_sequence,
                       uint64_t session,uint64_t start_us,int64_t wall_us,uint8_t *capsule,size_t capacity);
/* `present` means a capsule was retained, including malformed data. */
pg_retained_result pg_retained_recover(const pg_tutorial_reset_io *io,pg_checkpoint *current,
                                        const uint8_t *capsule,size_t length,bool present,int64_t now_wall_us);
/* Call after clearing the retained capsule. */
bool pg_retained_finish(const pg_tutorial_reset_io *io,pg_checkpoint *current);
#endif
