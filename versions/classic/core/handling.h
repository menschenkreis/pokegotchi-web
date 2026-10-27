#ifndef PG_HANDLING_H
#define PG_HANDLING_H
#include <stdbool.h>
#include <stdint.h>
struct pg_game;
typedef struct {float accel[3],gyro[3];uint64_t at_us;bool accel_valid,gyro_valid;} pg_imu_sample;
typedef struct {
    uint64_t last_us,cooldown_us,cosmetic_us,shake_at,spin_at,circle_at;
    float gravity[3],spin_angle,circle_angle,last_circle,backtrack;
    unsigned impulses,circle_samples;
    int shake_axis,shake_sign,spin_sign,context;
    bool initialized,gravity_ready,circle_ready;
    uint32_t gestures;int last_gesture,last_recipient;
} pg_handling;
void pg_handling_reset(struct pg_game *);
void pg_handling_sample(struct pg_game *,const pg_imu_sample *);
void pg_handling_event(struct pg_game *,bool shake);
#endif
