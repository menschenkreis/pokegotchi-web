#ifndef PG_TUTORIAL_LIFE_H
#define PG_TUTORIAL_LIFE_H

#include "game.h"
#include "tutorial.h"

#define PG_TUTORIAL_NEED_SCALE UINT64_C(1728000000)
#define PG_TUTORIAL_NEED_MAX UINT64_C(172800000000)
#define PG_TUTORIAL_EPISODE_MAX_US UINT64_C(129600000000)

typedef struct {
    pg_pet pet;
    uint64_t epoch, episode_us, food_fixed, energy_fixed, clean_fixed;
    double fractional_us;
    uint32_t policy;
    bool episode_active;
    pg_tutorial_stage stage;
} pg_tutorial_life;

/* Fully validates legacy v1–v5, revised v6 and tutorial v7 life records. */
bool pg_tutorial_life_decode(pg_tutorial_life *life,uint32_t *sequence,const uint8_t *bytes,size_t length);
size_t pg_tutorial_life_encode_v7(const pg_tutorial_life *life,uint32_t sequence,uint8_t *bytes,size_t capacity);
bool pg_tutorial_life_from_pet(pg_tutorial_life *life,const pg_pet *pet,uint64_t epoch,pg_tutorial_stage stage);
bool pg_tutorial_life_update_pet(pg_tutorial_life *life,const pg_pet *pet);
bool pg_tutorial_device_v2_decode(pg_device_data *device,uint32_t *sequence,uint64_t *epoch,
                                  const uint8_t *bytes,size_t length);
size_t pg_tutorial_device_v2_encode(const pg_device_data *device,uint32_t sequence,uint64_t epoch,
                                    uint8_t *bytes,size_t capacity);
/* Normalize a checked legacy reset-v1 record to equivalent reset-v2 framing. */
bool pg_tutorial_reset_v1_normalize(const uint8_t *bytes,size_t length,uint8_t *out,size_t capacity);
#endif
