#ifndef PG_DEVICE_H
#define PG_DEVICE_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define PG_DEVICE_BYTES 68
#define PG_UTILITY_CAPSULE_BYTES 112
#define PG_CALENDAR_END INT64_C(3155760000)
typedef enum { PG_STORE_ABSENT,PG_STORE_VALID,PG_STORE_RECOVERY_REQUIRED } pg_store_result;
typedef struct {
    uint32_t echo_best;
    unsigned volume;
    bool auto_sleep,clock_valid;
    int64_t clock_seconds;
    uint64_t anchor_us,source_session;
} pg_device_data;
typedef struct {uint64_t session,now_us;bool valid;} pg_time_source;
typedef struct {int year,month,day,hour,minute,second;} pg_calendar;
typedef struct {bool valid,present,charging,synthetic;unsigned percent;uint64_t sampled_us;} pg_battery_snapshot;
typedef struct {pg_device_data device;uint32_t device_sequence,life_sequence;uint64_t session,now_us;int64_t wall_us;} pg_utility_capsule;
void pg_device_defaults(pg_device_data *d);
size_t pg_device_encode(const pg_device_data *d,uint32_t sequence,uint8_t *out,size_t cap);
bool pg_device_decode(pg_device_data *d,uint32_t *sequence,const uint8_t *in,size_t len);
size_t pg_utility_capsule_encode(const pg_utility_capsule *c,uint8_t *out,size_t cap);
bool pg_utility_capsule_decode(pg_utility_capsule *c,const uint8_t *in,size_t len);
bool pg_battery_decode(pg_battery_snapshot *out,uint8_t status1,uint8_t status2,uint8_t gauge,uint8_t percent,uint64_t at);
#endif
