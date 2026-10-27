#ifndef PG_MOTION_H
#define PG_MOTION_H
#include <stdbool.h>
/* Input is QMI8658 sensor-frame g and degrees/second. Output is screen-frame:
   +x right, +y down. C-board mounting: screen right = sensor +Y,
   screen down = sensor +X (Waveshare Immersive Block reference). */
typedef struct {
    float gravity[3], anchor[3], gyro_bias[3], gyro_sum[3];
    float calibration_left, calibration_time, neutral_roll, neutral_pitch;
    float x, y;
    unsigned samples, gyro_samples;
    bool gravity_ready, centered;
} pg_motion;
void pg_motion_recenter(pg_motion *m);
bool pg_motion_sample(pg_motion *m,float ax,float ay,float az,
                      float gx,float gy,float gz,float dt);
#endif
