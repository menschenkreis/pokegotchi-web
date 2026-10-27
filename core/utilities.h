#ifndef PG_UTILITIES_H
#define PG_UTILITIES_H
#include "device.h"
struct pg_game;
typedef enum {PG_ACTIVITY_TOUCH_DOWN,PG_ACTIVITY_TOUCH_MOVE,PG_ACTIVITY_HELD_CONTACT,PG_ACTIVITY_BUTTON,PG_ACTIVITY_GESTURE,PG_ACTIVITY_TILT_CHANGE} pg_activity_kind;
typedef struct {uint64_t at_us;uint32_t session,round,score;int screen,stage,event,detail,gesture,recipient;unsigned attempts;} pg_diagnostic_entry;
typedef struct {
    pg_diagnostic_entry trace[64];
    uint32_t trace_count;
    pg_time_source source;
    pg_battery_snapshot battery;
    bool initialized,writable,prefs_dirty,best_dirty,checkpoint,storage_fault;
    double idle,preference_age,contact_age,contact_beat;
    float activity_x,activity_y,last_tilt_x,last_tilt_y;
    uint32_t revision;
    int settings_page,return_page,clock_field;
    bool editing,clock_error;
    pg_calendar edit;
} pg_device_runtime;
void pg_trace_event(struct pg_game *,int event,int detail);
void pg_device_attach(struct pg_game *,const pg_device_data *,bool writable);
void pg_device_changed(struct pg_game *);
void pg_utilities_step(struct pg_game *,pg_time_source);
void pg_user_activity(struct pg_game *,pg_activity_kind);
bool pg_audio_enabled(const struct pg_game *);
void pg_pcm_gain(int16_t *,size_t,unsigned);
bool pg_calendar_to_seconds(const pg_calendar *,int64_t *);
bool pg_calendar_from_seconds(int64_t,pg_calendar *);
bool pg_calendar_save(struct pg_game *,const pg_calendar *);
bool pg_calendar_now(const struct pg_game *,pg_calendar *);
void pg_clock_begin(struct pg_game *);
void pg_clock_adjust(struct pg_game *,int);
#endif
