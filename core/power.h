#ifndef PG_POWER_H
#define PG_POWER_H
#include <stdbool.h>
#include <stdint.h>
/* Active-high PWR signal. Consume the button that booted/woke the device until
   released; a held wake button must never immediately send it back to sleep. */
typedef struct {bool down,armed;uint64_t changed_at;} pg_power_button;
void pg_power_button_init(pg_power_button *b,bool level,uint64_t now);
bool pg_power_button_sample(pg_power_button *b,bool level,uint64_t now);
#endif
