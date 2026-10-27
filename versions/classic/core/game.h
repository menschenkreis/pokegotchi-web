#ifndef PG_GAME_H
#define PG_GAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "device.h"
#include "utilities.h"
#include "handling.h"

#define PG_W 466
#define PG_H 466
#define PG_EVENT_CAP 32
#define PG_HOUR 3600.0
#define PG_DAY 86400.0
#define PG_EGG_SECONDS (7.0 * 60.0)
#define PG_SAVE_MAX 8192
#define PG_VERSION "0.3.0-test.11"
#define PG_NAME_MAX 6

typedef enum { PG_EGG, PG_HATCHING, PG_ALIVE, PG_DEAD } pg_phase;
typedef enum { PG_MOSSLET, PG_EMBERKIN, PG_LUNAPIP, PG_SPECIES_COUNT } pg_species;
typedef enum { PG_BABY, PG_YOUNG, PG_ADULT } pg_stage;
typedef enum { PG_REQUEST_NONE, PG_REQUEST_DEV_RESET, PG_REQUEST_SUCCESSOR } pg_reset_request;
typedef enum { PG_EAT, PG_REST, PG_EXPLORE, PG_INSPECT, PG_PLAY, PG_ATTENTION,
               PG_INVITE, PG_GOAL_COUNT } pg_goal;
typedef enum { PG_HOME, PG_CARD, PG_FOOD, PG_CARE, PG_GAMES, PG_TTT,
               PG_TILT, PG_RHYTHM, PG_ALBUM, PG_SETTINGS, PG_MEMORIAL,
               PG_ECHO, PG_HELP, PG_BALANCE, PG_WASH, PG_DEBUG, PG_CLOCK_EDIT, PG_VOLUME, PG_DEVICE_STATUS, PG_NAME_EDIT, PG_SCREEN_COUNT } pg_screen;
#define PG_GAME_COUNT 6
typedef enum { PG_EV_FOUND, PG_EV_WARMED, PG_EV_HATCHED, PG_EV_FED, PG_EV_PETTED,
               PG_EV_CLEANED, PG_EV_SLEPT, PG_EV_WON, PG_EV_LOST, PG_EV_DRAW,
               PG_EV_GREW, PG_EV_DISCOVERY, PG_EV_FAVORITE, PG_EV_CLAP,
               PG_EV_DIED, PG_EV_SUCCESSOR } pg_event_kind;
typedef enum { PG_SOUND_NONE, PG_SOUND_TAP, PG_SOUND_HAPPY, PG_SOUND_FOOD,
               PG_SOUND_WARM, PG_SOUND_HATCH, PG_SOUND_WIN, PG_SOUND_LOSE,
               PG_SOUND_NOTE0, PG_SOUND_NOTE1, PG_SOUND_NOTE2,
               PG_SOUND_HELLO, PG_SOUND_CUDDLE, PG_SOUND_CURIOUS, PG_SOUND_SLEEP,
               PG_SOUND_CLEAN, PG_SOUND_GRUMBLE, PG_SOUND_COUNT } pg_sound;
typedef enum { PG_EMOTE_NONE, PG_EMOTE_GREET, PG_EMOTE_CUDDLE, PG_EMOTE_NIBBLE,
               PG_EMOTE_CURIOUS, PG_EMOTE_INVITE, PG_EMOTE_CHEER, PG_EMOTE_OOPS,
               PG_EMOTE_CLEAN, PG_EMOTE_YAWN, PG_EMOTE_SASS, PG_EMOTE_WOBBLE, PG_EMOTE_FRIGHT, PG_EMOTE_COUNT } pg_emote;

typedef struct { uint8_t kind, arg; double at; } pg_event;

/* Persistent game data. Save format is explicit little endian, not this ABI. */
typedef struct {
    uint32_t rng, identity, generation, species;
    pg_phase phase;
    double elapsed, age, incubation, last_egg_visit;
    float satiety, energy, clean, affection, health, trust;
    float warmth, comfort;
    float bold, curious, playful;
    float food_preference[3], toy_preference[3];
    float confidence[PG_GAME_COUNT];
    uint32_t meals, pets, games_played[PG_GAME_COUNT], games_won[PG_GAME_COUNT], discoveries;
    uint32_t egg_visits, milestones, legacy_count;
    uint8_t plant_style, bed_style, toy_style, legacy_species;
    bool sheltered, death_acknowledged, muted, naming_pending;
    uint8_t brightness; /* 20..100 percent; saved since v4 */
    char name[16];
    char legacy_name[16];
    pg_event events[PG_EVENT_CAP];
    uint32_t event_count;
    /* Precise care state; serialized by the versioned life codec. */
    uint64_t care_food, care_energy, care_clean, neglect_us;
    double care_fraction_us;
    bool care_initialized, neglect_active;
} pg_pet;

typedef struct {
    int8_t cells[9]; /* 1 player, -1 creature, 0 empty */
    int8_t turn, winner;
    bool finished, pet_starts;
    float think,look_time;
    int last_cell;
    uint32_t round;
} pg_ttt;

typedef struct {
    float x, y, vx, vy, target_x, target_y;
    float seconds, hold, ready;
    int score;
    bool finished, fell;
} pg_tilt;

typedef struct {
    uint8_t notes[16];
    int length, cursor, score, lit;
    float timer;
    bool showing, finished;
} pg_rhythm;

typedef enum { PG_SHAKE, PG_SWIPE, PG_TAP } pg_gesture;
typedef enum { PG_ECHO_OFF, PG_ECHO_CUE, PG_ECHO_GO, PG_ECHO_INPUT,
               PG_ECHO_PASS, PG_ECHO_RETRY, PG_ECHO_DONE } pg_echo_stage;
typedef struct {
    uint32_t session, round, pointer_round;
    pg_echo_stage stage;
    pg_gesture prompt;
    uint8_t sequence[3], bag_index, attempts, impulses;
    uint32_t score, prompt_rng;
    bool begun,finalized;
    float response_seconds;
    float timer, shake_gap, shake_cooldown;
    float gravity[3];
    int shake_axis, shake_sign;
    bool audio_wait, muted, pointer_armed, gravity_ready;
} pg_echo;

typedef struct {
    float angle, velocity, seconds, steady, gust, gust_timer, ready, rescue, neutral;
    unsigned rescues;
    bool started, finished;
} pg_balance;
#define PG_FOAM_SIDE 12
#define PG_FOAM_COUNT (PG_FOAM_SIDE*PG_FOAM_SIDE)
typedef struct {
    uint8_t foam[PG_FOAM_COUNT];
    unsigned total, left;
    float splash, sound_gap;
    bool finished, stroke;
    float last_x, last_y;
} pg_wash;
typedef struct pg_game {
    pg_pet pet;
    pg_device_data device;
    pg_device_runtime utilities;
    pg_handling handling;
    uint64_t synthetic_imu_us;
    pg_screen screen;
    pg_goal goal;
    pg_stage stage;
    float x, y, target_x, target_y, facing;
    float anim, goal_time, boredom, caption_time, particles, hatch_time;
    float since_decision, inspect_dwell;
    float player_idle, since_pet, since_clap, since_food, since_clean;
    float pointer_x, pointer_y, pointer_start_x, pointer_start_y;
    float pointer_time, pointer_distance, hold_accum, stroke_accum;
    float tilt_x, tilt_y, bowl_time, petting, mood, blink;
    /* Cosmetic performance state is transient and never changes care or AI RNG. */
    pg_emote emote;
    float emote_time, emote_duration;
    float warmth_time; /* Transient care performance; never serialized. */
    float sleep_settle; /* Starts after reaching the bed; pauses in standby. */
    uint32_t performance_serial, performance_variant;
    float hud_seconds, top_hud_seconds;
    float ambient_wait, stroll_wait;
    uint32_t ambient_serial;
    bool lamp_touch;
    bool reveal_touch, top_pull, debug_reset_armed;
    uint32_t revision, goal_serial, motion_serial;
    uint32_t session_games;
    int food_flavour, album_page, rhythm_feedback, games_page, icon_help_page;
    /* sleeping is visible night/pet rest; standby is the PWR/optional Auto-sleep display state. */
    bool pointer_down, hold_handled, sleeping, standby, muted, dirty, quit;
    bool brain_ready, brain_busy, brain_error, invitation;
    float brain_ms, brain_confidence;
    char name_edit[16];
    bool naming_hatch;
    int name_cursor;
    char caption[80];
    pg_sound sound;
    pg_ttt ttt;
    pg_tilt tilt;
    pg_rhythm rhythm;
    pg_echo echo;
    pg_balance balance;
    pg_wash wash;
    bool echo_audio;
    bool album_expanded;
    pg_event album_selected;
    /* Simulator storage coordinator owns these requests when enabled. */
    bool transactional_resets, care_paused, memorial_reset_armed;
    pg_reset_request reset_request;
} pg_game;

extern const char *const pg_species_names[3];
extern const char *const pg_goal_names[PG_GOAL_COUNT];
extern const char *const pg_stage_names[3];

/* Checked RTC scene checkpoint, independent of the persistent life codec. */
typedef struct {
    uint32_t version, night;
    float x,y,anim,settle;
    uint32_t variant,crc;
} pg_sleep_pose;
void pg_sleep_pose_capture(const pg_game *g,pg_sleep_pose *pose);
bool pg_sleep_pose_restore(pg_game *g,const pg_sleep_pose *pose);

uint32_t pg_random(pg_pet *p);
float pg_randf(pg_pet *p);
float pg_clamp(float v, float lo, float hi);
void pg_init(pg_game *g, uint32_t seed);
void pg_restore(pg_game *g, const pg_pet *p);
void pg_tick(pg_game *g, double seconds);
void pg_advance_care(pg_game *g, double seconds, bool sleeping);
void pg_sleep(pg_game *g, bool sleeping);
void pg_standby(pg_game *g, bool standby);
void pg_new_egg(pg_game *g);
void pg_event_add(pg_game *g, pg_event_kind kind, int arg);
void pg_react(pg_game *g, pg_emote emote, pg_sound sound);
void pg_say(pg_game *g, const char *text, pg_sound sound);
void pg_feed(pg_game *g, int flavour);
void pg_pet_gently(pg_game *g);
void pg_clean(pg_game *g);
void pg_egg_care(pg_game *g, float warmth, float comfort);
void pg_clap(pg_game *g);
void pg_pointer(pg_game *g, int kind, float x, float y); /* 0 down, 1 move, 2 up */
void pg_tap(pg_game *g, int x, int y);
void pg_back(pg_game *g);
void pg_name_begin(pg_game *g);
void pg_open_game(pg_game *g, int which);
void pg_game_result(pg_game *g, int which, int result);
bool pg_goal_feasible(const pg_game *g, pg_goal goal);
bool pg_accept_goal(pg_game *g, uint32_t identity, uint32_t revision,
                    pg_goal goal, float confidence, float latency_ms);
int pg_favorite_food(const pg_game *g);
int pg_favorite_toy(const pg_game *g);
const char *pg_event_text(const pg_event *e);

void pg_debug_hatch(pg_game *g);
void pg_debug_reset(pg_game *g);
void pg_balance_start(pg_game *g);
void pg_balance_go(pg_game *g);
void pg_wash_start(pg_game *g);
void pg_wash_pointer(pg_game *g, int kind, float x, float y);
void pg_nurture_tick(pg_game *g, float dt);
void pg_ttt_start(pg_game *g);
bool pg_ttt_move(pg_game *g, int cell);
int pg_ttt_best(const int8_t board[9], int player);
int pg_ttt_winner(const int8_t board[9]);
void pg_games_tick(pg_game *g, float dt);
void pg_tilt_start(pg_game *g);
void pg_tilt_recenter(pg_game *g);
void pg_rhythm_start(pg_game *g);
void pg_rhythm_tap(pg_game *g, int pad);
void pg_echo_start(pg_game *g);
void pg_echo_cancel(pg_game *g);
void pg_echo_finish(pg_game *g,bool natural);
void pg_echo_replay(pg_game *g);
void pg_echo_next(pg_game *g);
void pg_echo_tick(pg_game *g, float dt);
void pg_echo_audio_done(pg_game *g, uint32_t session, uint32_t round, pg_echo_stage stage);
void pg_echo_gesture(pg_game *g, pg_gesture action);
/* Acceleration in g. Provisional thresholds require authorized hardware tuning. */
void pg_echo_accel(pg_game *g, float x, float y, float z, float dt);
void pg_echo_shake_key(pg_game *g);
void pg_echo_pointer(pg_game *g, int kind, float x, float y);

uint32_t pg_crc32(const void *data, size_t len);
size_t pg_save_encode(const pg_pet *p, uint32_t sequence, uint8_t *out, size_t cap);
bool pg_save_decode(pg_pet *p, uint32_t *sequence, const uint8_t *data, size_t len);
bool pg_save_valid_pet(const pg_pet *p);

#endif
