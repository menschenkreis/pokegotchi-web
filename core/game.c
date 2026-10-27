#include "layout.h"
#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

const char *const pg_species_names[3] = {"MOSSLET", "EMBERKIN", "LUNAPIP"};
const char *const pg_goal_names[PG_GOAL_COUNT] = {
    "eat", "rest", "explore", "inspect", "play", "attention", "invite"};
const char *const pg_stage_names[3] = {"HATCHLING", "JUVENILE", "ADULT"};

float pg_clamp(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
uint32_t pg_random(pg_pet *p) {
    uint32_t x = p->rng ? p->rng : UINT32_C(0x97c29b3d);
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return p->rng = x;
}
float pg_randf(pg_pet *p) { return (pg_random(p) >> 8) * (1.0f / 16777216.0f); }
static int highest(const float *v) {
    int best = 0;
    for (int i = 1; i < 3; ++i) if (v[i] > v[best]) best = i;
    return best;
}
int pg_favorite_food(const pg_game *g) { return highest(g->pet.food_preference); }
int pg_favorite_toy(const pg_game *g) { return highest(g->pet.toy_preference); }

void pg_react(pg_game *g, pg_emote emote, pg_sound sound) {
    if(g->pet.phase!=PG_ALIVE)return;
    if(emote>PG_EMOTE_NONE&&emote<PG_EMOTE_COUNT){
        g->emote=emote;g->emote_duration=(emote==PG_EMOTE_SASS||emote==PG_EMOTE_WOBBLE)?1:emote==PG_EMOTE_CHEER?2.8f:emote==PG_EMOTE_NIBBLE?3.2f:2.0f;
        g->emote_time=g->emote_duration;
        uint32_t x=g->pet.identity+(++g->performance_serial)*UINT32_C(0x9e3779b9);
        x^=x>>16;x*=UINT32_C(0x7feb352d);x^=x>>15;
        g->performance_variant=x;
    }
    if(sound!=PG_SOUND_NONE&&!g->muted)g->sound=sound;
}
void pg_say(pg_game *g, const char *text, pg_sound sound) {
    snprintf(g->caption, sizeof g->caption, "%s", text);
    g->caption_time = 5.0f;
    if (sound != PG_SOUND_NONE && !g->muted) g->sound = sound;
    pg_emote e=sound==PG_SOUND_FOOD?PG_EMOTE_NIBBLE:sound==PG_SOUND_WIN||sound==PG_SOUND_HAPPY?PG_EMOTE_CHEER:
        sound==PG_SOUND_LOSE?PG_EMOTE_OOPS:sound==PG_SOUND_HELLO?PG_EMOTE_GREET:
        sound==PG_SOUND_CUDDLE?PG_EMOTE_CUDDLE:sound==PG_SOUND_CURIOUS?PG_EMOTE_CURIOUS:
        sound==PG_SOUND_SLEEP?PG_EMOTE_YAWN:sound==PG_SOUND_CLEAN?PG_EMOTE_CLEAN:PG_EMOTE_NONE;
    if(e!=PG_EMOTE_NONE)pg_react(g,e,PG_SOUND_NONE);
}
void pg_event_add(pg_game *g, pg_event_kind kind, int arg) {
    pg_pet *p = &g->pet;
    pg_event *e = &p->events[p->event_count % PG_EVENT_CAP];
    e->kind = (uint8_t)kind; e->arg = (uint8_t)arg; e->at = p->elapsed;
    if (p->event_count < UINT32_MAX) ++p->event_count;
    g->dirty = true;
}
const char *pg_event_text(const pg_event *e) {
    static const char *const names[] = {"A LITTLE MYSTERY ARRIVED", "WARM HANDS, SAFE SHELL",
        "A NEW FRIEND HATCHED", "SHARED A LITTLE MEAL", "A MOMENT OF TRUST",
        "A FRESH START", "A PEACEFUL NAP", "WON A GAME TOGETHER", "TRIED AGAIN, TOGETHER",
        "AN EVENLY MATCHED GAME", "GREW A LITTLE BIGGER", "FOUND A NEW DELIGHT",
        "A FAVOURITE EMERGED", "ANSWERED YOUR CLAP", "A SMALL LIFE REMEMBERED",
        "A NEW CHAPTER BEGINS"};
    return e->kind < sizeof names / sizeof *names ? names[e->kind] : "A QUIET MOMENT";
}

static void runtime_init(pg_game *g) {
    g->screen = PG_HOME; g->goal = g->pet.phase==PG_ALIVE?PG_EXPLORE:PG_REST;
    g->x = g->target_x = 233; g->y = g->target_y = 276;
    g->facing = 1; g->revision = 1;
    g->since_pet = g->since_food = g->since_clean = g->since_clap = 100;
    g->blink = 2; g->hud_seconds=8;g->top_hud_seconds=0;g->ambient_wait=9;
    g->muted = g->pet.muted;
    g->stage = g->pet.age >= 7 * PG_DAY ? PG_ADULT : g->pet.age >= 2 * PG_DAY ? PG_YOUNG : PG_BABY;
}
void pg_init(pg_game *g, uint32_t seed) {
    memset(g, 0, sizeof *g);
    pg_device_defaults(&g->device);g->utilities.writable=true;
    pg_pet *p = &g->pet;
    p->rng = seed ? seed : UINT32_C(0x5eaf1027);
    p->identity = pg_random(p);
    p->species = pg_random(p) % PG_SPECIES_COUNT;
    p->generation = 1; p->phase = PG_EGG; p->brightness=65; p->muted=true;
    p->satiety = p->energy = p->clean = p->health = 100;
    p->affection = 65; p->trust = 20; p->warmth = 58; p->comfort = 70;
    p->last_egg_visit = -60;
    p->bold = 25 + 50 * pg_randf(p);
    p->curious = 25 + 50 * pg_randf(p);
    p->playful = 25 + 50 * pg_randf(p);
    if (p->species == PG_MOSSLET) p->curious += 12;
    if (p->species == PG_EMBERKIN) p->playful += 12;
    if (p->species == PG_LUNAPIP) p->bold -= 12;
    for (int i = 0; i < 3; ++i) {
        p->food_preference[i] = 35 + 30 * pg_randf(p);
        p->toy_preference[i] = 35 + 30 * pg_randf(p);
        p->confidence[i] = 40;
    }
    for(int i=3;i<PG_GAME_COUNT;++i)p->confidence[i]=40;
    static const char *const names[] = {"PIP", "MISO", "BOO", "NORI", "BIM", "WISP", "MOMO", "POPPY", "FENN", "TOFU", "LUMI", "PEBBLE"};
    snprintf(p->name, sizeof p->name, "%s", names[pg_random(p) % 12]);
    runtime_init(g);
    pg_event_add(g, PG_EV_FOUND, 0);
    pg_say(g, "SOMEBODY SMALL IS IN THERE.", PG_SOUND_NONE);
}
void pg_restore(pg_game *g, const pg_pet *p) {
    memset(g, 0, sizeof *g);pg_device_defaults(&g->device);g->utilities.writable=true;
    g->pet = *p;
    /* Every fresh session is quiet, even after a previously unmuted save. */
    g->pet.muted=true;
    runtime_init(g);
    /* A hatch reveal interrupted by a reset starts visibly again. */
    if (g->pet.phase == PG_HATCHING) g->hatch_time = 0;
    if (g->pet.phase == PG_DEAD) g->screen = PG_MEMORIAL;
    if (g->pet.phase == PG_ALIVE && g->pet.naming_pending) pg_name_begin(g);
    pg_say(g, g->pet.phase == PG_EGG ? "A LITTLE WARMTH GOES A LONG WAY." :
        g->pet.phase == PG_DEAD ? "SMALL PAWS. A LASTING PLACE." : "THERE YOU ARE.", PG_SOUND_NONE);
}
void pg_new_egg(pg_game *g) {
    if (g->pet.phase != PG_DEAD || !g->pet.death_acknowledged) return;
    if(g->transactional_resets){g->reset_request=PG_REQUEST_SUCCESSOR;return;}
    pg_device_data device=g->device;pg_device_runtime utilities=g->utilities;
    uint32_t seed = pg_random(&g->pet), generation = g->pet.generation + 1;
    uint32_t legacy = g->pet.legacy_count + 1, species = g->pet.species;
    uint8_t plant = g->pet.plant_style, bed = g->pet.bed_style, toy = g->pet.toy_style;
    char legacy_name[16];memcpy(legacy_name,g->pet.name,sizeof legacy_name);
    bool muted = g->muted; uint8_t brightness=g->pet.brightness;
    pg_init(g, seed);g->device=device;g->utilities=utilities;g->utilities.editing=false;g->utilities.idle=0;
    g->pet.generation = generation; g->pet.legacy_count = legacy;
    g->pet.legacy_species = (uint8_t)species;
    memcpy(g->pet.legacy_name,legacy_name,sizeof legacy_name);
    g->pet.plant_style = plant; g->pet.bed_style = bed; g->pet.toy_style = toy;
    g->muted = muted;
    g->pet.muted = muted; g->pet.brightness=brightness;
    pg_event_add(g, PG_EV_SUCCESSOR, (int)generation);
}

static void die(pg_game *g) {
    if (g->pet.phase != PG_ALIVE) return;
    pg_echo_cancel(g);
    g->pet.phase = PG_DEAD;
    g->pet.health = 0; g->screen = PG_MEMORIAL; g->sleeping = false;
    g->pointer_down = false; g->invitation = false; ++g->revision;
    pg_event_add(g, PG_EV_DIED, 0);
    pg_say(g, "A SMALL LIFE. A LASTING PLACE.", PG_SOUND_NONE);
}
static void home_unlocks(pg_game *g){
    if((g->pet.meals>=5||g->pet.pets>=10)&&!(g->pet.milestones&2u)){
        g->pet.milestones|=2u;++g->pet.discoveries;pg_event_add(g,PG_EV_DISCOVERY,4);
        pg_say(g,"A NEW NEST COLOUR! TAP THE BED.",PG_SOUND_HAPPY);
    }
    if(g->pet.pets>=25&&!(g->pet.milestones&4u)){
        g->pet.milestones|=4u;++g->pet.discoveries;pg_event_add(g,PG_EV_DISCOVERY,5);
        pg_say(g,"SO MUCH TRUST. ANOTHER SNUG NEST.",PG_SOUND_HAPPY);
    }
}

#define CARE_UNIT UINT64_C(1728000000)
#define CARE_MAX (100 * CARE_UNIT)
#define CARE_LIMIT UINT64_C(129600000000)
static uint64_t care_from_float(float v){return (uint64_t)llround((double)pg_clamp(v,0,100)*CARE_UNIT);}
static void care_project(pg_pet *p){
    p->satiety=(float)((double)p->care_food/CARE_UNIT);
    p->energy=(float)((double)p->care_energy/CARE_UNIT);
    p->clean=(float)((double)p->care_clean/CARE_UNIT);
}
static void care_sync(pg_pet *p){
    if(!p->care_initialized){p->care_food=care_from_float(p->satiety);p->care_energy=care_from_float(p->energy);p->care_clean=care_from_float(p->clean);p->care_fraction_us=0;p->care_initialized=true;}
    else{
        if(p->satiety!=(float)((double)p->care_food/CARE_UNIT))p->care_food=care_from_float(p->satiety);
        if(p->energy!=(float)((double)p->care_energy/CARE_UNIT))p->care_energy=care_from_float(p->energy);
        if(p->clean!=(float)((double)p->care_clean/CARE_UNIT))p->care_clean=care_from_float(p->clean);
    }
}
static void care_rules(pg_game *g){
    pg_pet *p=&g->pet;
    if(p->phase!=PG_ALIVE)return;
    if(p->neglect_active && p->care_food>=70*CARE_UNIT && p->care_clean>=70*CARE_UNIT && p->care_energy>=95*CARE_UNIT){p->neglect_active=false;p->neglect_us=0;g->dirty=true;}
    if(!p->neglect_active && (p->care_food<=50*CARE_UNIT||p->care_clean<=50*CARE_UNIT||p->care_energy<=50*CARE_UNIT)){p->neglect_active=true;p->neglect_us=0;g->dirty=true;}
    p->health=p->neglect_active?(float)(100.0*(1.0-(double)p->neglect_us/CARE_LIMIT)):100;
}
static uint64_t ceil_div(uint64_t x,uint64_t y){return x/y+(x%y!=0);}
static uint64_t min_u64(uint64_t a,uint64_t b){return a<b?a:b;}
void pg_advance_care(pg_game *g, double seconds, bool sleeping) {
    if (!isfinite(seconds) || seconds <= 0 || seconds > 36525.0 * PG_DAY) return;
    pg_pet *p = &g->pet;
    if (p->phase == PG_EGG) {
        p->elapsed += seconds;
        p->incubation = fmin(p->incubation + seconds, PG_EGG_SECONDS);
        float target = sleeping && p->sheltered ? 53 : p->sheltered ? 47 : 25;
        p->warmth = target + (p->warmth - target) * expf(-(float)seconds / 1800);
        p->comfort = pg_clamp(p->comfort - (float)(seconds / PG_HOUR) * (p->sheltered ? 1 : 4), 0, 100);
        return;
    }
    if (p->phase != PG_ALIVE) return;
    care_sync(p);care_rules(g);
    double total=seconds*1000000.0+p->care_fraction_us;
    uint64_t left=(uint64_t)floor(total);p->care_fraction_us=total-(double)left;
    double lived=0;
    while(left && p->phase==PG_ALIVE){
        uint64_t dt=left;
        if(!p->neglect_active){
            if(p->care_food>50*CARE_UNIT)dt=min_u64(dt,ceil_div(p->care_food-50*CARE_UNIT,2));
            if(p->care_clean>50*CARE_UNIT)dt=min_u64(dt,p->care_clean-50*CARE_UNIT);
            if(!sleeping&&p->care_energy>50*CARE_UNIT)dt=min_u64(dt,ceil_div(p->care_energy-50*CARE_UNIT,2));
        }else{
            dt=min_u64(dt,CARE_LIMIT-p->neglect_us);
            if(sleeping&&p->care_food>=70*CARE_UNIT&&p->care_clean>=70*CARE_UNIT&&p->care_energy<95*CARE_UNIT){
                uint64_t rest=ceil_div(95*CARE_UNIT-p->care_energy,34560);
                if(p->care_food>=70*CARE_UNIT+2*rest && p->care_clean>=70*CARE_UNIT+rest)dt=min_u64(dt,rest);
            }
        }
        if(!dt)break;
        p->care_food=p->care_food>2*dt?p->care_food-2*dt:0;
        p->care_clean=p->care_clean>dt?p->care_clean-dt:0;
        if(sleeping){uint64_t add=dt>CARE_MAX/34560?CARE_MAX:dt*34560;p->care_energy=add>=CARE_MAX-p->care_energy?CARE_MAX:p->care_energy+add;}
        else p->care_energy=p->care_energy>2*dt?p->care_energy-2*dt:0;
        if(p->neglect_active)p->neglect_us+=dt;
        double d=(double)dt/1000000.0;p->elapsed+=d;p->age+=d;lived+=d;left-=dt;
        care_project(p);care_rules(g);
        if(p->neglect_active&&p->neglect_us>=CARE_LIMIT){p->neglect_active=false;die(g);break;}
    }
    p->affection=pg_clamp(p->affection-(float)(lived*100/(3*PG_DAY)),0,100);
    pg_stage stage=p->age>=7*PG_DAY?PG_ADULT:p->age>=2*PG_DAY?PG_YOUNG:PG_BABY;
    if(stage!=g->stage&&p->phase==PG_ALIVE){g->stage=stage;pg_event_add(g,PG_EV_GREW,stage);if(!sleeping)pg_say(g,"GROWING INTO THOSE LITTLE PAWS.",PG_SOUND_HAPPY);}
}

void pg_egg_care(pg_game *g, float warmth, float comfort) {
    pg_pet *p = &g->pet;
    if (p->phase != PG_EGG || g->sleeping) return;
    /* Warm hands are a thermostat: enthusiastic care cannot cook the shell. */
    if(warmth>0)p->warmth += pg_clamp(72-p->warmth,-warmth,warmth);
    else p->warmth=pg_clamp(p->warmth+warmth,0,100);
    p->comfort = pg_clamp(p->comfort + comfort, 0, 100);
    if (p->elapsed - p->last_egg_visit >= 30) {
        p->last_egg_visit = p->elapsed;
        if (p->egg_visits < 10000) ++p->egg_visits;
        pg_event_add(g, PG_EV_WARMED, 0);
    }
    if(warmth>0 && g->warmth_time<=0)g->warmth_time=2.8f;
    g->petting = 1; g->dirty = true;
}
void pg_feed(pg_game *g, int flavour) {
    if (g->pet.phase != PG_ALIVE || flavour < 0 || flavour > 2 || g->since_food < 3) return;
    g->food_flavour = flavour; g->bowl_time = 4.0f; g->since_food = 0;
    g->goal = PG_EAT; g->goal_time = 0; ++g->revision;
    g->since_decision=0;
    g->target_x = 233; g->target_y = 279;
    float need = 100 - g->pet.satiety;
    g->pet.satiety = 100; care_sync(&g->pet); care_rules(g);
    if (need > 5) {
        ++g->pet.meals;
        g->pet.food_preference[flavour] = pg_clamp(g->pet.food_preference[flavour] + 1.5f + need * 0.025f, 0, 100);
        g->pet.trust = pg_clamp(g->pet.trust + 1, 0, 100);
        pg_event_add(g, PG_EV_FED, flavour);
    }
    pg_say(g, need < 5 ? "JUST A LITTLE TASTE, THANK YOU." :
        flavour == pg_favorite_food(g) ? "THE GOOD STUFF. LITTLE HAPPY FEET." : "A MEAL SHARED IS A MOMENT KEPT.", PG_SOUND_FOOD);
    g->screen = PG_HOME; g->dirty = true;
    home_unlocks(g);
    /* A simultaneous nest discovery must not hide the meal just selected. */
    if(g->emote!=PG_EMOTE_NIBBLE)pg_react(g,PG_EMOTE_NIBBLE,PG_SOUND_NONE);
}
void pg_pet_gently(pg_game *g) {
    if (g->pet.phase == PG_EGG) { pg_egg_care(g, 2, 6); return; }
    if (g->pet.phase != PG_ALIVE || g->since_pet < 4) return;
    g->since_pet = 0; g->petting = 1.5f; ++g->pet.pets;
    g->pet.affection = pg_clamp(g->pet.affection + 10, 0, 100);
    g->pet.trust = pg_clamp(g->pet.trust + 0.8f, 0, 100);
    g->pet.bold = pg_clamp(g->pet.bold + 0.03f, 0, 100);
    pg_event_add(g, PG_EV_PETTED, 0);
    pg_say(g, g->pet.trust > 60 ? "LEANING INTO YOUR HAND." : "LEARNING YOUR KIND OF KINDNESS.", PG_SOUND_CUDDLE);
    home_unlocks(g);
}
void pg_clean(pg_game *g) {
    if (g->pet.phase == PG_EGG) {
        g->pet.sheltered = true; pg_egg_care(g, 6, 15);
        pg_say(g, "TUCKED IN. SAFE AND SNUG.", PG_SOUND_WARM);
    } else if (g->pet.phase == PG_ALIVE) {
        g->since_clean = 0;
        bool needed = g->pet.clean < 95;
        g->pet.clean = 100; care_sync(&g->pet); care_rules(g); g->particles = 2;
        if (needed) pg_event_add(g, PG_EV_CLEANED, 0);
        pg_say(g, "FRESH NEST. FRESH LITTLE START.", PG_SOUND_CLEAN);
    }
    g->dirty = true;
}
void pg_sleep(pg_game *g, bool sleeping) {
    if (g->standby || g->pet.phase == PG_DEAD || g->sleeping == sleeping) return;
    g->sleeping = sleeping; g->pointer_down = false;g->wash.stroke=false;g->reveal_touch=g->top_pull=false;g->hud_seconds=8;g->top_hud_seconds=0;g->tilt_x=g->tilt_y=0; ++g->revision;
    if (sleeping) {
        g->sleep_settle=0;g->target_x=195;g->target_y=282;g->invitation=false;g->petting=0;g->particles=0;
        g->warmth_time=0;g->bowl_time=0;
        pg_echo_cancel(g);
        g->screen = PG_HOME; g->goal = PG_REST; g->goal_time = 0;
        pg_event_add(g, PG_EV_SLEPT, 0);
        pg_react(g,PG_EMOTE_YAWN,PG_SOUND_SLEEP);
    } else {
        g->sleep_settle=0;
        g->goal=PG_EXPLORE;g->goal_time=0;g->stroll_wait=0;g->ambient_wait=4;
        g->target_x=g->x;g->target_y=g->y;g->player_idle=0;
        pg_say(g, g->pet.phase == PG_EGG ? "STILL A LITTLE MYSTERY." : "OH! HELLO, YOU.", PG_SOUND_HELLO);
    }
    g->dirty = true;
}
void pg_sleep_pose_capture(const pg_game *g,pg_sleep_pose *pose){
    memset(pose,0,sizeof *pose);
    pose->version=1;pose->night=g->sleeping;pose->x=g->x;pose->y=g->y;
    pose->anim=g->anim;pose->settle=g->sleep_settle;pose->variant=g->performance_variant;
    pose->crc=pg_crc32(pose,offsetof(pg_sleep_pose,crc));
}
bool pg_sleep_pose_restore(pg_game *g,const pg_sleep_pose *pose){
    if(pose->version!=1||pose->night>1||pose->crc!=pg_crc32(pose,offsetof(pg_sleep_pose,crc))||
       !isfinite(pose->x)||pose->x<0||pose->x>PG_W||!isfinite(pose->y)||pose->y<0||pose->y>PG_H||
       !isfinite(pose->anim)||pose->anim<0||!isfinite(pose->settle)||pose->settle<0||pose->settle>3)return false;
    g->sleeping=pose->night!=0;g->x=pose->x;g->y=pose->y;g->anim=pose->anim;
    g->sleep_settle=g->sleeping?pose->settle:0;g->performance_variant=pose->variant;
    g->goal=g->sleeping?PG_REST:PG_EXPLORE;g->goal_time=0;
    g->target_x=g->sleeping?195:g->x;g->target_y=g->sleeping?282:g->y;
    g->invitation=false;g->emote=PG_EMOTE_NONE;g->emote_time=0;g->sound=PG_SOUND_NONE;
    /* Resume a partially finished arrival yawn without replaying its cue. */
    if(g->sleeping&&pose->settle>0&&pose->settle<2){
        g->emote=PG_EMOTE_YAWN;g->emote_duration=2;g->emote_time=2-pose->settle;
    }
    return true;
}
void pg_standby(pg_game *g, bool standby) {
    if(g->standby==standby)return;
    g->standby=standby;g->utilities.idle=0;g->utilities.editing=false;g->utilities.checkpoint=true;
    g->pointer_down=g->lamp_touch=g->reveal_touch=g->top_pull=g->wash.stroke=false;
    g->tilt_x=g->tilt_y=0;g->sound=PG_SOUND_NONE;
    g->warmth_time=0;g->bowl_time=0;
    if(!g->sleeping){g->emote_time=0;g->emote=PG_EMOTE_NONE;}
    pg_echo_cancel(g);
    if(g->screen!=PG_NAME_EDIT||g->pet.phase!=PG_ALIVE)g->screen=g->pet.phase==PG_DEAD?PG_MEMORIAL:PG_HOME;
    g->hud_seconds=8;g->top_hud_seconds=0;g->player_idle=0;++g->revision;
    /* Preserve day/night. Standby is neither a lamp tap nor a pet sleep event. */
}
void pg_clap(pg_game *g) {
    if (g->standby || g->sleeping || g->pet.phase != PG_ALIVE || g->since_clap < 3) return;
    g->since_clap = 0; g->petting = 1; g->player_idle = 0;
    pg_event_add(g, PG_EV_CLAP, 0);
    pg_say(g, g->pet.trust > 50 ? "YOUR LITTLE CALL. A LITTLE ANSWER." : "OH? WAS THAT YOU?", PG_SOUND_HELLO);
}

bool pg_goal_feasible(const pg_game *g, pg_goal goal) {
    if (g->standby || g->pet.phase != PG_ALIVE || g->sleeping || goal < 0 || goal >= PG_GOAL_COUNT) return false;
    if (goal == PG_EAT) return g->bowl_time > 0;
    if (goal == PG_ATTENTION || goal == PG_INVITE) return g->player_idle < 90;
    return true;
}
bool pg_accept_goal(pg_game *g, uint32_t identity, uint32_t revision,
                    pg_goal goal, float confidence, float latency_ms) {
    if (identity != g->pet.identity || revision != g->revision || g->screen != PG_HOME ||
        !pg_goal_feasible(g, goal) || !isfinite(confidence) || !isfinite(latency_ms)) return false;
    if (g->pet.energy < 10 && goal != PG_REST && goal != PG_EAT) return false;
    g->brain_confidence = confidence; g->brain_ms = latency_ms;
    bool changed=g->goal!=goal;
    g->goal = goal; ++g->goal_serial;
    if(changed){g->goal_time=0;g->boredom*=0.35f;g->inspect_dwell=0;}
    g->invitation=goal==PG_INVITE;
    switch (goal) {
    case PG_EXPLORE:
        if(changed||hypotf(g->target_x-g->x,g->target_y-g->y)<5){
            g->target_x = 133 + 200 * pg_randf(&g->pet); g->target_y = 265 + 32 * pg_randf(&g->pet);
        }
        break;
    case PG_INSPECT: g->target_x = 333; g->target_y = 268;
        if(changed){pg_react(g,PG_EMOTE_CURIOUS,PG_SOUND_NONE);}
        break;
    case PG_REST: g->target_x = 195; g->target_y = 282; break;
    case PG_PLAY: g->target_x = 305; g->target_y = 291; break;
    case PG_ATTENTION: g->target_x = 233; g->target_y = 292;
        if(changed){pg_say(g, "A SMALL PAW REACHES YOUR WAY.", PG_SOUND_NONE);
            pg_react(g,PG_EMOTE_GREET,PG_SOUND_HELLO);} break;
    case PG_INVITE: g->invitation = true; g->target_x = 233; g->target_y = 285;
        if(changed){pg_say(g, "ONE QUICK GAME?", PG_SOUND_NONE);
            pg_react(g,PG_EMOTE_INVITE,PG_SOUND_CURIOUS);} break;
    default: break;
    }
    return true;
}

void pg_tick(pg_game *g, double seconds) {
    if (!isfinite(seconds) || seconds <= 0) return;
    if(!g->care_paused)pg_advance_care(g, seconds, g->sleeping);
    if(g->standby)return;
    if (seconds > 2) return; /* catch-up is not a burst of input or animation */
    float dt = (float)seconds;
    g->anim += dt;
    g->warmth_time=fmaxf(0,g->warmth_time-dt);
    g->emote_time=fmaxf(0,g->emote_time-dt);
    if(g->emote_time==0)g->emote=PG_EMOTE_NONE;
    if(g->sleeping){
        /* Walk at a measured speed, then climb/tuck in. Never slide asleep. */
        if(g->pet.phase==PG_ALIVE){
            float dx=195-g->x,dy=282-g->y,d=hypotf(dx,dy),step=32*dt;
            g->target_x=195;g->target_y=282;
            if(d>step){g->x+=dx/d*step;g->y+=dy/d*step;g->sleep_settle=0;if(fabsf(dx)>1)g->facing=dx>0?1:-1;}
            else{
                g->x=195;g->y=282;
                if(g->sleep_settle==0)pg_react(g,PG_EMOTE_YAWN,PG_SOUND_NONE);
                float before=g->sleep_settle;
                g->sleep_settle=fminf(3,g->sleep_settle+dt);
                if(before<2&&g->sleep_settle>=2){g->dirty=true;++g->revision;}
            }
        }
        return;
    }
    if(g->screen==PG_HOME&&!g->pointer_down){g->hud_seconds=fmaxf(0,g->hud_seconds-dt);g->top_hud_seconds=fmaxf(0,g->top_hud_seconds-dt);}
    g->goal_time += dt; g->since_decision+=dt; g->player_idle += dt;
    g->since_pet += dt; g->since_food += dt; g->since_clean += dt; g->since_clap += dt;
    g->caption_time = fmaxf(0, g->caption_time - dt);
    g->particles = fmaxf(0, g->particles - dt); g->petting = fmaxf(0, g->petting - dt);
    g->bowl_time = fmaxf(0, g->bowl_time - dt);
    g->boredom = pg_clamp(g->boredom + dt * 1.4f, 0, 100);
    g->blink -= dt;
    if (g->blink < -0.16f) g->blink = 2.5f + pg_randf(&g->pet) * 4;
    g->mood = (g->pet.satiety + g->pet.energy + g->pet.clean + g->pet.affection) / 4;
    if (g->pointer_down&&!g->top_pull) {
        g->pointer_time += dt;
        float dx = g->pointer_x - g->x, dy = g->pointer_y - (g->y - 20);
        if (g->screen == PG_HOME && dx * dx + dy * dy < 10000 && g->pointer_time > 0.45f) {
            if (g->pet.phase == PG_EGG) {
                if(!g->hold_handled)pg_egg_care(g,dt*5,dt*2);
                else{
                    g->pet.warmth+=pg_clamp(72-g->pet.warmth,-dt*5,dt*5);
                    g->pet.comfort=pg_clamp(g->pet.comfort+dt*2,0,100);
                    g->petting=1;g->dirty=true;
                    g->warmth_time=fmaxf(g->warmth_time,1.5f);
                }
                if (!g->hold_handled) pg_say(g, "A LITTLE WARMTH. A LITTLE WIGGLE.", PG_SOUND_WARM);
            } else if (!g->hold_handled) pg_pet_gently(g);
            g->hold_handled = true;
        }
    }
    if (g->pet.phase == PG_EGG && g->screen == PG_HOME &&
        g->pet.incubation >= PG_EGG_SECONDS && g->pet.egg_visits >= 3) {
        g->pet.phase = PG_HATCHING; g->hatch_time = 0; ++g->revision;
        pg_say(g, "WAIT... DID YOU SEE THAT?", PG_SOUND_HATCH);
        g->dirty = true;
    }
    if (g->pet.phase == PG_HATCHING && g->screen == PG_HOME) {
        g->hatch_time += dt;
        if (g->hatch_time >= 5.5f) {
            g->pet.phase = PG_ALIVE; g->pet.age = 0; g->stage = PG_BABY;
            g->pet.satiety = g->pet.energy = g->pet.health = 100;
            g->goal = PG_EXPLORE; g->goal_time = 0; g->particles = 3;
            char s[80]; snprintf(s, sizeof s, "HELLO, %s. HELLO, LITTLE WORLD.", g->pet.name);
            pg_event_add(g, PG_EV_HATCHED, (int)g->pet.species);
            pg_say(g, s, PG_SOUND_HELLO);
            g->pet.naming_pending=true;g->dirty=true;pg_name_begin(g);
        }
    }
    if (g->pet.phase == PG_ALIVE && g->screen == PG_HOME) {
        if (g->pet.energy < 5 && g->goal != PG_EAT) {
            g->goal = PG_REST; g->target_x = 195; g->target_y = 282;
        }
        /* Cosmetic choreography uses its own hash, never the care/teacher RNG. */
        g->ambient_wait-=dt;
        if(g->ambient_wait<=0&&!g->pointer_down&&g->emote_time<=0&&g->pet.energy>=10&&g->goal!=PG_REST){
            uint32_t r=g->pet.identity+(++g->ambient_serial)*UINT32_C(0x9e3779b9);
            r^=r>>16;r*=UINT32_C(0x7feb352d);r^=r>>15;
            bool attention=g->goal==PG_ATTENTION||g->goal==PG_INVITE||g->pet.affection<45||g->player_idle>18;
            pg_emote options[]={PG_EMOTE_CURIOUS,PG_EMOTE_CHEER,PG_EMOTE_GREET,PG_EMOTE_CUDDLE};
            pg_react(g,attention?PG_EMOTE_GREET:options[r%4],PG_SOUND_NONE);
            g->ambient_wait=(attention?5:9)+(r>>8)%7;
        }
        if(g->goal==PG_EXPLORE&&g->pet.energy>=10&&g->emote_time<=0&&!g->pointer_down&&hypotf(g->target_x-g->x,g->target_y-g->y)<3){
            g->stroll_wait-=dt;
            if(g->stroll_wait<=0){
                uint32_t r=g->pet.identity+(++g->ambient_serial)*UINT32_C(0x85ebca6b);r^=r>>13;
                g->target_x=178+r%111;g->target_y=278+(r>>8)%23;g->stroll_wait=2+(r>>16)%4;
            }
        }
        float dx = g->target_x - g->x, dy = g->target_y - g->y;
        float d = sqrtf(dx * dx + dy * dy), step = dt * (18 + g->pet.bold * 0.15f);
        if (d > 1) {
            g->x += dx / d * fminf(step, d); g->y += dy / d * fminf(step, d);
            if (fabsf(dx) > 2) g->facing = dx > 0 ? 1 : -1;
        }
        /* Object discoveries require time actually spent there, not model prose. */
        if(g->goal==PG_INSPECT&&d<5)g->inspect_dwell+=dt;else g->inspect_dwell=0;
        if (g->inspect_dwell > 3 && !(g->pet.milestones & 1u)) {
            g->pet.milestones |= 1u; ++g->pet.discoveries;
            g->pet.plant_style = 1; pg_event_add(g, PG_EV_DISCOVERY, 0);
            pg_say(g, "THERE'S A TINY FLOWER IN THERE!", PG_SOUND_HAPPY);
        }
    }
    pg_games_tick(g, dt);
}

void pg_name_begin(pg_game *g) {
    if(g->pet.phase!=PG_ALIVE)return;
    memcpy(g->name_edit,g->pet.name,sizeof g->name_edit);
    g->name_edit[PG_NAME_MAX]=0; /* Preserve old saved names until the edit is confirmed. */
    g->naming_hatch=g->pet.naming_pending;g->name_cursor=0;
    g->screen=PG_NAME_EDIT;g->pointer_down=false;++g->revision;
}
static void name_letter(pg_game *g,int direction){
    size_t n=strlen(g->name_edit);int i=g->name_cursor;
    if(i<0||i>=PG_NAME_MAX||i>(int)n)return;
    char current=i==(int)n?' ':g->name_edit[i];
    int letter=current>='A'&&current<='Z'?current-'A'+1:0;
    letter=(letter+direction+27)%27;
    /* The empty end position becomes a new letter when first scrolled. */
    if(i==(int)n){if(!letter)return;g->name_edit[i+1]=0;}
    g->name_edit[i]=letter?(char)('A'+letter-1):' ';
    ++g->revision;
}
static void name_finish(pg_game *g,bool save) {
    if(save){
        size_t n=strlen(g->name_edit);
        while(n&&g->name_edit[n-1]==' ')g->name_edit[--n]=0;
        while(n&&g->name_edit[0]==' '){memmove(g->name_edit,g->name_edit+1,n);--n;}
        if(g->name_cursor>(int)n)g->name_cursor=(int)n;
        if(!n||n>PG_NAME_MAX)return;
        memcpy(g->pet.name,g->name_edit,sizeof g->pet.name);
        g->dirty=true;
    }
    if(g->naming_hatch){g->pet.naming_pending=false;g->dirty=true;}
    g->screen=g->naming_hatch?PG_HOME:PG_CARD;
    g->pointer_down=false;++g->revision;
}
void pg_back(pg_game *g) {
    if(g->standby)return;
    pg_user_activity(g,PG_ACTIVITY_BUTTON);
    if(g->screen==PG_NAME_EDIT){name_finish(g,false);return;}
    if(g->screen==PG_CLOCK_EDIT||g->screen==PG_VOLUME||g->screen==PG_DEVICE_STATUS){g->utilities.editing=false;g->screen=PG_SETTINGS;g->utilities.settings_page=g->utilities.return_page;return;}
    if(g->screen==PG_ALBUM&&g->album_expanded){g->album_expanded=false;g->pointer_down=false;++g->revision;return;}
    if (g->pet.phase == PG_DEAD) { g->memorial_reset_armed = false; g->screen = PG_MEMORIAL; return; }
    g->hud_seconds=8;g->debug_reset_armed=false;g->reveal_touch=g->top_pull=false;g->wash.stroke=false;
    if(g->screen==PG_HELP||g->screen==PG_DEBUG)g->screen=PG_SETTINGS;
    else if(g->screen==PG_BALANCE||g->screen==PG_WASH){g->screen=PG_GAMES;g->games_page=1;}
    else if(g->screen==PG_ECHO){pg_echo_cancel(g);g->screen=PG_GAMES;g->games_page=1;}
    else if (g->screen == PG_TTT || g->screen == PG_TILT || g->screen == PG_RHYTHM) g->screen = PG_GAMES;
    else if(g->screen==PG_GAMES&&g->games_page){--g->games_page;}
    else g->screen = PG_HOME;
    g->pointer_down = false; ++g->revision;
}

static bool tilt_arrow(pg_game *g,float x,float y){
    static const int centers[4][2]={{233,142},{97,244},{369,244},{233,346}};
    static const int directions[4][2]={{0,-1},{-1,0},{1,0},{0,1}};
    if(g->screen!=PG_TILT||g->tilt.finished)return false;
    for(int i=0;i<4;++i)if(fabsf(x-centers[i][0])<=32&&fabsf(y-centers[i][1])<=32){
        g->tilt.ready=0;g->tilt_x=(float)directions[i][0];g->tilt_y=(float)directions[i][1];return true;
    }
    return false;
}

static int *menu_page(pg_game *g,int *count){
    switch(g->screen){
    case PG_SETTINGS:*count=3;return &g->utilities.settings_page;
    case PG_GAMES:*count=2;return &g->games_page;
    case PG_HELP:*count=8;return &g->icon_help_page;
    case PG_ALBUM:
        if(g->album_expanded)return NULL;
        *count=(int)((g->pet.event_count<PG_EVENT_CAP?g->pet.event_count:PG_EVENT_CAP)+4)/5;
        if(*count<1)*count=1;
        return &g->album_page;
    default:return NULL;
    }
}

void pg_pointer(pg_game *g, int kind, float x, float y) {
    if(g->standby)return;
    if(kind==3){g->pointer_down=g->lamp_touch=g->reveal_touch=g->top_pull=g->wash.stroke=g->echo.pointer_armed=false;g->tilt_x=g->tilt_y=0;return;}
    if (!isfinite(x) || !isfinite(y)){if(kind!=0){g->pointer_down=g->top_pull=false;}return;}
    if(!pg_ui_visible(x,y)){
        if(kind!=0){g->pointer_down=false;false;g->echo.pointer_armed=false;g->wash.stroke=false;g->reveal_touch=g->top_pull=false;g->lamp_touch=false;g->tilt_x=g->tilt_y=0;}
        return;
    }
    if(kind==0){pg_user_activity(g,PG_ACTIVITY_TOUCH_DOWN);g->utilities.activity_x=x;g->utilities.activity_y=y;g->utilities.contact_age=g->utilities.contact_beat=0;}
    else if(kind==1&&(g->pointer_down||g->wash.stroke||g->lamp_touch)&&hypotf(x-g->utilities.activity_x,y-g->utilities.activity_y)>=4){pg_user_activity(g,PG_ACTIVITY_TOUCH_MOVE);g->utilities.activity_x=x;g->utilities.activity_y=y;}
    /* The upper home contact owns its whole gesture, so pulling the HUD cannot
       also warm/pet the creature or release onto a newly revealed control. */
    if(kind==0&&g->screen==PG_HOME&&!g->sleeping&&g->pet.phase!=PG_HATCHING&&y<=160){
        g->top_pull=g->pointer_down=true;g->pointer_start_x=g->pointer_x=x;g->pointer_start_y=g->pointer_y=y;
        g->pointer_time=g->pointer_distance=0;g->hold_handled=false;g->player_idle=0;return;
    }
    if(g->top_pull){
        if(kind==2){
            float dx=x-g->pointer_start_x,dy=y-g->pointer_start_y;
            g->top_pull=g->pointer_down=false;
            if(dy>=42&&fabsf(dx)<=64&&dy>fabsf(dx)*1.2f)g->top_hud_seconds=8;
            else if(dx*dx+dy*dy<400)pg_tap(g,(int)x,(int)y);
        }
        return;
    }
    if(kind==0&&g->screen==PG_HOME&&g->pet.phase!=PG_HATCHING&&pg_ui_hit(pg_ui_lamp,(int)x,(int)y)){
        g->lamp_touch=true;g->pointer_down=false;g->pointer_start_x=x;g->pointer_start_y=y;return;
    }
    if(g->lamp_touch){
        if(kind==2){g->lamp_touch=false;if(pg_ui_hit(pg_ui_lamp,(int)x,(int)y)&&hypotf(x-g->pointer_start_x,y-g->pointer_start_y)<20)pg_sleep(g,!g->sleeping);}
        return;
    }
    if(kind==0&&g->screen==PG_HOME&&!g->sleeping&&g->hud_seconds<=0){
        g->hud_seconds=8;g->reveal_touch=true;g->player_idle=0;return;
    }
    if(g->reveal_touch){if(kind==2)g->reveal_touch=false;return;}
    if(g->screen==PG_WASH&&!g->sleeping){pg_wash_pointer(g,kind,x,y);return;}
    if(g->screen==PG_ECHO&&!g->sleeping){pg_echo_pointer(g,kind,x,y);return;}
    if (kind == 0) {
        if (g->sleeping) { pg_sleep(g, false); return; }
        g->pointer_down = true; g->pointer_time = g->pointer_distance = 0;
        g->hold_handled = false; g->pointer_start_x = x; g->pointer_start_y = y;
        g->player_idle = 0;
        tilt_arrow(g,x,y);
        if(g->screen==PG_BALANCE&&g->balance.started&&y>=150&&y<320)g->tilt_x=pg_clamp((x-233)/150,-0.65f,0.65f);
    } else if (kind == 1 && g->pointer_down) {
        float dx = x - g->pointer_x, dy = y - g->pointer_y;
        float distance = sqrtf(dx * dx + dy * dy);
        g->pointer_distance += distance;
        if (g->screen == PG_HOME && fabsf(x - g->x) < 90 && fabsf(y - (g->y - 20)) < 90 &&
            g->pointer_distance > 50 && g->pointer_time > 0.35f) {
            pg_pet_gently(g); g->pointer_distance = 0; g->hold_handled = true;
        }
        if(g->screen==PG_BALANCE&&g->balance.started)g->tilt_x=pg_clamp((x-233)/150,-0.65f,0.65f);
        if (g->screen == PG_TILT) {
            if(!tilt_arrow(g,g->pointer_start_x,g->pointer_start_y)){
                g->tilt.ready=0;g->tilt_x = pg_clamp((x - 233) / 110, -1, 1);
                g->tilt_y = pg_clamp((y - 250) / 110, -1, 1);
            }
        }
    } else if (kind == 2 && g->pointer_down) {
        g->pointer_down = false;
        float dx = x - g->pointer_start_x, dy = y - g->pointer_start_y;
        int pages=0;int *page=menu_page(g,&pages);
        if(g->screen==PG_NAME_EDIT&&fabsf(dy)>=32&&fabsf(dx)<=64&&fabsf(dy)>fabsf(dx)*1.25f){
            name_letter(g,dy<0?1:-1);
        }
        else if(page&&fabsf(dx)>=48&&fabsf(dy)<=64&&fabsf(dx)>fabsf(dy)*1.25f){
            int next=*page+(dx<0?1:-1);
            if(next>=0&&next<pages){*page=next;++g->revision;if(!g->muted)g->sound=PG_SOUND_TAP;}
            /* Even a swipe at the first/last page consumes its release. */
        }
        else if (!g->hold_handled && dx * dx + dy * dy < 400 &&
                 ((!page&&g->screen!=PG_NAME_EDIT)||g->pointer_distance<24))
            pg_tap(g, (int)x, (int)y);
        if (g->screen == PG_TILT||g->screen==PG_BALANCE) g->tilt_x = g->tilt_y = 0;
    }
    g->pointer_x = x; g->pointer_y = y;
}

void pg_tap(pg_game *g, int x, int y) {
    if(g->standby)return;
    if(!pg_ui_visible((float)x,(float)y))return;
    g->player_idle = 0;
    if(g->screen==PG_HOME&&g->pet.phase!=PG_HATCHING&&pg_ui_hit(pg_ui_lamp,x,y)){pg_sleep(g,!g->sleeping);return;}
    if(g->screen==PG_HOME&&!g->sleeping&&g->hud_seconds<=0&&!(g->top_hud_seconds>0&&pg_ui_hit(pg_ui_home_settings,x,y))){g->hud_seconds=8;return;}
    g->hud_seconds=8;
    if (g->sleeping) { pg_sleep(g, false); return; }
    if (g->pet.phase == PG_HATCHING) return;
    if (g->pet.phase == PG_DEAD) {
        if(pg_ui_hit((pg_ui_rect){110,347,246,45},x,y)){
            if(!g->memorial_reset_armed)g->memorial_reset_armed=true;
            else {g->pet.death_acknowledged=true;pg_new_egg(g);}
        }else g->memorial_reset_armed=false;
        return;
    }
    if (g->screen != PG_HOME && g->screen != PG_NAME_EDIT && pg_ui_hit((g->screen==PG_GAMES||g->screen==PG_SETTINGS)?pg_ui_games_home:pg_ui_back,x,y)) {
        if(g->screen==PG_SETTINGS||(g->screen==PG_GAMES&&g->games_page)){g->screen=PG_HOME;g->pointer_down=false;++g->revision;}
        else pg_back(g);
        return;
    }
    ++g->revision;
    if (!g->muted) g->sound = PG_SOUND_TAP;
    switch (g->screen) {
    case PG_HOME:
        if(g->top_hud_seconds>0&&pg_ui_hit(pg_ui_home_settings,x,y)){g->screen=PG_SETTINGS;break;}
        if (pg_ui_dock_hit(0,x,y)||pg_ui_dock_hit(1,x,y)||pg_ui_dock_hit(2,x,y)||pg_ui_dock_hit(3,x,y)) {
            if (pg_ui_dock_hit(0,x,y)) {
                if(g->pet.phase==PG_EGG){pg_egg_care(g,12,4);pg_say(g,"WARMED BY KIND HANDS.",PG_SOUND_WARM);}
                else g->screen=PG_FOOD;
            }
            else if (pg_ui_dock_hit(1,x,y)) g->screen = PG_CARE;
            else if (pg_ui_dock_hit(2,x,y)) {
                if (g->pet.phase == PG_EGG) g->screen = PG_CARD;
                else g->screen = PG_GAMES;
            } else if (pg_ui_dock_hit(3,x,y)) { g->screen = PG_ALBUM; g->album_page = 0;g->album_expanded=false; }
        } else if(g->pet.legacy_count && x>=80 && x<=122 && y>=286 && y<=326){
            char memory[80];snprintf(memory,sizeof memory,"A LITTLE KEEPSAKE FROM %s.",g->pet.legacy_name[0]?g->pet.legacy_name:"BEFORE");
            pg_say(g,memory,PG_SOUND_NONE);
        } else if (fabsf(x - g->x) < 70 && fabsf(y - (g->y - 20)) < 80) {
            g->screen = g->invitation ? PG_GAMES : PG_CARD;
        } else if(x>=112&&x<=177&&y>=272&&y<=305&&g->pet.phase==PG_ALIVE){
            int choices=(g->pet.milestones&4u)?3:(g->pet.milestones&2u)?2:1;
            g->pet.bed_style=(uint8_t)((g->pet.bed_style+1)%choices);g->dirty=true;
            pg_say(g,choices>1?"A LITTLE CHANGE. STILL THEIR HOME.":"A SAFE SPOT FOR LITTLE DREAMS.",PG_SOUND_NONE);
        } else if (x > 315 && y > 190 && y < 320) {
            if (g->pet.milestones & 1u) {
                g->pet.plant_style = (g->pet.plant_style + 1) % 3; g->dirty = true;
                pg_say(g, "A NEW LEAF FOR YOUR LITTLE WORLD.", PG_SOUND_HAPPY);
            } else pg_say(g, "A LITTLE SOMETHING TO DISCOVER.", PG_SOUND_NONE);
        }
        break;
    case PG_NAME_EDIT: {
        size_t n=strlen(g->name_edit);
        if(pg_ui_hit(pg_ui_confirm,x,y)){name_finish(g,true);break;}
        if(pg_ui_hit(pg_ui_cancel,x,y)){name_finish(g,false);break;}
        if(pg_ui_hit(pg_ui_name_previous,x,y)){if(g->name_cursor>0)--g->name_cursor;break;}
        if(pg_ui_hit(pg_ui_name_next,x,y)){if(g->name_cursor<(int)n&&g->name_cursor<PG_NAME_MAX-1)++g->name_cursor;break;}
        if(pg_ui_hit(pg_ui_name_delete,x,y)){
            if(g->name_cursor<(int)n)memmove(g->name_edit+g->name_cursor,g->name_edit+g->name_cursor+1,n-(size_t)g->name_cursor);
            else if(n)g->name_edit[--n]=0;
            if(g->name_cursor>(int)strlen(g->name_edit))g->name_cursor=(int)strlen(g->name_edit);
        }
        break;
    }
    case PG_CARD:
        if(pg_ui_hit(pg_ui_rename,x,y)){pg_name_begin(g);break;}
        if (pg_ui_hit((pg_ui_rect){119,340,228,37},x,y)) { g->screen = PG_HOME; pg_pet_gently(g); }
        break;
    case PG_FOOD:
        if (y >= 160 && y <= 304) {
            int i = (x - 81) / 106;
            if (x >= 81 && i >= 0 && i < 3 && x < 81+i*106+94) pg_feed(g, i);
        }
        break;
    case PG_CARE:
        if (x >= 95 && x < 371) {
            if (y >= 155 && y < 213) {
                g->screen = PG_HOME;
                if (g->pet.phase == PG_EGG) { pg_egg_care(g, 12, 4); pg_say(g, "WARMED BY KIND HANDS.", PG_SOUND_WARM); }
                else pg_pet_gently(g);
            } else if (y >= 222 && y < 280) { if(g->pet.phase==PG_ALIVE)pg_open_game(g,5);else {pg_clean(g);g->screen=PG_HOME;} }
            else if (y >= 289 && y < 347) {
                if (g->pet.phase == PG_EGG) {
                    g->pet.sheltered = true; pg_egg_care(g, 3, 10);
                    pg_say(g, "A SOFT NEST FOR A SMALL MYSTERY.", PG_SOUND_WARM); g->screen = PG_HOME;
                }
            }
        }
        break;
    case PG_GAMES:
        if(g->games_page<1&&pg_ui_hit(pg_ui_page_next,x,y)){++g->games_page;break;}
        if(g->games_page&&pg_ui_hit(pg_ui_page_previous,x,y)){--g->games_page;break;}
        if (x >= 95 && x < 371) {
            if(g->games_page==1){if(y>=148&&y<212)pg_open_game(g,3);else if(y>=221&&y<283)pg_open_game(g,4);else if(y>=294&&y<356)pg_open_game(g,5);}
            else if (y >= 148 && y < 212) pg_open_game(g, 0);
            else if (y >= 221 && y < 283) pg_open_game(g, 1);
            else if (y >= 294 && y < 356) pg_open_game(g, 2);
        }
        break;
    case PG_ECHO:
        if(x>=119&&x<=347&&y>=343&&y<=383){
            if(g->echo.stage==PG_ECHO_DONE)pg_echo_start(g);
            else pg_echo_replay(g);
        }
        break;
    case PG_TTT:
        if (g->ttt.finished && y > 337 && y < 382) pg_ttt_start(g);
        else if (pg_ui_hit(pg_ui_paw_board,x,y))
            pg_ttt_move(g, ((y-pg_ui_paw_board.y)/PG_UI_PAW_STEP)*3+(x-pg_ui_paw_board.x)/PG_UI_PAW_STEP);
        break;
    case PG_TILT:
        if(!g->tilt.finished&&pg_ui_hit((pg_ui_rect){201,33,64,58},x,y)){pg_tilt_recenter(g);break;}
        if (g->tilt.finished && y > 337 && y < 382) pg_tilt_start(g);
        break;
    case PG_RHYTHM:
        if (g->rhythm.finished && y > 337 && y < 382) pg_rhythm_start(g);
        else if (y >= 190 && y < 315 && x >= 85 && x < 382) pg_rhythm_tap(g, (x - 85) / 99);
        break;
    case PG_ALBUM:
        if(g->album_expanded)break;
        if(x>=90&&x<=371&&y>=124&&y<327){
            unsigned row=(unsigned)(y-124)/37,offset=(unsigned)g->album_page*5+row;
            unsigned n=g->pet.event_count<PG_EVENT_CAP?g->pet.event_count:PG_EVENT_CAP;
            if(row<5&&offset<n){g->album_selected=g->pet.events[(g->pet.event_count-1-offset)%PG_EVENT_CAP];g->album_expanded=true;}
        }else if (y > 325 && y < 380){unsigned pages=((g->pet.event_count<PG_EVENT_CAP?g->pet.event_count:PG_EVENT_CAP)+4)/5;
            g->album_page=(g->album_page+1)%(int)(pages?pages:1);}
        break;
    case PG_HELP:
        if(x>=151&&x<=315&&y>=344&&y<=380)g->icon_help_page=(g->icon_help_page+1)%8;
        break;
    case PG_BALANCE:
        if(pg_ui_hit((pg_ui_rect){123,327,220,54},x,y)){
            if(g->balance.finished)pg_balance_start(g);else if(!g->balance.started)pg_balance_go(g);
        }
        /* Large touch fallback for a child who prefers not to tilt. */

        break;
    case PG_WASH:
        if(g->wash.finished&&pg_ui_hit((pg_ui_rect){123,327,220,54},x,y))pg_wash_start(g);
        break;
    case PG_DEBUG:
        if(pg_ui_hit((pg_ui_rect){103,150,260,66},x,y))pg_debug_hatch(g);
        else if(pg_ui_hit((pg_ui_rect){103,238,260,66},x,y)){
            if(g->debug_reset_armed)pg_debug_reset(g);else g->debug_reset_armed=true;
        }
        break;
    case PG_SETTINGS: {
        int page=g->utilities.settings_page;
        if(pg_ui_hit(pg_ui_page_previous,x,y)&&page>0){--g->utilities.settings_page;break;}
        if(pg_ui_hit(pg_ui_page_next,x,y)&&page<2){++g->utilities.settings_page;break;}
        int row=pg_ui_hit(pg_ui_settings_rows[0],x,y)?0:pg_ui_hit(pg_ui_settings_rows[1],x,y)?1:pg_ui_hit(pg_ui_settings_rows[2],x,y)?2:-1;
        if(row<0)break;
        if(page==0){if(row==0){static const uint8_t levels[]={20,40,65,85,100};unsigned i=0;while(i<4&&g->pet.brightness>=levels[i])++i;g->pet.brightness=g->pet.brightness>=100?20:levels[i];g->dirty=true;}else if(row==1){g->muted=!g->muted;g->pet.muted=g->muted;g->dirty=true;}}
        else if(page==1){g->utilities.return_page=page;if(row==0)g->screen=PG_VOLUME;else if(row==1){g->device.auto_sleep=!g->device.auto_sleep;g->utilities.idle=0;pg_device_changed(g);}else pg_clock_begin(g);}
        else{g->utilities.return_page=page;if(row==0)g->screen=PG_DEVICE_STATUS;else if(row==1){g->screen=PG_HELP;g->icon_help_page=0;}else{g->screen=PG_DEBUG;g->debug_reset_armed=false;}}
        break;}
    case PG_VOLUME:
        if(pg_ui_hit(pg_ui_decrease,x,y)){if(g->device.volume>=25)g->device.volume-=25;pg_device_changed(g);}
        else if(pg_ui_hit(pg_ui_increase,x,y)){if(g->device.volume<100)g->device.volume+=25;pg_device_changed(g);}
        else if(pg_ui_hit(pg_ui_volume_mute,x,y)){g->muted=!g->muted;g->pet.muted=g->muted;g->dirty=true;}
        break;
    case PG_CLOCK_EDIT:
        if(pg_ui_hit(pg_ui_decrease,x,y))pg_clock_adjust(g,-1);
        else if(pg_ui_hit(pg_ui_increase,x,y))pg_clock_adjust(g,1);
        else if(pg_ui_hit(pg_ui_clock_previous,x,y))g->utilities.clock_field=(g->utilities.clock_field+4)%5;
        else if(pg_ui_hit(pg_ui_clock_next,x,y))g->utilities.clock_field=(g->utilities.clock_field+1)%5;
        else if(pg_ui_hit(pg_ui_cancel,x,y))pg_back(g);
        else if(pg_ui_hit(pg_ui_confirm,x,y)){if(pg_calendar_save(g,&g->utilities.edit))pg_back(g);else g->utilities.clock_error=true;}
        break;
    default: break;
    }
}
